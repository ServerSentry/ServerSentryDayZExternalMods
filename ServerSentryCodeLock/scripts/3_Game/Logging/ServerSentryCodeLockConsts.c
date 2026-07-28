class ServerSentryCodeLockConsts
{
    const static bool ENABLED = true;

    // Master switch: if false, no CodeLock bridge events are emitted.
    const static bool LOG_CODELOCK_EVENTS = true;

    // Original CodeLockLogger categories.
    const static bool LOG_ACCESS_EVENTS = true;
    const static bool LOG_ATTACH_EVENTS = true;
    const static bool LOG_RAID_EVENTS = true;
    const static bool LOG_SYNC_EVENTS = false;
    const static bool LOG_ADMIN_EVENTS = true;

    // Access subtypes.
    const static bool LOG_SET_CODE = true;
    const static bool LOG_CHANGE_CODE = true;
    const static bool LOG_NOT_OWNER_DENIED = true;
    const static bool LOG_ENTER_CODE = true;
    const static bool LOG_QUICK_ACCESS = false;
    const static bool LOG_CLAIM_OWNER = true;
    const static bool LOG_ADD_GUEST = true;
    const static bool LOG_REMOVE_LOCK = true;

    // Additional check via CodeLock.IsLocked while RPC context is active.
    // Passcodes are never written; only the failed result is logged.
    const static bool LOG_FAILED_PASSCODE = true;

    // Raid subtypes.
    const static bool LOG_RAID_PROGRESS = true;
    const static bool LOG_RAID_SUCCESS = true;

    // Admin subtypes.
    const static bool LOG_ADMIN_OPEN = true;
    const static bool LOG_ADMIN_CHANGE = true;
    const static bool LOG_ADMIN_REMOVE = true;
    const static bool LOG_ADMIN_JOIN = true;
    const static bool LOG_ADMIN_OTHER = true;

    // Optional lower-level CodeLock method hooks. These can be useful for debugging
    // or for edge cases not covered by CodeLockLogger, but they may create extra
    // events beside the normal semantic event. Defaults stay false to avoid spam.
    const static bool LOG_INTERNAL_LOCK_SERVER = false;
    const static bool LOG_INTERNAL_UNLOCK_SERVER = false;
    const static bool LOG_INTERNAL_OWNER_SET = false;
    const static bool LOG_INTERNAL_GUEST_ADD = false;
    const static bool LOG_INTERNAL_GUEST_CLEAR = false;

    // Dedupe identical CodeLock bridge events in a tiny window.
    const static int DEDUP_WINDOW_MS = 500;
    const static int DEDUP_RETENTION_MS = 60000;
    const static int DEDUP_CLEANUP_INTERVAL_MS = 30000;
    const static int DEDUP_MAX_ENTRIES = 2048;
}
