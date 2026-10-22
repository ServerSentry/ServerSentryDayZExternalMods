class ServerSentryCodeLockRpcContext
{
    protected static PlayerIdentity s_Sender;
    protected static Object s_Target;
    protected static int s_RpcType;
    protected static bool s_Active;
    protected static int s_StartedAt;
    const static int MAX_CONTEXT_AGE_MS = 10000;

    static void Begin(PlayerIdentity sender, Object target, int rpcType)
    {
        // Always clear a stale context before starting the next RPC.
        End();

        s_Sender = sender;
        s_Target = target;
        s_RpcType = rpcType;
        s_Active = true;
        if (GetGame())
        {
            s_StartedAt = GetGame().GetTime();
        }
    }

    static void End()
    {
        s_Sender = null;
        s_Target = null;
        s_RpcType = 0;
        s_Active = false;
        s_StartedAt = 0;
    }

    static bool IsActive()
    {
        if (!s_Active)
        {
            return false;
        }

        if (GetGame() && s_StartedAt > 0)
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
        if (!IsActive())
        {
            return null;
        }

        return s_Sender;
    }

    static Object GetTarget()
    {
        if (!IsActive())
        {
            return null;
        }

        return s_Target;
    }

    static PlayerBase GetPlayer()
    {
        if (!IsActive() || !s_Sender)
        {
            return null;
        }

        return PlayerBase.Cast(s_Sender.GetPlayer());
    }

    static int GetRpcType()
    {
        if (!IsActive())
        {
            return 0;
        }

        return s_RpcType;
    }

    static bool IsPasscodeCheckRpc()
    {
        return IsActive() && (s_RpcType == CLRPC.RPC_SERVER_ENTERCODE || s_RpcType == CLRPC.RPC_SERVER_CLAIMCODE);
    }
}

class ServerSentryCodeLockBridge
{
    static bool Enabled()
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return false;
        }

        return ServerSentryCodeLockConsts.ENABLED && ServerSentryCodeLockConsts.LOG_CODELOCK_EVENTS && ServerSentryDefaults.LogCodeLockEvents() && ServerSentryConfigManager.Get().IsAllowedToStartTelemetryLogging();
    }

    static bool IsSyncLogType(string type)
    {
        string normalizedType = Normalize(type);
        return normalizedType == "synch" || normalizedType == "sync";
    }

    static void EmitLegacyLog(string type, PlayerIdentity playerId = null, vector pos = "0 0 0", string accessType = "", bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        if (IsSyncLogType(type) || !Enabled())
        {
            return;
        }

        string normalizedType = Normalize(type);
        string normalizedAccess = Normalize(accessType);
        string action = ResolveLegacyAction(normalizedType, normalizedAccess, cut);

        if (!ShouldLogLegacy(normalizedType, normalizedAccess, cut, action))
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = action;
        data.result = ResolveLegacyResult(normalizedType, normalizedAccess, cut, action);
        data.reason = ResolveLegacyReason(normalizedType, normalizedAccess, cut);
        data.sourceHook = "CodeLockLogger.WriteLog";
        data.codeLockLogType = normalizedType;
        data.codeLockAccessType = normalizedAccess;
        data.raidDamage = damage;
        data.raidHealth = health;
        data.raidSuccess = cut;
        data.raidClaim = claim;
        data.isAdmin = normalizedType == "admin";
        data.raw = "type=" + normalizedType + ";accessType=" + normalizedAccess;

        FillIdentity(data, playerId);
        if (data.actorName == "" && ServerSentryCodeLockRpcContext.IsActive())
        {
            FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        }

        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (!player && playerId)
        {
            player = PlayerBase.Cast(playerId.GetPlayer());
        }
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }

        CodeLock codelock = ResolveRpcCodeLock();
        if (!codelock)
        {
            codelock = FindCodeLockAt(pos);
        }
        FillLockAndTarget(data, codelock, pos);

        EmitData(data, player);
    }

    static void EmitFailedPasscode(CodeLock codelock)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_FAILED_PASSCODE || !ServerSentryCodeLockRpcContext.IsPasscodeCheckRpc())
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "passcode_failed";
        data.result = "denied";
        data.reason = "wrong_passcode";
        data.sourceHook = "CodeLock.IsLocked";
        data.rpcType = ServerSentryCodeLockRpcContext.GetRpcType();

        PlayerIdentity sender = ServerSentryCodeLockRpcContext.GetSender();
        FillIdentity(data, sender);

        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }

        FillLockAndTarget(data, codelock, codelock.GetPosition());
        EmitData(data, player);
    }

    static void EmitInternalLock(CodeLock codelock, ItemBase parent)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_INTERNAL_LOCK_SERVER)
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "lock_server";
        data.result = "success";
        data.reason = "internal_lock_server";
        data.sourceHook = "CodeLock.LockServer";
        FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }
        FillLockAndTarget(data, codelock, codelock.GetPosition(), parent);
        EmitData(data, player);
    }

    static void EmitInternalUnlock(CodeLock codelock, EntityAI playerEntity, EntityAI parent)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_INTERNAL_UNLOCK_SERVER)
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "unlock_server";
        data.result = "success";
        data.reason = "internal_unlock_server";
        data.sourceHook = "CodeLock.NewUnlockServer";

        PlayerBase player = PlayerBase.Cast(playerEntity);
        if (!player)
        {
            player = ServerSentryCodeLockRpcContext.GetPlayer();
        }
        if (player && player.GetIdentity())
        {
            FillIdentity(data, player.GetIdentity());
            data.actorPosition.FromVector(player.GetPosition());
        }
        else
        {
            FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        }

        FillLockAndTarget(data, codelock, codelock.GetPosition(), EntityAI.Cast(parent));
        EmitData(data, player);
    }

    static void EmitInternalOwnerSet(CodeLock codelock, string id)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_INTERNAL_OWNER_SET)
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "owner_set";
        data.result = "success";
        data.reason = "internal_owner_set";
        data.sourceHook = "CodeLock.ServerSetOwner";
        data.ownerBohemiaId = id;
        FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }
        FillLockAndTarget(data, codelock, codelock.GetPosition());
        EmitData(data, player);
    }

    static void EmitInternalGuestAdd(CodeLock codelock, string id)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_INTERNAL_GUEST_ADD)
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "guest_add";
        data.result = "success";
        data.reason = "internal_guest_add";
        data.sourceHook = "CodeLock.ServerAddGuest";
        data.extra = "guestBohemiaId=" + id;
        FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }
        FillLockAndTarget(data, codelock, codelock.GetPosition());
        EmitData(data, player);
    }

    static void EmitInternalGuestClear(CodeLock codelock)
    {
        if (!Enabled() || !ServerSentryCodeLockConsts.LOG_INTERNAL_GUEST_CLEAR)
        {
            return;
        }

        ServerSentryCodeLockEventData data = new ServerSentryCodeLockEventData();
        data.sourceMod = "CodeLock";
        data.codeLockAction = "guest_clear";
        data.result = "success";
        data.reason = "internal_guest_clear";
        data.sourceHook = "CodeLock.ServerClearGuests";
        FillIdentity(data, ServerSentryCodeLockRpcContext.GetSender());
        PlayerBase player = ServerSentryCodeLockRpcContext.GetPlayer();
        if (player)
        {
            data.actorPosition.FromVector(player.GetPosition());
        }
        FillLockAndTarget(data, codelock, codelock.GetPosition());
        EmitData(data, player);
    }

    protected static void EmitData(ServerSentryCodeLockEventData data, PlayerBase player = null)
    {
        if (!data)
        {
            return;
        }

        string signature = data.sourceHook + "|" + data.codeLockAction + "|" + data.actorSteamId + "|" + data.actorBohemiaId + "|" + data.lockObjectId + "|" + data.targetClassName + "|" + data.targetPosition.x.ToString() + ":" + data.targetPosition.y.ToString() + ":" + data.targetPosition.z.ToString();
        if (!ServerSentryCodeLockDeduper.ShouldEmit(signature))
        {
            return;
        }

        ServerSentryApi.EmitCodeLockEvent(data, DayZPlayer.Cast(player));
    }

    protected static void FillIdentity(ServerSentryCodeLockEventData data, PlayerIdentity identity)
    {
        if (!data || !identity)
        {
            return;
        }

        data.actorSteamId = identity.GetPlainId();
        data.actorBohemiaId = identity.GetId();
        data.actorName = identity.GetName();
    }

    protected static void FillLockAndTarget(ServerSentryCodeLockEventData data, CodeLock codelock, vector fallbackPos, EntityAI forcedParent = null)
    {
        if (!data)
        {
            return;
        }

        if (codelock)
        {
            data.lockClassName = codelock.GetType();
            data.lockObjectId = codelock.ToString();
            data.lockPosition.FromVector(codelock.GetPosition());
            data.lockHealth = codelock.GetHealth();
            data.lockMaxHealth = codelock.GetMaxHealth("", "Health");
            data.isLocked = codelock.GetLockState();
            data.lockStateKnown = true;
            data.ownerBohemiaId = codelock.GetOwner();
            data.hasOwner = data.ownerBohemiaId != "";

            array<string> guests = codelock.GetGuests();
            if (guests)
            {
                data.guestCount = guests.Count();
            }

            if (data.actorBohemiaId != "")
            {
                data.actorIsOwner = codelock.IsOwner(data.actorBohemiaId);
                data.actorIsGuest = codelock.IsGuest(data.actorBohemiaId);
            }
        }
        else
        {
            data.lockPosition.FromVector(fallbackPos);
        }

        EntityAI parent = forcedParent;
        if (!parent && codelock)
        {
            parent = EntityAI.Cast(codelock.GetHierarchyParent());
        }

        if (parent)
        {
            data.parentClassName = parent.GetType();
            data.parentDisplayName = parent.GetDisplayName();
            data.parentObjectId = parent.ToString();
            data.parentPosition.FromVector(parent.GetPosition());

            data.targetClassName = data.parentClassName;
            data.targetDisplayName = data.parentDisplayName;
            data.targetPosition.FromVector(parent.GetPosition());

            if (Fence.Cast(parent))
            {
                data.targetType = "fence";
            }
            else if (TentBase.Cast(parent))
            {
                data.targetType = "tent";
            }
            else
            {
                data.targetType = "codelock_parent";
            }
        }
        else
        {
            data.targetType = "codelock";
            data.targetClassName = data.lockClassName;
            data.targetDisplayName = data.lockClassName;
            data.targetPosition.FromVector(fallbackPos);
        }
    }

    protected static CodeLock ResolveRpcCodeLock()
    {
        Object target = ServerSentryCodeLockRpcContext.GetTarget();
        if (!target)
        {
            return null;
        }

        CodeLock directLock = CodeLock.Cast(target);
        if (directLock)
        {
            return directLock;
        }

        ItemBase parent = ItemBase.Cast(target);
        if (parent)
        {
            return CodeLock.Cast(parent.GetCodeLockRS());
        }

        return null;
    }

    protected static CodeLock FindCodeLockAt(vector pos)
    {
        if (!GetGame())
        {
            return null;
        }

        array<Object> objects = new array<Object>();
        array<CargoBase> cargos = new array<CargoBase>();
        GetGame().GetObjectsAtPosition3D(pos, 2.0, objects, cargos);

        foreach (Object obj : objects)
        {
            CodeLock directLock = CodeLock.Cast(obj);
            if (directLock)
            {
                return directLock;
            }
        }

        foreach (Object parentObj : objects)
        {
            ItemBase itemParent = ItemBase.Cast(parentObj);
            if (!itemParent)
            {
                continue;
            }

            CodeLock attachedLock = CodeLock.Cast(itemParent.GetCodeLockRS());
            if (attachedLock)
            {
                return attachedLock;
            }
        }

        return null;
    }

    protected static string Normalize(string value)
    {
        string result = value;
        result.ToLower();
        return result;
    }

    protected static string ResolveLegacyAction(string type, string accessType, bool cut)
    {
        if (type == "attach")
        {
            return "attach";
        }

        if (type == "raid")
        {
            if (cut)
            {
                return "raid_success";
            }
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
            return "quick_access";
        }

        return "codelock_" + type;
    }

    protected static string ResolveLegacyResult(string type, string accessType, bool cut, string action)
    {
        if (action == "not_owner_denied")
        {
            return "denied";
        }

        if (type == "raid" && !cut)
        {
            return "progress";
        }

        if (action.IndexOf("codelock_") == 0)
        {
            return "observed";
        }

        return "success";
    }

    protected static string ResolveLegacyReason(string type, string accessType, bool cut)
    {
        if (type == "raid" && cut)
        {
            return "raid_completed";
        }
        if (type == "raid")
        {
            return "raid_increment";
        }
        if (type == "access" && accessType == "notowner")
        {
            return "not_owner";
        }
        return type + "_" + accessType;
    }

    protected static bool ShouldLogLegacy(string type, string accessType, bool cut, string action)
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
    override void WriteLog(string type, PlayerIdentity playerId = null, vector pos = "0 0 0", string accessType = "", bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        // Ignore synchronization before either the original file logger or telemetry runs.
        if (ServerSentryCodeLockBridge.IsSyncLogType(type))
        {
            return;
        }

        super.WriteLog(type, playerId, pos, accessType, claim, cut, damage, health);
        ServerSentryCodeLockBridge.EmitLegacyLog(type, playerId, pos, accessType, claim, cut, damage, health);
    }
}

modded class CodeLockServerRPC
{
    override void ServerRPCHandler(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        ServerSentryCodeLockRpcContext.Begin(sender, target, rpc_type);
        super.ServerRPCHandler(sender, target, rpc_type, ctx);
        ServerSentryCodeLockRpcContext.End();
    }
}

modded class CodeLock
{
    override bool IsLocked(string ui_Passcode)
    {
        bool isLocked = super.IsLocked(ui_Passcode);
        if (isLocked)
        {
            ServerSentryCodeLockBridge.EmitFailedPasscode(this);
        }
        return isLocked;
    }

    override void LockServer(ItemBase parent, string passcode)
    {
        super.LockServer(parent, passcode);
        ServerSentryCodeLockBridge.EmitInternalLock(this, parent);
    }

    override void NewUnlockServer(EntityAI player, EntityAI parent)
    {
        super.NewUnlockServer(player, parent);
        ServerSentryCodeLockBridge.EmitInternalUnlock(this, player, parent);
    }

    override void ServerSetOwner(string id)
    {
        super.ServerSetOwner(id);
        ServerSentryCodeLockBridge.EmitInternalOwnerSet(this, id);
    }

    override void ServerAddGuest(string id)
    {
        super.ServerAddGuest(id);
        ServerSentryCodeLockBridge.EmitInternalGuestAdd(this, id);
    }

    override void ServerClearGuests()
    {
        super.ServerClearGuests();
        ServerSentryCodeLockBridge.EmitInternalGuestClear(this);
    }
}
