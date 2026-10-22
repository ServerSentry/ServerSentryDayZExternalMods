# Changelog

## Unreleased

- Use `GetCodeLockRS()` for the DayZ 1.30-compatible CodeLock attachment lookup.
- Permanently suppress synchronization records before the original file logger and ServerSentry emission; remove the sync logging switch.
- Honor the current core server/telemetry gate before capturing events.
- Document limitations of legacy target attribution and outcome timing.

## 1.1.0

- Resolves the exact RPC target before proximity fallback.
- Adds stale RPC-context expiration and defensive reset behavior.
- Makes passcode-check context validation consistent.
- Uses semantic results for denied access, raid progress, and observed sync/unknown events.
- Adds timed cleanup and a hard cap to duplicate-suppression state.
- Adds consistent PBO metadata and English documentation.
