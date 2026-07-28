# Changelog

## 1.1.1

- Fixed the same Enforce Script multiline boolean parsing issue in Market signal matching before it could surface at runtime.
- Added a regression check for unsupported line-leading boolean operators.

## 1.1.0

- Emits Expansion Chat after the original handler runs.
- Emits at most one Market record per operation.
- Distinguishes `success`, `failed`, and `observed` Market outcomes.
- Adds bounded Market logs and stale-context expiration.
- Removes guessed safe-zone settings values.
- Adds null-safe live configuration access and English documentation.
