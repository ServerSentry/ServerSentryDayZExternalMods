// Wrap existing methods, including private members supported by modded classes.
// Never consume the RPC stream or reproduce CodeLock's gameplay/permission logic.
modded class CodeLockServerRPC
{
    override void ServerRPCHandler(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        ServerSentryCodeLockRpcContext.Begin(sender, rpc_type);
        super.ServerRPCHandler(sender, target, rpc_type, ctx);
        ServerSentryCodeLockRpcContext.End();
    }

    protected PlayerIdentity ServerSentryActor()
    {
        PlayerIdentity sender = ServerSentryCodeLockRpcContext.GetSender();
        if (sender) return sender;
        if (player) return player.GetIdentity();
        return null;
    }

    override private void SetCode(ItemBase itembase, string passcode)
    {
        CodeLock codeLock = ServerSentryCodeLockBridge.AttachedLock(itembase);
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Start(codeLock, ServerSentryActor(), itembase, "CodeLockServerRPC.SetCode");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.SetCode(itembase, passcode);
        ServerSentryCodeLockBridge.Restore(previous);

        if (!operation) return;
        string accessType = "changecode";
        if (!operation.Data.hasOwner) accessType = "setcode";
        // Compare transiently; neither the supplied nor stored PIN enters the event.
        if (codeLock && codeLock.GetLockState() && codeLock.GetPasscode() == passcode)
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", accessType, "success", "passcode_set");
        else
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", accessType, "failed", "passcode_not_set");
        ServerSentryCodeLockBridge.EmitOwnershipChange(operation);
    }

    override private void EnterCode(ItemBase target, string passcode, string widget)
    {
        CodeLock codeLock = ServerSentryCodeLockBridge.AttachedLock(target);
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Start(codeLock, ServerSentryActor(), target, "CodeLockServerRPC.EnterCode");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.EnterCode(target, passcode, widget);
        ServerSentryCodeLockBridge.Restore(previous);

        if (!operation) return;
        if (operation.Reply == "passcode" || (operation.PasscodeChecked && !operation.PasscodeAccepted))
        {
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "passcode_failed", "denied", "incorrect_passcode");
            return;
        }
        if (!operation.PasscodeChecked)
        {
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "", "observed", "passcode_check_not_observed");
            return;
        }

        switch (widget)
        {
            case "btnEnterPin":
                operation.Data.widgetName = "btnEnterPin";
                ServerSentryCodeLockBridge.EmitOwnershipChange(operation);
                ServerSentryCodeLockBridge.EmitOpen(operation, "access", "entercode");
                break;
            case "btnRemove":
                operation.Data.widgetName = "btnRemove";
                if (!operation.Data.actorIsOwner)
                    ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "notowner", "denied", "remove_requires_owner");
                else
                    ServerSentryCodeLockBridge.EmitRemoval(operation, "access", "removelock");
                break;
            case "btnChangePin":
                operation.Data.widgetName = "btnChangePin";
                if (!operation.Data.actorIsOwner)
                    ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "notowner", "denied", "change_requires_owner");
                else if (operation.MenuRequested)
                    ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "changecode", "observed", "change_menu_requested");
                break;
            default:
                ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "", "observed", "unsupported_widget");
                break;
        }
    }

    override protected void AdminMenu(ItemBase target, string widgetName)
    {
        CodeLock codeLock = ServerSentryCodeLockBridge.AttachedLock(target);
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Start(codeLock, ServerSentryActor(), target, "CodeLockServerRPC.AdminMenu");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.AdminMenu(target, widgetName);
        ServerSentryCodeLockBridge.Restore(previous);

        if (!operation) return;
        string accessType;
        if (widgetName == "btnOpen") accessType = "open";
        else if (widgetName == "btnChange") accessType = "change";
        else if (widgetName == "btnRemove") accessType = "remove";
        else
        {
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "admin", "", "observed", "unsupported_widget");
            return;
        }
        operation.Data.widgetName = widgetName;
        if (operation.Reply == "admin")
        {
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "admin", accessType, "denied", "permission_denied");
            return;
        }
        if (accessType == "open" && operation.Reply == "exit")
            ServerSentryCodeLockBridge.EmitOpen(operation, "admin", accessType);
        else if (accessType == "remove")
            ServerSentryCodeLockBridge.EmitRemoval(operation, "admin", accessType);
        else if (accessType == "change" && operation.MenuRequested)
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "admin", accessType, "observed", "change_menu_requested");
        else
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "admin", accessType, "observed", "action_not_observed");
    }

    override private void ClaimCode(CodeLock codelockRS, string passcode)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Start(codelockRS, ServerSentryActor(), null, "CodeLockServerRPC.ClaimCode");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.ClaimCode(codelockRS, passcode);
        ServerSentryCodeLockBridge.Restore(previous);

        if (!operation) return;
        if (operation.Reply == "owner")
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "claimowner", "denied", "owner_already_assigned");
        else if (operation.Reply == "passcode" || (operation.PasscodeChecked && !operation.PasscodeAccepted))
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "passcode_failed", "denied", "incorrect_passcode");
        else if (!operation.Data.hasOwner && codelockRS && codelockRS.GetOwner() == operation.Data.actorBohemiaId)
            ServerSentryCodeLockBridge.EmitOwnershipChange(operation);
        else
            ServerSentryCodeLockBridge.EmitLockEvent(operation, "access", "claimowner", "observed", "claim_not_confirmed");
    }

    override private void SendMenuError(string errorType)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Current();
        if (operation)
        {
            if (errorType == "exit" || errorType == "passcode" || errorType == "owner" || errorType == "admin")
                operation.Reply = errorType;
        }
        super.SendMenuError(errorType);
    }

    override private void SendShowSetCodeRPC(ItemBase target)
    {
        super.SendShowSetCodeRPC(target);
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Current();
        if (operation && operation.Parent == target) operation.MenuRequested = true;
    }
}

modded class CodeLockLogger
{
    override void WriteLog(string type, PlayerIdentity playerId = null, vector pos = "0 0 0", string accessType = "", bool claim = false, bool cut = false, float damage = 0, float health = 0)
    {
        if (ServerSentryCodeLockBridge.IsSyncLogType(type)) return;
        super.WriteLog(type, playerId, pos, accessType, claim, cut, damage, health);
        ServerSentryCodeLockBridge.EmitAdminJoin(type, playerId, accessType);

        // The legacy position is never used for target attribution. Normal events
        // are emitted by the surrounding operation, avoiding duplicates on both
        // stock CodeLock and locally patched versions of its logger.
        string normalizedType = type;
        string normalizedAccess = accessType;
        normalizedType.ToLower();
        normalizedAccess.ToLower();
        if (normalizedType == "access" && normalizedAccess == "notowner" && !ServerSentryCodeLockBridge.Current())
            ServerSentryCodeLockRpcContext.EmitCodeChangeDenied(playerId);
    }
}
