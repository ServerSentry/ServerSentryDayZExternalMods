# ServerSentry CodeLock Bridge

Version **1.1.0** — server-side integration for the current **ServerSentryMod** core and the updated **CodeLock** logging API.

## Features

- Access, attachment, raid, and admin telemetry.
- Failed-passcode result detection without storing the passcode.
- Exact lock, original attachment parent, and actor snapshots supplied by the operation itself.
- Explicit outcomes recorded after the operation, including requests whose completion is not yet confirmed.
- Optional low-level lock/owner/guest hooks for diagnostics.
- Every semantic event is retained; optional internal diagnostics use bounded duplicate suppression.

## Security

The bridge never copies `ui_Passcode` into a ServerSentry model, raw field, or log message. Failed attempts record only semantic information such as actor, target, and `passcode_failed`.

No target is inferred from coordinates, nearby objects, or the RPC envelope target. Position-only legacy calls cannot produce lock telemetry. The legacy admin-join event remains actor-only and has no associated lock.

## Requirements

- DayZ server
- The current local ServerSentryMod core, providing `ServerSentryCodeLockEventData`, `ServerSentryApi.EmitCodeLockEvent`, and `IsAllowedToStartTelemetryLogging`
- Updated CodeLock with `CodeLockLogContext`, `CodeLockLogger.CaptureLockEvent`, and `CodeLockLogger.WriteLockEvent` (`CodeLock` patch; adjust `requiredAddons[]` if your build uses another patch name)

The updated CodeLock and bridge must be rebuilt and deployed together. This integration requires no changes to the core mod. CodeLock's gameplay attachment access continues to use the DayZ 1.30-compatible `GetCodeLockRS()` API; the bridge no longer performs attachment lookups to attribute logs.

## Installation

```text
@CodeLock;@ServerSentryMod;@ServerSentryCodeLock
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`. Deploy the matching CodeLock build to the server and clients, and the rebuilt bridge to the server. An older CodeLock build does not provide the new logging methods required by this bridge.

## Configuration

Edit:

```text
scripts/3_Game/Logging/ServerSentryCodeLockConsts.c
```

Main switches include access, attach, raid, admin, failed-passcode, and optional internal-hook logging. Internal hooks are disabled by default; when explicitly enabled, they record separate diagnostic details alongside semantic events.

Synchronization records (`synch`/`sync`, case-insensitive) are always discarded before the original CodeLock file logger and ServerSentry emission. There is no switch to re-enable them. The bridge also honors the current core telemetry gate (`IsAllowedToStartTelemetryLogging`).

Results describe the observed outcome:

- `success`: the relevant operation or state change was confirmed.
- `failed`: the operation did not produce its expected state.
- `denied`: an ownership, permission, or passcode check rejected the request.
- `progress`: a raid reduced the lock's health without completing removal.
- `observed`: the request was observed, but completion is not yet confirmed. Predictive attachment, tent animation, and deferred raid deletion requests can have this result. A raid tick that does not reduce health is also recorded as an observation.

Success for a fence means the server's open state was confirmed; it does not certify that the visual animation has finished. Opening an admin code-change menu is an observed request; the subsequent code-setting operation produces the actual change event.

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
admin_open
admin_change
admin_remove
admin_join
```

## Exact event capture

Each operation supplies its actual CodeLock instance to `CodeLockLogger.CaptureLockEvent` before changing it. The bridge captures primitive actor, lock, and original parent metadata at that point. An attachment request also supplies its intended fence or tent explicitly. `CodeLockLogger.WriteLockEvent` emits the captured event after the operation with an explicit result and reason.

The captured identifiers, positions, ownership, and parent metadata belong to the state before the operation. This is labeled in the event's `extra` field. They remain available if the lock is moved, detached, or deleted during removal or a successful raid. Raid damage and remaining health are measured separately before deletion and included in the result. If detachment was not confirmed but deletion was requested, the raid is recorded as `observed/raid_lock_deletion_requested`; the bridge does not inspect a deleted object to infer completion.

There is no proximity search, RPC-target guessing, or attribution from a legacy log position. The legacy `WriteLog` telemetry hook only handles `admin/join`, which is an actor-only event. Semantic events do not use timed deduplication, so repeated failed passcode attempts and other distinct operations are retained. Bounded deduplication applies only to the optional internal diagnostic hooks.

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

- Set/change a code and perform owner, guest, and valid/invalid passcode access.
- Claim an unowned lock; repeat access as an existing guest and confirm no extra guest-add event is fabricated.
- Confirm no passcode is present in payloads or server logs.
- Place two locks next to each other and verify that each operation reports the exact lock identifier and original parent.
- Remove a lock normally and through the admin menu; verify that the event retains its original fence/tent identity after detachment.
- Test raid increments and successful raids with deletion enabled and disabled; verify actual damage, remaining health, and the original lock/parent snapshot.
- Test attachment and tent animation requests; unconfirmed completion must be `observed`, not a fabricated success or failure.
- Repeat wrong-code attempts quickly and confirm each semantic attempt is retained with the correct actor and lock.
- Test admin open/change-menu/remove paths separately and verify their results match the performed operation.
- Trigger repeated owner/guest synchronization: no sync file records or ServerSentry sync events should be produced.
- Disable core telemetry and confirm the bridge emits no ServerSentry events.
- Verify an admin join produces only actor information, with no lock inferred from any location.
