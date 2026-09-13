# UxWidgets (vendored)

Self-contained copy of the reusable Qt Widgets controls library
(`UxTextField`, `UxNumberField`, `UxDateField` and the `UxInput` composites:
label + typed field + selector button in a single control).

- Origin: external `Qt.UxWidgets` repository, snapshot 2026-09-13
  (library sources only: `include/`, `src/`; the demo application was not
  vendored).
- License: same MIT license as the rest of the repository (`LICENSE.md` at the
  root); no separate license file (same author).
- Scope of this vendoring: the library only needs to compile and link into
  the build. Adopting the controls in the application screens, and adding new
  controls or features, belong to future issues.
