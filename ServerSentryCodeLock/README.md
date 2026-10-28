# ServerSentry CodeLock Bridge

Version **1.2.0** — server-side integration for the RoomService/Wardog **CodeLock** mod and the current **ServerSentryMod** core.

## Compatibility

This extension uses existing CodeLock RPC, action and entity methods. It does not require a modified CodeLock distribution or an additional logger API. In particular, it does not reference `CodeLockLogContext`, `CaptureLockEvent` or `WriteLockEvent`; this fixes the `Unknown type CodeLockLogContext` error with the regular mod. It also works with the previously patched local logger because ordinary logger calls do not produce duplicate semantic telemetry.

Vanilla **DigitalCodeLock** is already logged by the main mod through `CodeLockComponent`, `ActionInteractCodeLockItem` and `ActionResetCodeOnCodeLock`. No extension is required for vanilla-only servers. Install this bridge only alongside the third-party CodeLock mod. The bridge never hooks vanilla digital-lock classes or calls the vanilla `GetCodeLock()` getter.

## Installation

1. Load the current ServerSentry core and the CodeLock mod on the server.
2. Build this extension and deploy the resulting PBO on the server.
3. Restart and test the interactions below. The CodeLock clients continue to use the matching normal CodeLock build.

```text
@CodeLock;@ServerSentryMod;@ServerSentryCodeLock
```

Preserve `$PBOPREFIX$.txt`. See the [repository packing instructions](../README.md#packing) for Addon Builder prefix and script-file settings. **Only this extension needs rebuilding for this compatibility fix.** There is no required change to the main mod or the CodeLock mod.

## Exact target and outcome logging

- Each operation captures its actual lock, actor and attachment parent before calling the original method, then emits its observed outcome afterwards.
- Fence and tent actions use their exact action target; attachment actions use the CodeLock in `m_MainItem`. Tent proxy actions use `GetParent()`.
- RPC method wrappers receive the target already decoded by CodeLock. The RPC byte stream is passed unchanged to the original handler. Its envelope supplies only the sender identity.
- Attachment resolution casts the item in the explicit target's `Att_CombinationLock` slot to `CodeLock`. It does not depend on the conflicting `GetCodeLock()` / `GetCodeLockRS()` getter names.
- There is no proximity search, coordinate matching or guessed RPC target.
- Snapshots preserve the original object IDs, positions, ownership and parent after removal. The event `extra` field identifies the snapshot as `before_action`.
- Raid completion reads health and removal results in `NewUnlockServer`, before the raid handler can delete the lock. The completion hook uses those captured values.
- Setting a code can be denied before the `SetCode` method runs. For that path, the exact ownership-check object is retained briefly and used only when the original logger confirms the rejection for the same RPC sender. Ambiguous candidates are discarded.

The wrappers call their corresponding `super` methods once with unchanged arguments. They do not reproduce gameplay, permission checks or inventory moves. Modded classes can override existing private methods, as documented in [Bohemia's Enforce Script reference](https://community.bistudio.com/wiki/DayZ:Enforce_Script_Syntax#Modded_private_members).

### Outcomes

- `success`: the relevant server state or operation was confirmed.
- `failed`: the expected state was not reached.
- `denied`: the original code rejected a PIN, ownership check or permission.
- `progress`: a raid reduced the lock's health without completing removal.
- `observed`: an interaction/request was seen but its completion is not confirmed, including predictive attachment and delayed tent animation.

An open fence state does not certify completion of its visual animation. Opening a code-change menu is recorded as a request, not a completed code change. Ownership/guest events describe actual changes; existing guests are not repeatedly reported as newly added.

## Configuration and privacy

Edit `scripts/3_Game/Logging/ServerSentryCodeLockConsts.c` to select event categories. Quick access is enabled. Optional internal diagnostics remain disabled by default; their bounded deduper does not affect actual attempts. Every semantic attempt is retained, including rapid incorrect PIN submissions.

The bridge honors the core server/telemetry gate. Synchronization (`synch`/`sync`, case-insensitive) is always discarded before the original file logger and ServerSentry emission. PINs are passed through to original methods and compared only transiently; they are never stored in a context, event or log.

The legacy file logger continues to operate for non-sync entries. Its position-only records are not used to infer lock targets. Admin joins remain actor-only events without a lock identifier.

## Validation

Run the extension contract checks from the repository root:

```text
python -m unittest discover -s tests -p "test_codelock_bridge_contract.py" -v
```

These checks catch accidental dependencies on the former custom API, unsupported hook signatures, RPC-stream consumption and changes to the original-call contract. They do not replace a DayZ script compile or an in-game test.

After rebuilding:

- Start with the regular CodeLock mod; confirm there is no unknown-type or override-signature error.
- Open a fence/tent as owner, guest and with a valid PIN; submit invalid PINs rapidly and verify each attempt.
- Set/change a code, claim an unowned lock and attempt owner-only operations without ownership.
- Use two adjacent locks and verify exact lock and original parent IDs in every event.
- Test both fence and tent proxy targets, attachment, admin open/change/remove and normal removal.
- Raid with deletion enabled and disabled; verify original metadata and measured health/damage without duplicate success events.
- Use a vanilla DigitalCodeLock alongside the modded lock; vanilla events should come only from the main mod.
- Trigger repeated synchronization and disable core telemetry; verify the respective events remain suppressed.
