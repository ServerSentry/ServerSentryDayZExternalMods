# ServerSentry LB Groups Bridge

Version **1.8.2** — server-side integration for the **2026-07-29 ServerSentryDayZMod 1.4.8 API**, **LBmaster Core**, **LBmaster Groups**, and the **LB Groups Plotpole module**.

LB Killfeed support is intentionally not included. Install `ServerSentryLBKillfeed` separately when required.

## Features

- Group member join/leave telemetry.
- Subgroup-change telemetry.
- Direct hooks plus `SaveGroup` snapshot-diff fallback.
- LB Groups chat capture and external-provider registration.
- Idempotent shop action for chat tags.
- Idempotent shop action for clan-tag colors.
- Live-map Flagpole actions to raise or lower a territory flag.
- Live-map Flagpole cleanup for all root world items in the vanilla territory radius.
- Live-map context action to delete root world items in a configurable radius.
- Live-map Flagpole details for Advanced Groups ownership.

## Snapshot safety

- Member snapshots store a prefixed SteamID/UID correlation key.
- Group data owned by LBmaster is never modified by the telemetry layer.
- A null/unloaded member collection is treated as unknown, not as confirmation that all members left.
- Direct removal and subgroup events are emitted only after the original LBmaster method completes.
- Duplicate state is bounded and handles engine-clock wrap; orphaned group snapshot maps are removed during full group loads.

## Requirements

- DayZ server
- ServerSentryDayZMod 1.4.8, build `17d9ea8` or newer
- LBmaster Core
- LBmaster Groups
- LB Groups Plotpole addon (`LBmaster_GroupDLCPlotpole`)

**LBmaster Killfeed is not required.**

## Installation

```text
@LBmaster_Core;@LBmaster_Groups;@ServerSentry;@ServerSentryLBGroups
```

Pack this folder separately and preserve `$PBOPREFIX$.txt`. The current core PBO is `@ServerSentry/addons/ServerSentryDayZMod.pbo`. Remove the superseded `ServerSentryMod.pbo` instead of loading both core PBOs together.

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

## Flagpole and map-context actions

The existing live-map Flagpole menu exposes:

- `Raise flag`
- `Lower flag`
- `Delete all items in flagpole radius`

The live-map world context menu also exposes `Delete Items in radius` with a configurable horizontal radius from 1 to 500 meters. The Flagpole action uses DayZ's `GameConstants.REFRESHER_RADIUS`.

The three Flagpole descriptors use the current `base_flagpole` target type and their registered stable action IDs. The map action uses the current `contextMenu` target type and `ServerSentryContextMenuModActionHandler`, so its position comes directly from the administrator's right-click location. ServerSentry now attaches registered actions to compatible Live Map targets; this bridge no longer modifies `ServerSentryLiveBaseDto`.

Radius cleanup uses the DayZ circle query and a deduplicated root-entity set. It deletes unique root `ItemBase`, `Transport`, and ServerSentry-recognized Flagpole entities inside the horizontal map radius, including storage, construction objects, vehicles, their attachments, and all Flagpoles. Deleting a root object also removes the contents and attachments owned by that root. Occupied vehicles are skipped only while a crew member is inside, because immediate vehicle deletion cannot safely preserve that player. Player inventories, other static map structures, animals, and infected are not targeted. Large radii in dense bases can briefly affect server performance.

Flagpole-radius deletion supports every placed Flagpole recognized by ServerSentry's live-map classifier and deletes the selected Flagpole along with any other Flagpoles in range. Raise and lower operations require a vanilla `TerritoryFlag` or a compatible subclass because they use DayZ's native flag animation and refresher methods. When a name-compatible third-party Flagpole does not inherit `TerritoryFlag`, these two state-changing actions fail safely without changing the object.

Both radius cleanup actions reserve and complete a persistent ServerSentry fulfillment receipt. Repeating the same request or fulfillment key does not execute the destructive cleanup again.

## Advanced Groups Flagpole details

The bridge registers the current ServerSentry Live Base Detail provider from `ServerSentryDayZExternalMods`. Clicking an Advanced Groups Flagpole can show:

- Group name
- Group tag
- Group level
- Current and configured maximum members

Unclaimed flags are shown as `Unclaimed`. If the saved owner can no longer be resolved, the persisted group tag and hash remain visible for diagnostics.

## Project structure

```text
ServerSentryLBGroups/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── config.cpp
├── README.md
└── scripts/
    ├── 3_Game/Actions/
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
- Raise and lower an attached Flagpole flag and verify the final animation phase and refresher state.
- Confirm the three Flagpole actions appear only on `base_flagpole` targets and the radius action appears in the map right-click menu.
- Open a claimed Advanced Groups Flagpole and verify group name, tag, level, and member count.
- Run Flagpole-radius cleanup and confirm the selected Flagpole, other Flagpoles, and unoccupied nearby vehicles are deleted while players, their inventories, and occupied vehicles remain.
- Run map-context cleanup with the 1 m and 500 m boundary values and reject values outside that range.
