# ServerSentry LB Groups Bridge

Version **1.7.1** — server-side integration for **ServerSentryMod 1.4.8**, **LBmaster Core**, and **LBmaster Groups**.

LB Killfeed support is intentionally not included. Install `ServerSentryLBKillfeed` separately when required.

## Features

- Group member join/leave telemetry.
- Subgroup-change telemetry.
- Direct hooks plus `SaveGroup` snapshot-diff fallback.
- LB Groups chat capture and external-provider registration.
- Idempotent shop action for chat tags.
- Idempotent shop action for clan-tag colors.

## Snapshot safety

- Member snapshots store a prefixed SteamID/UID correlation key.
- Group data owned by LBmaster is never modified by the telemetry layer.
- A null/unloaded member collection is treated as unknown, not as confirmation that all members left.
- Direct removal and subgroup events are emitted only after the original LBmaster method completes.
- Duplicate state is bounded and handles engine-clock wrap; orphaned group snapshot maps are removed during full group loads.

## Requirements

- DayZ server
- ServerSentryMod 1.4.8
- LBmaster Core
- LBmaster Groups

**LBmaster Killfeed is not required.**

## Installation

```text
@LBmaster_Core;@LBmaster_Groups;@ServerSentryMod;@ServerSentryLBGroups
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`.

## Group actions

| `groupAction` | Meaning |
|---|---|
| `join` | A member was confirmed in the group after an add/diff. |
| `leave` | A previously captured member is no longer present. |
| `subgroup_change` | The effective subgroup changed. |

## Chat capture

When `LB_DISABLE_CHAT` is not defined, the bridge records the processed LB message after `LBChatHandler.OnChatMessage(...)` completes and registers `LBmaster_Groups` as an external ServerSentry chat provider.

## Shop actions

### `shop.player.chatTag.assign`

```json
{
  "chatTagText": "[VIP]",
  "chatTagColor": "#FFD700",
  "removeProduct": false
}
```

The grant validates text/color, creates or reuses the exact `PrefixGroup`, and assigns the SteamID. Because LB Groups resolves the first direct prefix, the grant removes other direct prefix memberships for that player to make the purchased tag deterministic. Removal deletes only the exact purchased text/color membership.

### `shop.player.group.clanTag.color`

```json
{
  "hexColor": "#FF0000",
  "removeProduct": false
}
```

The action resolves the buyer's persistent group and applies the RGB color. Removal is compare-and-remove: it clears the effect only when the current color still matches the product color, preserving later changes.

Both actions use `ServerSentryShopFulfillment.BeginProductOperation(...)` and `Finish(...)`. Repeating a request with the same fulfillment key is safe.

## Project structure

```text
ServerSentryLBGroups/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/
    ├── 4_World/
    │   ├── Actions/
    │   └── hooks/
    └── 5_Mission/hooks/
```

## Verification checklist

- Restart with existing groups and confirm no false join spam.
- Add/remove members through all available LB Groups paths.
- Move a member between subgroups.
- Test chat with direct, global, group, admin, and system channels.
- Retry grants/removals with identical fulfillment keys.
- Change a clan-tag color after purchase and confirm old-product removal preserves the newer value.
