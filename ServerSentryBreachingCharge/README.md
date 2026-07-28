# ServerSentry BreachingCharge Bridge

Version **1.1.0** — server-side integration for **ServerSentryMod 1.4.8** and **HDSN BreachingCharge**.

## Features

- Charge placement, arming, defusing, and explosion telemetry.
- Target damage and confirmed destruction telemetry.
- Optional pre-explosion player-damage estimates.
- Typed `ServerSentryBreachingChargeEventData` payloads.
- Bounded duplicate suppression for overlapping third-party callbacks.

## Best-practice behavior

- All lifecycle hooks preserve the original HDSN implementation with `super`.
- Target and charge metadata is captured before HDSN may delete an object, but destruction/deletion events are emitted only after the original handler returns normally.
- Player-damage prediction is marked `result = "estimated"`; it is never represented as authoritative game damage.
- No HTTP or persistence logic is duplicated in this bridge.

## Requirements

- DayZ server
- ServerSentryMod 1.4.8
- HDSN BreachingCharge (`HDSN_BreachingCharge` patch)

## Installation

1. Pack the `ServerSentryBreachingCharge` folder as its own PBO.
2. Preserve `$PBOPREFIX$.txt`.
3. Load HDSN BreachingCharge and ServerSentryMod before this bridge.
4. Compile and test against the exact installed HDSN version.

Example order:

```text
@HDSN_BreachingCharge;@ServerSentryMod;@ServerSentryBreachingCharge
```

## Configuration

Edit compile-time switches in:

```text
scripts/3_Game/Logging/ServerSentryBreachingChargeConsts.c
```

| Setting | Default | Description |
|---|---:|---|
| `LOG_PLACE_EVENTS` | `false` | Optional placement event; core deploy logging may already cover it. |
| `LOG_ARM_EVENTS` | `true` | Charge armed. |
| `LOG_DEFUSE_EVENTS` | `true` | Charge defused. |
| `LOG_EXPLODE_EVENTS` | `true` | Charge exploded. |
| `LOG_TARGET_DAMAGE_EVENTS` | `true` | Target health changed. |
| `LOG_TARGET_DESTROYED_EVENTS` | `true` | HDSN destruction handler completed. |
| `LOG_PLAYER_DAMAGE_EVENTS` | `false` | Estimated player damage; potentially noisy. |
| `LOG_CHARGE_DELETE_EVENTS` | `false` | Charge-object deletion. |
| `DEDUP_WINDOW_MS` | `500` | Duplicate suppression window. |

The ServerSentry core `LogBreachingChargeEvents()` setting must also be enabled.

## Emitted actions

`place`, `arm`, `defuse`, `explode`, `target_damage`, `target_destroyed`, `player_damage`, `charge_delete`, and optional `legacy_target_destroyed`.

## Project structure

```text
ServerSentryBreachingCharge/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/
    ├── 3_Game/Logging/
    └── 4_World/hooks/
```

## Verification checklist

- Arm, defuse, and detonate one charge.
- Confirm one destruction event per target.
- Enable player estimates temporarily and verify `result` is `estimated`.
- Confirm event maps remain stable during repeated detonations.
