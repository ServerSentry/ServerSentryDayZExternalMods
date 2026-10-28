// Server-side wrappers preserve the original actions and their inventory junctures.
modded class ActionInteractLockOnFence
{
    override void OnStartServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionInteractLockOnFence.OnStartServer");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnStartServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishQuickAccess(operation);
    }
}

modded class ActionInteractLockOnTent
{
    override void OnStartServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionInteractLockOnTent.OnStartServer", false, true);
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnStartServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishQuickAccess(operation);
    }
}

modded class ActionAttachCodeLockToFence
{
    override void OnExecuteServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionAttachCodeLockToFence.OnExecuteServer", true);
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnExecuteServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishAttachment(operation);
    }
}

modded class ActionAttachCodeLockToTent
{
    override void OnExecuteServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionAttachCodeLockToTent.OnExecuteServer", true, true);
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnExecuteServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishAttachment(operation);
    }
}

modded class ActionDestroyCodeLockOnFence
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionDestroyCodeLockOnFence.OnFinishProgressServer");
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnFinishProgressServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishRaid(operation);
    }
}

modded class ActionDestroyCodeLockOnTent
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.StartAction(action_data, "ActionDestroyCodeLockOnTent.OnFinishProgressServer", false, true);
        ServerSentryCodeLockOperation previous = ServerSentryCodeLockBridge.Bind(operation);
        super.OnFinishProgressServer(action_data);
        ServerSentryCodeLockBridge.Restore(previous);
        ServerSentryCodeLockBridge.FinishRaid(operation);
    }
}
