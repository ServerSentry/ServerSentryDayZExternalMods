class ServerSentryBreachingChargeConsts
{
    const static bool ENABLED = true;

    // Master switch: if false, no HDSN BreachingCharge bridge events are emitted.
    const static bool LOG_BREACHING_CHARGE_EVENTS = true;

    // Semantic charge lifecycle events.
    // Place is default false because ServerSentry core deploy/place logging can already cover the deployment action.
    const static bool LOG_PLACE_EVENTS = false;
    const static bool LOG_ARM_EVENTS = true;
    const static bool LOG_DEFUSE_EVENTS = true;
    const static bool LOG_EXPLODE_EVENTS = true;

    // Object damage and destruction caused by a charge explosion.
    const static bool LOG_TARGET_DAMAGE_EVENTS = true;
    const static bool LOG_TARGET_DESTROYED_EVENTS = true;

    // Optional, noisy event groups. Keep false unless you explicitly need them.
    const static bool LOG_PLAYER_DAMAGE_EVENTS = false;
    const static bool LOG_CHARGE_DELETE_EVENTS = false;
    const static bool LOG_LEGACY_ON_TARGET_BEING_DESTROYED = false;

    // Dedupe identical bridge events in a tiny window.
    const static int DEDUP_WINDOW_MS = 500;
    const static int DEDUP_RETENTION_MS = 60000;
    const static int DEDUP_CLEANUP_INTERVAL_MS = 30000;
    const static int DEDUP_MAX_ENTRIES = 2048;
}
