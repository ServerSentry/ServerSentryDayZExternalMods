# Changelog

## Unreleased

- Replace all proximity searches, RPC-target guessing, and position-based legacy attribution with exact lock/parent/actor snapshots supplied through the updated CodeLock logging API.
- Capture primitive metadata with `CaptureLockEvent` before mutation and emit explicit outcomes through `WriteLockEvent` afterwards; preserve the original lock and parent across detachment and deletion.
- Label snapshot metadata as the state before the operation, and record raid damage and remaining health separately before lock deletion.
- Record unchanged raid health and unconfirmed deferred lock deletion as `observed`, without reading the lock after deletion.
- Record unconfirmed predictive attachments, tent animation requests, and admin code-change menu requests as `observed`; emit ownership/guest events only for actual changes.
- Restrict legacy `WriteLog` telemetry to the actor-only admin-join event.
- Retain all semantic events, including repeated failed attempts, without timed deduplication. Keep bounded deduplication only for optional internal diagnostics, which remain disabled by default.
- Permanently suppress synchronization records before the original file logger and ServerSentry emission; remove the sync logging switch.
- Honor the current core server/telemetry gate before capturing events.
- Require the updated CodeLock and bridge to be rebuilt and deployed together; use the current core API without changes to the core mod.
- Remove the bridge attachment lookup entirely, superseding the earlier `GetCodeLockRS()` lookup adjustment. CodeLock gameplay continues to use `GetCodeLockRS()`.
- Expand verification to adjacent locks, owner/guest claims, repeated failed attempts, removal, raids with deletion, and deferred attachment/animation outcomes. Never include PINs in event data.

## 1.1.0

- Resolves the exact RPC target before proximity fallback.
- Adds stale RPC-context expiration and defensive reset behavior.
- Makes passcode-check context validation consistent.
- Uses semantic results for denied access, raid progress, and observed sync/unknown events.
- Adds timed cleanup and a hard cap to duplicate-suppression state.
- Adds consistent PBO metadata and English documentation.
