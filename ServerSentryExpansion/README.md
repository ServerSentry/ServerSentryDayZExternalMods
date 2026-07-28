# ServerSentry Expansion Bridge

Version **1.1.0** — server-side integration for **ServerSentryMod 1.4.8** and selected **DayZ Expansion** modules.

## Features

- Safe-zone enter/leave telemetry.
- ServerSentry live-map safe-zone markers.
- Expansion Market buy/sell observations.
- Optional Expansion Chat capture when `EXPANSIONMODCHAT` is available.

## Requirements

- DayZ server
- Community Framework / `JM_CF_Scripts`
- ServerSentryMod 1.4.8
- Expansion Core Scripts
- Expansion Market Scripts
- Expansion Chat Scripts only when the chat feature is compiled

## Installation

```text
@CF;@DayZ-Expansion-Core;@DayZ-Expansion-Market;@ServerSentryMod;@ServerSentryExpansion
```

Use the actual folder names from your server. Pack this bridge separately and preserve `$PBOPREFIX$.txt`.

## Market result semantics

The bridge opens a short-lived capture context around Expansion's purchase/sell execution and emits **at most one** record.

| Result | Meaning |
|---|---|
| `success` | A stock delta or explicit success signal was observed. |
| `failed` | Expansion emitted a failure signal. |
| `observed` | The operation returned normally, but the available internal hook exposed no authoritative success signal. |

Raw Expansion Market messages are bounded to 4,096 characters. The context expires after 30 seconds and is reset before every transaction.

> Expansion Market currently requires version-sensitive internal method overrides. Compile this bridge against the exact Expansion build deployed on the server after every Expansion update.

## Chat behavior

Expansion processes and distributes the message first. ServerSentry then records the normalized message and registers `DayZExpansion_Chat` as an external provider to avoid duplicate vanilla capture.

## Safe-zone behavior

The bridge resolves circle, cylinder, and polygon zones from public safe-zone collections and publishes live markers. ServerSentry 1.4.8 contains legacy boolean fields for PvP/kill-on-entry without an unknown state; this bridge deliberately does not guess values from unstable private Expansion members. Treat those two default-false fields as non-authoritative for this integration.

## Project structure

```text
ServerSentryExpansion/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/
    ├── 4_World/
    │   ├── Entities/ManBase/
    │   └── hooks/
    └── 5_Mission/hooks/
```

## Verification checklist

- Enter and leave circle, cylinder, and polygon zones.
- Confirm marker refresh/clear behavior when live logging is disabled.
- Buy and sell finite-stock and unlimited-stock items.
- Test failed purchases and confirm only one event is emitted.
- Test all enabled Expansion Chat channels.
