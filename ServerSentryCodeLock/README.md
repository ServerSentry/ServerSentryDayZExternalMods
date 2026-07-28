# ServerSentry CodeLock Bridge

Version **1.1.0** — server-side integration for **ServerSentryMod 1.4.8** and **CodeLock**.

## Features

- Access, attachment, raid, synchronization, and admin telemetry.
- Failed-passcode result detection without storing the passcode.
- Exact RPC-target correlation with proximity fallback.
- Optional low-level lock/owner/guest hooks for diagnostics.
- Bounded duplicate suppression.

## Security

The bridge never copies `ui_Passcode` into a ServerSentry model, raw field, or log message. Failed attempts record only semantic information such as actor, target, and `passcode_failed`.

RPC correlation is short-lived, cleared before every request, and expires after ten seconds to prevent stale actor/target attribution.

## Requirements

- DayZ server
- ServerSentryMod 1.4.8
- CodeLock (`CodeLock` patch; adjust `requiredAddons[]` if your build uses another patch name)

## Installation

```text
@CodeLock;@ServerSentryMod;@ServerSentryCodeLock
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`.

## Configuration

Edit:

```text
scripts/3_Game/Logging/ServerSentryCodeLockConsts.c
```

Main switches include access, attach, raid, sync, admin, failed-passcode, and optional internal-hook logging. Internal hooks are disabled by default to avoid duplicate/noisy records.

Result semantics use `denied` for ownership denial, `progress` for incomplete raids, and `observed` for sync/unknown logger records.

Common actions:

```text
set_code
change_code
not_owner_denied
enter_code
quick_access
claim_owner
add_guest
remove_lock
passcode_failed
attach
raid_progress
raid_success
sync_request
admin_open
admin_change
admin_remove
admin_join
```

## Target resolution

1. Use the exact object supplied to `CodeLockServerRPC`.
2. Resolve a lock attached to that target when possible.
3. Only then fall back to a two-metre proximity search for legacy logger calls.

This avoids attributing an action to a neighboring lock in dense bases.

## Project structure

```text
ServerSentryCodeLock/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/
    ├── 3_Game/Logging/
    └── 4_World/hooks/
```

## Verification checklist

- Set/change a code and perform valid/invalid access.
- Confirm no passcode is present in payloads or server logs.
- Test two nearby CodeLocks and confirm the correct target is selected.
- Test raid and admin paths separately.
