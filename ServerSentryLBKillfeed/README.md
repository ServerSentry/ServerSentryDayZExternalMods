# ServerSentry LB Killfeed Bridge

Version **1.1.0** — independent shop-action integration for **ServerSentryMod 1.4.8** and **LBmaster Killfeed**.

This package has no dependency on LBmaster Groups.

## Registered action

```text
shop.player.killfeed.color
```

Input:

```json
{
  "killfeedColor": "#00FF00",
  "removeProduct": false
}
```

## Grant behavior

- Validates the website-provided hex color through ServerSentry.
- Keeps exactly one `KillfeedCustomColor` entry for the SteamID.
- Repairs duplicate/null entries for the affected player.
- Refreshes LB Killfeed's in-memory cache.
- Persists the configuration.

## Removal behavior

- Removes only the matching product color.
- Preserves a different color assigned later.
- Treats an already absent color as successful idempotent removal.

All operations use ServerSentry's product-fulfillment state, so retried requests with the same fulfillment key do not apply twice.

## Requirements

- DayZ server
- ServerSentryMod 1.4.8
- LBmaster Core
- LBmaster Killfeed

## Installation

```text
@LBmaster_Core;@LBmaster_Killfeed;@ServerSentryMod;@ServerSentryLBKillfeed
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`.

## Project structure

```text
ServerSentryLBKillfeed/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/4_World/
    ├── Actions/
    └── hooks/
```

## Verification checklist

- Grant a color and verify immediate runtime use.
- Retry the same request and confirm no duplicate entry.
- Apply a newer color, then remove the older product and confirm the newer color remains.
- Remove the active product twice and confirm idempotent success.
