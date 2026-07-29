# Changelog

## 1.8.2

- Migrates Flagpole actions to the new `base_flagpole` target type and `ServerSentryFlagPoleModActionHandler`.
- Migrates the map right-click action to `contextMenu` and `ServerSentryContextMenuModActionHandler`.
- Uses the registered stable action IDs directly and removes the obsolete `ServerSentryLiveBaseDto` constructor injection.
- Adds the upstream Advanced Groups Flagpole detail provider for group name, tag, level, and member count.
- Adds the upstream `LBmaster_GroupDLCPlotpole` addon dependency.
- Requires the 2026-07-29 ServerSentryDayZMod API update (`17d9ea8` or newer).
- Documents the renamed core PBO `ServerSentryDayZMod.pbo`; the obsolete `ServerSentryMod.pbo` must not be loaded in parallel.

## 1.8.1

- Includes vehicle roots and all ServerSentry-recognized Flagpoles in both radius cleanup actions.
- Deletes the selected Flagpole when running `Delete all items in flagpole radius`.
- Skips only occupied vehicles to preserve their crew; unoccupied vehicles are deleted.
- Continues to preserve players and player inventories while excluding other static map structures, animals, and infected.

## 1.8.0

- Adds live-map Flagpole actions to raise and lower vanilla DayZ territory flags.
- Adds a dangerous Flagpole action that deletes root world items in the vanilla territory radius.
- Adds a dangerous live-map context action that deletes root world items in a configurable 1–500 meter horizontal radius.
- Preserves players, player inventories, vehicles, vehicle attachments, and every Flagpole recognized by ServerSentry's live-map classifier during radius cleanup.
- Supports Flagpole-radius deletion from both vanilla-compatible and name-compatible Flagpoles recognized by ServerSentry.
- Uses a set for linear-time root-item deduplication and warns administrators about large-radius scans on dense bases.
- Uses persistent ServerSentry fulfillment receipts to prevent duplicate radius cleanup execution.
- Publishes the new Flagpole action IDs on live base DTOs.
- Aligns the packaged mod version with the documented release version.

## 1.7.1

- Preserves the previous group snapshot when LBmaster temporarily exposes a null/unloaded member collection.
- Prevents false join telemetry during the next group reconciliation.

## 1.7.0

- Adds stable member snapshot keys and correct key-based removal.
- Moves direct event capture after successful LBmaster operations.
- Stops mutating LBmaster-owned member collections.
- Treats unloaded/null collections as unknown rather than mass removal.
- Adds null-safe group loading, orphan snapshot cleanup, and bounded deduplication.
- Reserves fulfillment keys before volatile group checks and verifies applied clan-tag state.
- Updates action metadata and documentation to English.

## 1.6.0

- Removed the LB Killfeed dependency.
- Moved `shop.player.killfeed.color` into `ServerSentryLBKillfeed`.
