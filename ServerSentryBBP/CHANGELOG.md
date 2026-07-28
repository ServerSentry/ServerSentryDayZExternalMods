# Changelog

## 1.1.0

- Added English installation, configuration, event, architecture, and validation documentation.
- Added a consistent `$PBOPREFIX$.txt`.
- Updated the package version and script-only config metadata.
- Preserved every third-party method chain with one `super` call.
- Prepared destructive-action snapshots before BBP execution and emitted them afterward.
- Added authoritative ServerSentry configuration checks for build, dismantle, and destroy categories.
- Moved optional placement fallback to `ActionDeployObject.OnEndServer`.
- Required confirmed `PlaceObjectActionData.m_AlreadyPlaced` before optional placement telemetry.
- Kept optional placement disabled by default to prevent duplicate core events.
- Added null guards and a narrow BBP target/item filter.

## 1.0.0

- Initial BBP bridge implementation.
