# UxWidgets (vendored)

Self-contained copy of the reusable Qt Widgets controls library
(`UxTextField`, `UxNumberField`, `UxDateField` and the `UxInput` composites:
label + typed field + selector button in a single control), plus the locally
added `UxLabel` (non-editable label) and `UxComboInput` (label + combo box)
composites.

- Origin: external `Qt.UxWidgets` repository, snapshot 2026-09-13
  (library sources only: `include/`, `src/`; the demo application was not
  vendored).
- Local additions after the snapshot: `UxLabel` and `UxComboInput`, ported
  from the original VB.NET controls (`UxLabel.vb`, `UxComboBox.vb`/`UxCombo.vb`)
  as screens needed them. They are not part of the upstream snapshot.
- License: same MIT license as the rest of the repository (`LICENSE.md` at the
  root); no separate license file (same author).
