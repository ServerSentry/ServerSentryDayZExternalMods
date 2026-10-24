# ServerSentry BBP Bridge

Optional DayZ server integration between **ServerSentryMod 1.4.8** and **BaseBuildingPlus (BBP)**.

The bridge restores typed ServerSentry telemetry for BBP construction actions that may bypass or replace vanilla action implementations. It contains no BaseBuildingPlus assets or source files and must be packaged independently.

## Features

- BBP part build telemetry
- Separate BBP admin-tool build telemetry
- BBP dismantle telemetry
- BBP destruction telemetry
- BBP fold telemetry
- Optional BBP placement telemetry
- Server-only execution
- Shared ServerSentry lifecycle deduplication
- Preservation of the full `modded class`/`super` chain
- Compatibility with ServerSentry's existing category switches

## Requirements

- DayZ Server
- ServerSentryMod **1.4.8**
- BaseBuildingPlus
- All dependencies required by those mods

The package declares hard PBO dependencies on:

```text
ServerSentryScripts
BaseBuildingPlus_Scripts
```

## Installation

1. Pack the `ServerSentryBBP` folder as its own PBO.
2. Set the PBO prefix to `ServerSentryDayZExternalMods\ServerSentryBBP` (see the [repository packing instructions](../README.md#packing)).
3. Install the bridge as a separate server/client mod package.
4. Load ServerSentryMod and BaseBuildingPlus before this bridge.
5. Do not copy or repack BaseBuildingPlus files into this package.

Example logical load order:

```text
@CF;@BaseBuildingPlus;@ServerSentryMod;@ServerSentryBBP
```

Use the dependency order required by the exact BBP release installed on the server.

## Configuration

Feature switches are defined in:

```text
scripts/3_Game/Logging/ServerSentryBBPConsts.c
```

Default values:

| Constant | Default | Purpose |
|---|---:|---|
| `LOG_BBP_EVENTS` | `true` | Master switch for this bridge. |
| `LOG_BUILD_PART` | `true` | Normal BBP part builds. |
| `LOG_ADMIN_BUILD_PART` | `true` | Builds performed with the BBP admin tool. |
| `LOG_DISMANTLE` | `true` | BBP dismantle actions. |
| `LOG_DESTROY` | `true` | BBP destruction actions. |
| `LOG_FOLD` | `true` | BBP fold actions. |
| `LOG_PLACEMENT` | `false` | Optional placement fallback. |
| `LOG_ONLY_BBP_TARGETS` | `true` | Filters shared action hooks to BBP targets/items. |

The bridge constants only narrow telemetry. They do not override disabled ServerSentry categories:

- Build/admin-build/fold respect `LogBaseBuilding`.
- Dismantle respects `LogActionDismantlePart`.
- Destroy respects `LogActionDestroyPart`.
- Placement remains subject to ServerSentry's placement lifecycle switch.

### Optional placement fallback

`LOG_PLACEMENT` is disabled by default because ServerSentry core already observes standard `ActionDeployObject` placement.

Enable it only when testing shows that the installed BBP version or PBO order bypasses the core placement hook. The fallback runs in `OnEndServer`, requires `PlaceObjectActionData.m_AlreadyPlaced`, and uses the shared lifecycle deduplicator to avoid a second event.

## Event mapping

| BBP/DayZ hook | ServerSentry action name | Result timing |
|---|---|---|
| `ActionBuildPart.OnFinishProgressServer` | `basebuilding_build_part` | Emitted after the original action returns. |
| BBP admin-tool build | `basebuilding_admin_build_part` | Emitted after the original action returns. |
| `ActionDismantleBBP.OnFinishProgressServer` | `basebuilding_dismantle_bbp` | Emitted after the original action returns. |
| `ActionDestroyBBP.OnFinishProgressServer` | `basebuilding_destroy_bbp` | Emitted after the original action returns. |
| `ActionFoldBBP.OnFinishProgressServer` | `basebuilding_fold_bbp` | Emitted after the original action returns. |
| `ActionDeployObject.OnEndServer` | `place_object` | Optional; only for confirmed placement state. |

Every emitted bridge event sets:

```text
mod = BaseBuildingPlus
action.reason = bbp_bridge
```

Admin-tool builds use:

```text
action.reason = bbp_admin_tool
```

## Hook design

Destructive actions can invalidate or delete their target. The bridge therefore:

1. creates an immutable ServerSentry snapshot before the original BBP method;
2. calls `super` exactly once;
3. emits the prepared snapshot only after the original method returns.

This preserves target metadata without replacing BBP business logic.

The bridge depends on ServerSentry's shared build-action deduplicator. Because this PBO depends on `ServerSentryScripts`, its hook can reserve a BBP-labelled snapshot before the generic core hook executes.

## Project structure

```text
ServerSentryBBP/
├── $PBOPREFIX$.txt
├── CHANGELOG.md
├── README.md
├── config.cpp
└── scripts/
    ├── 3_Game/Logging/ServerSentryBBPConsts.c
    └── 4_World/hooks/ServerSentryBBPBridge.c
```

## Validation checklist

Before release, compile and test with the exact production versions of DayZ, ServerSentryMod, and BaseBuildingPlus.

Verify:

1. one normal BBP build creates one event;
2. one admin-tool build creates one admin event;
3. dismantle, destroy, and fold each create one event;
4. disabling each ServerSentry category suppresses the corresponding bridge event;
5. vanilla/non-BBP actions are ignored when `LOG_ONLY_BBP_TARGETS` is enabled;
6. optional placement produces one event and only after a successful placement;
7. no event is duplicated by the ServerSentry core hook;
8. every original BBP action still behaves normally.

## Compatibility note

The action class names and `isBBPAdminTool()` method are supplied by BaseBuildingPlus and are therefore version-sensitive. A Workbench/Add-on Builder compile is required after BBP updates.

## License

No license file is included in this package.
