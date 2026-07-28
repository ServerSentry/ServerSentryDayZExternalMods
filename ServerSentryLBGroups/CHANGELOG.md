# Changelog

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
