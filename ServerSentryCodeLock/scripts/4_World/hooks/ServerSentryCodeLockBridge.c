// Only actor/RPC metadata is scoped here. The lock always comes from CaptureLockEvent.
class ServerSentryCodeLockRpcContext
{
    protected static PlayerIdentity s_Sender;
    protected static int s_RpcType;
    protected static bool s_Active;
    protected static int s_StartedAt;
    const static int MAX_CONTEXT_AGE_MS = 10000;

    static void Begin(PlayerIdentity sender, int rpcType)
    {
        End();
        s_Sender = sender;
        s_RpcType = rpcType;
        s_Active = true;
        if (GetGame()) s_StartedAt = GetGame().GetTime();
    }

    static void End()
    {
        s_Sender = null;
        s_RpcType = 0;
        s_Active = false;
        s_StartedAt = 0;
    }

    static bool IsActive()
    {
        if (!s_Active) return false;
        if (GetGame())
        {
            int now = GetGame().GetTime();
            if (now < s_StartedAt || (now - s_StartedAt) > MAX_CONTEXT_AGE_MS)
            {
                End();
                return false;
            }
        }
        return true;
    }

    static PlayerIdentity GetSender()
    {
        if (!IsActive()) return null;
        return s_Sender;
    }

    static int GetRpcType()
    {
        if (!IsActive()) return 0;
        return s_RpcType;
    }
}

modded class CodeLockLogContext
{
    ref ServerSentryCodeLockEventData SentryData;
}

class ServerSentryCodeLockBridge
{
    static bool Enabled()
    {
        if (!GetGame() || !GetGame().IsServer()) return false;
        return ServerSentryCodeLockConsts.ENABLED && ServerSentryCodeLockConsts.LOG_CODELOCK_EVENTS && ServerSentryDefaults.LogCodeLockEvents() && ServerSentryConfigManager.Get().IsAllowedToStartTelemetryLogging();
    }

    static bool IsSyncLogType(string type)
    {
        string normalizedType = Normalize(type);
        return normalizedType == "synch" || normalizedType == "sync";
    }

    // Copy primitive metadata while both the exact lock and its original parent exist.
    // Emission never dereferences either object, including after a raid deletes the lock.
    static ServerSentryCodeLockEventData Capture(CodeLock codelock, PlayerIdentity identity, EntityAI parent = null)
    {
        if (!codelock || !Enabled()) return null;

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.sourceHook = "CodeLockLogger.WriteLockEvent";
        data.rpcType = ServerSentryCodeLockRpcContext.GetRpcType();
        data.extra = "snapshot=before_action";

        PlayerIdentity sender = ServerSentryCodeLockRpcContext.GetSender();
        if (sender) identity = sender;
        FillIdentity(data, identity);

        data.lockClassName = codelock.GetType();
        int lockId = codelock.GetID();
        data.lockObjectId = lockId.ToString();
        data.lockPosition.FromVector(codelock.GetPosition());
        data.lockHealth = codelock.GetHealth("", "Health");
        data.lockMaxHealth = codelock.GetMaxHealth("", "Health");
        data.isLocked = codelock.GetLockState();
        data.lockStateKnown = true;
        data.ownerBohemiaId = codelock.GetOwner();
        data.hasOwner = data.ownerBohemiaId != "";

        array<string> guests = codelock.GetGuests();
        if (guests) data.guestCount = guests.Count();
        if (data.actorBohemiaId != "")
        {
            data.actorIsOwner = codelock.IsOwner(data.actorBohemiaId);
            data.actorIsGuest = codelock.IsGuest(data.actorBohemiaId);
        }

        if (!parent) parent = codelock.GetHierarchyParent();
        if (parent && !PlayerBase.Cast(parent))
        {
            data.parentClassName = parent.GetType();
            data.parentDisplayName = parent.GetDisplayName();
            int parentId = parent.GetID();
            data.parentObjectId = parentId.ToString();
            data.parentPosition.FromVector(parent.GetPosition());
            data.targetClassName = data.parentClassName;
            data.targetDisplayName = data.parentDisplayName;
            data.targetPosition.FromVector(parent.GetPosition());
            data.targetType = "codelock_parent";
            if (Fence.Cast(parent)) data.targetType = "fence";
            else if (TentBase.Cast(parent)) data.targetType = "tent";
        }
        else
        {
            data.targetType = "codelock";
            data.targetClassName = data.lockClassName;
            data.targetDisplayName = codelock.GetDisplayName();
            data.targetPosition.FromVector(codelock.GetPosition());
        }
        return data;
    }

    static void EmitLockEvent(CodeLockLogContext context, string type, string accessType, string result, string reason, bool claim, bool cut, float damage, float health)
    {
        if (!context || !context.SentryData || IsSyncLogType(type) || !Enabled()) return;
        string normalizedType = Normalize(type);
        string normalizedAccess = Normalize(accessType);
        string action = ResolveAction(normalizedType, normalizedAccess, cut);
        if (!ShouldLogEvent(normalizedType, action)) return;

        ServerSentryCodeLockEventData data = context.SentryData;
        data.codeLockAction = action;
        data.result = result;
        data.reason = reason;
        data.codeLockLogType = normalizedType;
        data.codeLockAccessType = normalizedAccess;
        data.raidDamage = damage;
        data.raidHealth = health;
        data.raidSuccess = cut && result == "success";
        data.raidClaim = claim;
        data.isAdmin = normalizedType == "admin";
        data.raw = "type=" + normalizedType + ";accessType=" + normalizedAccess;

        // Each call represents a real attempt. Never merge nearby or repeated attempts.
        ServerSentryApi.EmitCodeLockEvent(data);
    }

    // Joining is actor-only. Position-only legacy entries cannot identify a lock.
    static void EmitAdminJoin(string type, PlayerIdentity identity, string accessType)
    {
        if (Normalize(type) != "admin" || Normalize(accessType) != "join" || !identity || !Enabled()) return;
        if (!ShouldLogEvent("admin", "admin_join")) return;

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.sourceHook = "CodeLockLogger.WriteLog";
        data.codeLockAction = "admin_join";
        data.codeLockLogType = "admin";
        data.codeLockAccessType = "join";
        data.result = "observed";
        data.reason = "admin_joined";
        data.isAdmin = true;
        data.lockClassName = "";
        data.targetType = "none";
        FillIdentity(data, identity);
        ServerSentryApi.EmitCodeLockEvent(data);
    }

    static void EmitInternal(ServerSentryCodeLockEventData data, string action, string sourceHook, string result, string reason)
    {
        if (!data || !Enabled()) return;
        data.codeLockAction = action;
        data.sourceHook = sourceHook;
        data.result = result;
        data.reason = reason;

        // Optional low-level diagnostics may overlap. Only these use bounded deduplication.
        string signature = sourceHook + "|" + action + "|" + result + "|" + data.actorSteamId + "|" + data.actorBohemiaId + "|" + data.lockObjectId + "|" + data.extra;
        if (ServerSentryCodeLockDeduper.ShouldEmit(signature)) ServerSentryApi.EmitCodeLockEvent(data);
    }

    protected static void FillIdentity(ServerSentryCodeLockEventData data, PlayerIdentity identity)
    {
        if (!identity) return;
        data.actorSteamId = identity.GetPlainId();
        data.actorBohemiaId = identity.GetId();
        data.actorName = identity.GetName();
        Man player = identity.GetPlayer();
        if (player) data.actorPosition.FromVector(player.GetPosition());
    }

    protected static string Normalize(string value)
    {
        string result = value;
        result.ToLower();
        return result;
    }

    protected static string ResolveAction(string type, string accessType, bool cut)
    {
        if (type == "attach") return "attach";
        if (type == "raid")
        {
            if (cut) return "raid_success";
            return "raid_progress";
        }
        if (type == "admin")
        {
            if (accessType == "open") return "admin_open";
            if (accessType == "change") return "admin_change";
            if (accessType == "remove") return "admin_remove";
            if (accessType == "join") return "admin_join";
            return "admin_action";
        }
        if (type == "access")
        {
            if (accessType == "setcode") return "set_code";
            if (accessType == "notowner") return "not_owner_denied";
            if (accessType == "entercode") return "enter_code";
            if (accessType == "claimowner") return "claim_owner";
            if (accessType == "addguest") return "add_guest";
            if (accessType == "removelock") return "remove_lock";
            if (accessType == "changecode") return "change_code";
            if (accessType == "passcode_failed") return "passcode_failed";
            return "quick_access";
        }
        return "codelock_" + type;
    }

    protected static bool ShouldLogEvent(string type, string action)
    {
        if (type == "access" && !ServerSentryCodeLockConsts.LOG_ACCESS_EVENTS) return false;
        if (type == "attach" && !ServerSentryCodeLockConsts.LOG_ATTACH_EVENTS) return false;
        if (type == "raid" && !ServerSentryCodeLockConsts.LOG_RAID_EVENTS) return false;
        if (type == "admin" && !ServerSentryCodeLockConsts.LOG_ADMIN_EVENTS) return false;
        if (action == "set_code") return ServerSentryCodeLockConsts.LOG_SET_CODE;
        if (action == "change_code") return ServerSentryCodeLockConsts.LOG_CHANGE_CODE;
        if (action == "not_owner_denied") return ServerSentryCodeLockConsts.LOG_NOT_OWNER_DENIED;
        if (action == "enter_code") return ServerSentryCodeLockConsts.LOG_ENTER_CODE;
        if (action == "quick_access") return ServerSentryCodeLockConsts.LOG_QUICK_ACCESS;
        if (action == "claim_owner") return ServerSentryCodeLockConsts.LOG_CLAIM_OWNER;
        if (action == "add_guest") return ServerSentryCodeLockConsts.LOG_ADD_GUEST;
        if (action == "remove_lock") return ServerSentryCodeLockConsts.LOG_REMOVE_LOCK;
        if (action == "passcode_failed") return ServerSentryCodeLockConsts.LOG_FAILED_PASSCODE;
        if (action == "raid_progress") return ServerSentryCodeLockConsts.LOG_RAID_PROGRESS;
        if (action == "raid_success") return ServerSentryCodeLockConsts.LOG_RAID_SUCCESS;
        if (action == "admin_open") return ServerSentryCodeLockConsts.LOG_ADMIN_OPEN;
        if (action == "admin_change") return ServerSentryCodeLockConsts.LOG_ADMIN_CHANGE;
        if (action == "admin_remove") return ServerSentryCodeLockConsts.LOG_ADMIN_REMOVE;
        if (action == "admin_join") return ServerSentryCodeLockConsts.LOG_ADMIN_JOIN;
        if (action == "admin_action") return ServerSentryCodeLockConsts.LOG_ADMIN_OTHER;
        return true;
    }
}

modded class CodeLockLogger
{
    override CodeLockLogContext CaptureLockEvent(CodeLock lock, PlayerIdentity identity, EntityAI parent = null)
    {
        CodeLockLogContext context = super.CaptureLockEvent(lock, identity, parent);
        if (context) context.SentryData = ServerSentryCodeLockBridge.Capture(lock, identity, context.Parent);
        return context;
    }

    override void WriteLockEvent(CodeLockLogContext context, string type, string accessType = "", string result = "success", string reason = "", bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        if (ServerSentryCodeLockBridge.IsSyncLogType(type)) return;
        super.WriteLockEvent(context, type, accessType, result, reason, claim, cut, damage, health);
        ServerSentryCodeLockBridge.EmitLockEvent(context, type, accessType, result, reason, claim, cut, damage, health);
    }

    override void WriteLog(string type, PlayerIdentity playerId = null, vector pos = "0 0 0", string accessType = "", bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        if (ServerSentryCodeLockBridge.IsSyncLogType(type)) return;
        super.WriteLog(type, playerId, pos, accessType, claim, cut, damage, health);
        ServerSentryCodeLockBridge.EmitAdminJoin(type, playerId, accessType);
    }
}

modded class CodeLockServerRPC
{
    override void ServerRPCHandler(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        ServerSentryCodeLockRpcContext.Begin(sender, rpc_type);
        super.ServerRPCHandler(sender, target, rpc_type, ctx);
        ServerSentryCodeLockRpcContext.End();
    }
}

modded class CodeLock
{
    override void LockServer(ItemBase parent, string passcode)
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_LOCK_SERVER) data = ServerSentryCodeLockBridge.Capture(this, null, parent);
        super.LockServer(parent, passcode);
        if (!data) return;
        string result = "failed";
        if (GetLockState()) result = "success";
        ServerSentryCodeLockBridge.EmitInternal(data, "lock_server", "CodeLock.LockServer", result, "internal_lock_server");
    }

    override void NewUnlockServer(EntityAI player, EntityAI parent)
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_UNLOCK_SERVER)
        {
            PlayerIdentity identity;
            PlayerBase actor = PlayerBase.Cast(player);
            if (actor) identity = actor.GetIdentity();
            data = ServerSentryCodeLockBridge.Capture(this, identity, parent);
        }
        super.NewUnlockServer(player, parent);
        if (!data) return;
        string result = "failed";
        if (!GetLockState() && GetHierarchyParent() != parent) result = "success";
        ServerSentryCodeLockBridge.EmitInternal(data, "unlock_server", "CodeLock.NewUnlockServer", result, "internal_unlock_server");
    }

    override void ServerSetOwner(string id)
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_OWNER_SET) data = ServerSentryCodeLockBridge.Capture(this, null);
        super.ServerSetOwner(id);
        if (data && data.ownerBohemiaId != GetOwner())
        {
            ServerSentryCodeLockBridge.EmitInternal(data, "owner_set", "CodeLock.ServerSetOwner", "success", "owner_changed");
        }
    }

    override void ServerAddGuest(string id)
    {
        ServerSentryCodeLockEventData data;
        bool wasGuest;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_GUEST_ADD)
        {
            data = ServerSentryCodeLockBridge.Capture(this, null);
            wasGuest = IsGuest(id);
            if (data) data.extra = data.extra + ";guestBohemiaId=" + id;
        }
        super.ServerAddGuest(id);
        if (data && !wasGuest && IsGuest(id))
        {
            ServerSentryCodeLockBridge.EmitInternal(data, "guest_add", "CodeLock.ServerAddGuest", "success", "guest_added");
        }
    }

    override void ServerClearGuests()
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_GUEST_CLEAR) data = ServerSentryCodeLockBridge.Capture(this, null);
        super.ServerClearGuests();
        array<string> guests = GetGuests();
        if (data && data.guestCount > 0 && guests && guests.Count() == 0)
        {
            ServerSentryCodeLockBridge.EmitInternal(data, "guest_clear", "CodeLock.ServerClearGuests", "success", "guests_cleared");
        }
    }
}
