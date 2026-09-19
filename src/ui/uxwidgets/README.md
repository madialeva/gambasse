# UxWidgets (vendored)

Self-contained copy of the reusable Qt Widgets controls library
(`UxTextField`, `UxNumberField`, `UxDateField` and the `UxInput` composites:
label + typed field + selector button in a single control), plus the locally
added `UxLabel` (non-editable label), `UxComboInput` (label + combo box)
composites and `UxCheck` (check box with focus highlighting and read-only
mode).

- Origin: external `Qt.UxWidgets` repository, snapshot 2026-09-13
  (library sources only: `include/`, `src/`; the demo application was not
  vendored).
- Local additions after the snapshot: `UxLabel`, `UxComboInput` and `UxCheck`,
  added as screens needed them; they are not part of the upstream snapshot.
  `UxCheck` is kept at parity with the upstream fixes as of 2026-09-19.
- License: same MIT license as the rest of the repository (`LICENSE.md` at the
  root); no separate license file (same author).
