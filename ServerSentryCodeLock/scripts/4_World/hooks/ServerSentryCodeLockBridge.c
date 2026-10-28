// Extension-owned context; no patched CodeLock logger API is required.
class ServerSentryCodeLockOperation
{
    ref ServerSentryCodeLockEventData Data;
    CodeLock Lock;
    EntityAI Parent;
    bool WasOpen;
    bool PasscodeChecked;
    bool PasscodeAccepted;
    bool MenuRequested;
    bool UnlockObserved;
    bool RemovalSucceeded;
    float FinalHealth;
    string Reply;
}

// The RPC envelope supplies identity only, never a guessed lock target.
class ServerSentryCodeLockRpcContext
{
    protected static PlayerIdentity s_Sender;
    protected static int s_RpcType;
    protected static int s_StartedAt;
    protected static ref ServerSentryCodeLockEventData s_DeniedCodeChange;
    protected static CodeLock s_DeniedLock;
    protected static bool s_AmbiguousDenial;
    protected static bool s_DenialEmitted;

    static void Begin(PlayerIdentity sender, int rpcType)
    {
        End();
        s_Sender = sender;
        s_RpcType = rpcType;
        if (GetGame()) s_StartedAt = GetGame().GetTime();
    }

    static void End()
    {
        s_Sender = null;
        s_RpcType = 0;
        s_StartedAt = 0;
        s_DeniedCodeChange = null;
        s_DeniedLock = null;
        s_AmbiguousDenial = false;
        s_DenialEmitted = false;
    }

    static PlayerIdentity GetSender()
    {
        if (!s_Sender || !GetGame()) return null;
        int now = GetGame().GetTime();
        if (now < s_StartedAt || now - s_StartedAt > 10000)
        {
            End();
            return null;
        }
        return s_Sender;
    }

    static int GetRpcType()
    {
        if (!GetSender()) return 0;
        return s_RpcType;
    }

    // SETCODE can be rejected before SetCode() is called. Retain the exact
    // object checked for ownership, and emit only if the logger confirms denial.
    static void ObserveOwnerCheck(CodeLock codeLock, string identityId, bool isOwner)
    {
        PlayerIdentity sender = GetSender();
        if (isOwner || !sender || identityId != sender.GetId()) return;
        if (s_RpcType != CLRPC.RPC_SERVER_SETCODE || ServerSentryCodeLockBridge.Current()) return;
        if (s_AmbiguousDenial || s_DenialEmitted) return;
        if (s_DeniedLock && s_DeniedLock != codeLock)
        {
            s_AmbiguousDenial = true;
            s_DeniedCodeChange = null;
            return;
        }
        s_DeniedLock = codeLock;
        s_DeniedCodeChange = ServerSentryCodeLockBridge.Capture(codeLock, sender);
        if (s_DeniedCodeChange) s_DeniedCodeChange.sourceHook = "CodeLockServerRPC.ServerRPCHandler";
    }

    static void EmitCodeChangeDenied(PlayerIdentity identity)
    {
        PlayerIdentity sender = GetSender();
        if (!sender || !identity || sender.GetId() != identity.GetId()) return;
        if (!s_DeniedCodeChange || s_AmbiguousDenial || s_DenialEmitted) return;
        s_DenialEmitted = true;
        ServerSentryCodeLockOperation operation = new ServerSentryCodeLockOperation();
        operation.Data = s_DeniedCodeChange;
        ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "notowner", "denied", "change_requires_owner");
    }
}

class ServerSentryCodeLockBridge
{
    protected static ref ServerSentryCodeLockOperation s_Current;
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
        data.sourceHook = "ServerSentryCodeLockBridge";
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
            data.actorIsOwner = data.ownerBohemiaId == data.actorBohemiaId;
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

    static void EmitLockEvent(ServerSentryCodeLockOperation context, string type, string accessType, string result, string reason, bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        if (!context || !context.Data || IsSyncLogType(type) || !Enabled()) return;
        string normalizedType = Normalize(type);
        string normalizedAccess = Normalize(accessType);
        string action = ResolveAction(normalizedType, normalizedAccess, cut);
        if (!ShouldLogEvent(normalizedType, action)) return;

        ServerSentryCodeLockEventData data = context.Data;
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

    // Resolve only the attachment of the explicitly supplied target. This avoids
    // the GetCodeLock()/GetCodeLockRS() name collision with vanilla DigitalCodeLock.
    static CodeLock AttachedLock(EntityAI target)
    {
        if (!target) return null;
        return CodeLock.Cast(target.FindAttachmentBySlotName("Att_CombinationLock"));
    }

    static ServerSentryCodeLockOperation Start(CodeLock codeLock, PlayerIdentity identity, EntityAI parent, string sourceHook)
    {
        ServerSentryCodeLockEventData data = Capture(codeLock, identity, parent);
        if (!data) return null;
        ServerSentryCodeLockOperation operation = new ServerSentryCodeLockOperation();
        operation.Data = data;
        operation.Lock = codeLock;
        operation.Parent = parent;
        if (!operation.Parent) operation.Parent = codeLock.GetHierarchyParent();
        operation.FinalHealth = data.lockHealth;
        data.sourceHook = sourceHook;
        Fence fence = Fence.Cast(operation.Parent);
        TentBase tent = TentBase.Cast(operation.Parent);
        if (fence) operation.WasOpen = fence.IsOpened();
        else if (tent) operation.WasOpen = tent.GetDoorAnimState();
        return operation;
    }

    static ServerSentryCodeLockOperation StartAction(ActionData actionData, string sourceHook, bool attachment = false, bool tentTarget = false)
    {
        if (!Enabled() || !actionData || !actionData.m_Target || !actionData.m_Player) return null;
        EntityAI parent;
        if (tentTarget) parent = EntityAI.Cast(actionData.m_Target.GetParent());
        else parent = EntityAI.Cast(actionData.m_Target.GetObject());
        if (!parent) return null;
        CodeLock codeLock;
        if (attachment) codeLock = CodeLock.Cast(actionData.m_MainItem);
        else codeLock = AttachedLock(parent);
        return Start(codeLock, actionData.m_Player.GetIdentity(), parent, sourceHook);
    }

    static ServerSentryCodeLockOperation Current()
    {
        return s_Current;
    }

    static ServerSentryCodeLockOperation Bind(ServerSentryCodeLockOperation operation)
    {
        ServerSentryCodeLockOperation previous = s_Current;
        s_Current = operation;
        return previous;
    }

    static void Restore(ServerSentryCodeLockOperation previous)
    {
        s_Current = previous;
    }

    static void ObservePasscode(CodeLock codeLock, bool rejected)
    {
        if (!s_Current || s_Current.Lock != codeLock) return;
        s_Current.PasscodeChecked = true;
        s_Current.PasscodeAccepted = !rejected;
    }

    static void EmitOwnershipChange(ServerSentryCodeLockOperation operation)
    {
        if (!operation || !operation.Lock) return;
        string actorId = operation.Data.actorBohemiaId;
        if (actorId == "") return;
        if (!operation.Data.hasOwner && operation.Lock.GetOwner() == actorId)
            EmitLockEvent(operation, "access", "claimowner", "success", "owner_assigned");
        else if (!operation.Data.actorIsGuest && operation.Lock.IsGuest(actorId))
            EmitLockEvent(operation, "access", "addguest", "success", "guest_added");
    }

    static void EmitOpen(ServerSentryCodeLockOperation operation, string type, string accessType)
    {
        if (!operation) return;
        Fence fence = Fence.Cast(operation.Parent);
        TentBase tent = TentBase.Cast(operation.Parent);
        if (fence)
        {
            if (operation.WasOpen)
                EmitLockEvent(operation, type, accessType, "observed", "fence_already_open");
            else if (fence.IsOpened())
                EmitLockEvent(operation, type, accessType, "success", "fence_opened");
            else
                EmitLockEvent(operation, type, accessType, "failed", "fence_remained_closed");
        }
        else if (tent)
        {
            if (tent.GetDoorAnimState() != operation.WasOpen)
                EmitLockEvent(operation, type, accessType, "success", "tent_entrance_toggled");
            else
                EmitLockEvent(operation, type, accessType, "observed", "tent_entrance_toggle_requested");
        }
        else
            EmitLockEvent(operation, type, accessType, "failed", "unsupported_target");
    }

    static void EmitRemoval(ServerSentryCodeLockOperation operation, string type, string accessType)
    {
        if (!operation) return;
        if (operation.UnlockObserved && operation.RemovalSucceeded)
            EmitLockEvent(operation, type, accessType, "success", "lock_removed");
        else
            EmitLockEvent(operation, type, accessType, "failed", "lock_not_removed");
    }

    static void FinishQuickAccess(ServerSentryCodeLockOperation operation)
    {
        if (!operation) return;
        if (!operation.Data.isLocked)
            EmitLockEvent(operation, "access", "", "observed", "code_setup_interaction");
        else if (!operation.Data.actorIsOwner && !operation.Data.actorIsGuest)
            EmitLockEvent(operation, "access", "", "observed", "passcode_interaction");
        else
            EmitOpen(operation, "access", "");
    }

    static void FinishAttachment(ServerSentryCodeLockOperation operation)
    {
        if (!operation) return;
        if (operation.Lock && operation.Lock.GetHierarchyParent() == operation.Parent)
            EmitLockEvent(operation, "attach", "", "success", "attachment_confirmed");
        else
            EmitLockEvent(operation, "attach", "", "observed", "attachment_requested");
    }

    static void FinishRaid(ServerSentryCodeLockOperation operation)
    {
        if (!operation) return;
        // NewUnlockServer captured the final health and removal result BEFORE
        // the raid handler could delete the object. Do not touch that lock again.
        if (operation.UnlockObserved)
        {
            float finalDamage = Math.Max(0, operation.Data.lockHealth - operation.FinalHealth);
            if (operation.RemovalSucceeded)
                EmitLockEvent(operation, "raid", "", "success", "raid_completed", false, true, finalDamage, operation.FinalHealth);
            else
                EmitLockEvent(operation, "raid", "", "observed", "raid_removal_unconfirmed", false, false, finalDamage, operation.FinalHealth);
            return;
        }
        if (!operation.Lock || operation.Lock.IsSetForDeletion())
        {
            EmitLockEvent(operation, "raid", "", "observed", "raid_lock_deleted_without_result");
            return;
        }
        float health = operation.Lock.GetHealth("", "Health");
        float damage = Math.Max(0, operation.Data.lockHealth - health);
        if (damage > 0)
            EmitLockEvent(operation, "raid", "", "progress", "raid_increment", false, false, damage, health);
        else
            EmitLockEvent(operation, "raid", "", "observed", "raid_health_unchanged", false, false, damage, health);
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

