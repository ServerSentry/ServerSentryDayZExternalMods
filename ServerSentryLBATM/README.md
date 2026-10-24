# ServerSentry LB ATM Bridge

Version **1.1.1** — server-side telemetry integration for **ServerSentryMod 1.4.8** and **LBmaster Groups ATM / Enhanced Banking**.

## Design

This bridge observes LBmaster's existing webhook pipeline. It does **not** replace ATM RPC permission checks, teleport logic, spawn/despawn behavior, or any other banking business logic.

`LB_Webhook_Manager_.SendMessage(...)` runs first through `super`; ServerSentry then converts supported webhook arguments into `ServerSentryLBATMEventData`.

## Security and privacy

PIN events are redacted before raw telemetry is constructed:

```text
CreditCardwrongPinEntered
CreditCardPinChanged
```

For those types, `raw` is always `[redacted]` and the entire webhook argument array is treated as opaque. Attribution uses only the separate SteamID parameter, so a future argument-order change cannot place a PIN into a typed field.

Non-sensitive raw webhook telemetry is capped at 4,096 characters per event.

## Requirements

- DayZ server
- ServerSentryMod 1.4.8
- LBmaster Core
- LBmaster Groups ATM (`LBmaster_Groups_ATM` patch)

## Installation

```text
@LBmaster_Core;@LBmaster_Groups_ATM;@ServerSentryMod;@ServerSentryLBATM
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`. See [repository packing instructions](../README.md#packing) for the exact Addon Builder prefix and script-file settings.

## Captured categories

- Personal and group deposits/withdrawals
- Credit-card deposits/withdrawals
- Wrong-PIN and PIN-change security events
- Player transfers and transfer fees
- Group account creation
- Admin account changes, money drops, ATM edits/deletes, and robber reset events
- ATM robbery start/stop/success

All event switches are in:

```text
scripts/3_Game/Logging/ServerSentryLBATMConsts.c
```

Unknown webhook types are disabled by default.

## Intentional limitation

Direct ATM spawn/despawn/teleport actions are not captured unless LBmaster publishes a webhook for them. This is intentional: replacing `LB_RPC_ATM_Server` methods would duplicate security-sensitive third-party logic and could bypass future LBmaster fixes.

## Project structure

```text
ServerSentryLBATM/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/3_Game/
    ├── Logging/
    └── hooks/
```

## Verification checklist

- Deposit/withdraw from personal, group, and card accounts.
- Perform one transfer and verify fee handling.
- Trigger wrong-PIN and PIN-change events, then inspect every raw/log field for secrets.
- Test robbery and admin webhooks.
- Confirm native LB ATM admin behavior is unchanged with the bridge installed.
