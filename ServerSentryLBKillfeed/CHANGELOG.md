# Changelog

## 1.1.0

- Keeps the standalone LB Killfeed dependency boundary.
- Retains deterministic one-entry-per-SteamID behavior and verifies the applied state.
- Checks idempotency before volatile LB Killfeed configuration availability.
- Documents compare-and-remove semantics and ServerSentry 1.4.8 compatibility.
- Adds consistent script-only package metadata.

## 1.0.0

- Initial standalone extraction from ServerSentryLBGroups.
