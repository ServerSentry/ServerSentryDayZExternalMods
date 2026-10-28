modded class CodeLock
{
    override bool IsLocked(string ui_Passcode)
    {
        bool rejected = super.IsLocked(ui_Passcode);
        ServerSentryCodeLockBridge.ObservePasscode(this, rejected);
        return rejected;
    }

    override bool IsOwner(string id)
    {
        bool isOwner = super.IsOwner(id);
        ServerSentryCodeLockRpcContext.ObserveOwnerCheck(this, id, isOwner);
        return isOwner;
    }

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
        ServerSentryCodeLockOperation operation = ServerSentryCodeLockBridge.Current();
        if (operation && operation.Lock == this) operation.FinalHealth = GetHealth("", "Health");

        super.NewUnlockServer(player, parent);

        // Raid handlers may delete this lock immediately after this method returns.
        // Store only the observed result; their completion hooks use these values.
        bool removed = !GetLockState() && GetHierarchyParent() != parent;
        if (operation && operation.Lock == this)
        {
            operation.UnlockObserved = true;
            operation.RemovalSucceeded = removed;
        }
        if (!data) return;
        string result = "failed";
        if (removed) result = "success";
        ServerSentryCodeLockBridge.EmitInternal(data, "unlock_server", "CodeLock.NewUnlockServer", result, "internal_unlock_server");
    }

    override void ServerSetOwner(string id)
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_OWNER_SET) data = ServerSentryCodeLockBridge.Capture(this, null);
        super.ServerSetOwner(id);
        if (data && data.ownerBohemiaId != GetOwner())
            ServerSentryCodeLockBridge.EmitInternal(data, "owner_set", "CodeLock.ServerSetOwner", "success", "owner_changed");
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
            ServerSentryCodeLockBridge.EmitInternal(data, "guest_add", "CodeLock.ServerAddGuest", "success", "guest_added");
    }

    override void ServerClearGuests()
    {
        ServerSentryCodeLockEventData data;
        if (ServerSentryCodeLockConsts.LOG_INTERNAL_GUEST_CLEAR) data = ServerSentryCodeLockBridge.Capture(this, null);
        super.ServerClearGuests();
        array<string> guests = GetGuests();
        if (data && data.guestCount > 0 && guests && guests.Count() == 0)
            ServerSentryCodeLockBridge.EmitInternal(data, "guest_clear", "CodeLock.ServerClearGuests", "success", "guests_cleared");
    }
}
