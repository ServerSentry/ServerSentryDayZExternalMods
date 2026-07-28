class ServerSentryBreachingChargeDeduper
{
    protected static ref map<string, int> s_LastEmitBySignature;
    protected static int s_LastCleanupAt;

    static bool ShouldEmit(string signature)
    {
        if (signature == "")
        {
            return true;
        }

        if (!s_LastEmitBySignature)
        {
            s_LastEmitBySignature = new map<string, int>();
        }

        int now = 0;
        if (GetGame())
        {
            now = GetGame().GetTime();
        }

        CleanupIfNeeded(now);

        int last;
        if (s_LastEmitBySignature.Find(signature, last))
        {
            // GetTime() can wrap on very long-running servers. A wrapped clock
            // invalidates the previous timestamp instead of suppressing events.
            if (now >= last && (now - last) < ServerSentryBreachingChargeConsts.DEDUP_WINDOW_MS)
            {
                return false;
            }
        }

        s_LastEmitBySignature.Set(signature, now);
        return true;
    }

    protected static void CleanupIfNeeded(int now)
    {
        if (!s_LastEmitBySignature)
        {
            return;
        }

        // Prefer a short dedupe reset over unbounded memory growth during an
        // unexpected event storm. Normal operation uses the timed cleanup.
        if (s_LastEmitBySignature.Count() > (ServerSentryBreachingChargeConsts.DEDUP_MAX_ENTRIES * 2))
        {
            s_LastEmitBySignature.Clear();
            s_LastCleanupAt = now;
            return;
        }

        if (s_LastEmitBySignature.Count() < ServerSentryBreachingChargeConsts.DEDUP_MAX_ENTRIES)
        {
            if (now >= s_LastCleanupAt && (now - s_LastCleanupAt) < ServerSentryBreachingChargeConsts.DEDUP_CLEANUP_INTERVAL_MS)
            {
                return;
            }
        }

        array<string> expired = new array<string>();
        foreach (string signature, int timestamp : s_LastEmitBySignature)
        {
            if (now < timestamp || (now - timestamp) > ServerSentryBreachingChargeConsts.DEDUP_RETENTION_MS)
            {
                expired.Insert(signature);
            }
        }

        foreach (string key : expired)
        {
            s_LastEmitBySignature.Remove(key);
        }

        s_LastCleanupAt = now;
    }
}
