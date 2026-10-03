# AGENTS.md — Gambasse

## Project

Gambasse is a clinical-record and nursing-consultation application for an
NGO in Guinea-Bissau. It continues an earlier desktop application that is no
longer maintained; the SQLite schema is owned by this project, and a legacy
database with Spanish table and column names is still supported through the
embedded migration patches (see Persistent compatibility below).

A local copy of the original application lives in `tmp/dispensario/`
(git-ignored, so it may be absent after a fresh clone) — consult it as a
functional and UX reference for every screen and business rule still to be
developed: `src/Dispensario/Forms/` (including the six
`FrmHistoria*`/`FrmConsulta*` child forms), `src/Dispensario/ORM/` (data access
per area), and `src/Dispensario/Utilidades/` (shared framework, NOT to be
ported: Qt covers those services natively).

Use Qt Quick (QML), CMake, Ninja, and a MinGW/GCC Qt kit for Windows
deployment. Qt is 6.6 everywhere (CMake requires it; the CI installs 6.6.2):
never add code paths or workarounds for other Qt versions. Do not introduce Qt Widgets, MSVC, PostgreSQL, or new production
dependencies without an approved OpenSpec change.

## Source layout

The Qt sources under `src/` are organized by layer, and the views live in the
`Gambasse` QML module under `qml/`:

```text
src/
├── main.cpp
├── Paths.h                  shared base-path helper
├── data/
│   ├── common/              database and migration infrastructure
│   └── model/               data model (Patient, PatientsModel)
├── logic/                   business logic, UI-independent
└── ui/                      bridge to QML: interface settings, shared formats
    ├── controller/          one controller per screen (patients, histories)
    └── filter/              view helpers (proxy models)
qml/
├── App.qml                  entry point: splash, then the main shell
├── Root.qml, Theme.qml, TitleBar.qml, ...   shell and theme singleton
├── components/              Gx* custom control library
└── windows/                 screens
```

Dependencies flow `qml -> ui -> logic -> data`; `logic/` uses only QtCore and
`data/`, never Qt Quick or `ui/`, so it can be tested without a GUI. QML only
presents: business rules (patient writes, photo files, clinical availability,
language/theme persistence) live in `logic/`, and the `ui/` controllers expose
them to the views.

CMake mirrors the layers: `GambasseCore` (static, `data/` + `logic/`, QtCore
and QtSql only), `GambasseQml` (the `Gambasse` QML module: the `qml/` views plus
the `ui/` C++ types) and the `Gambasse` executable (`main.cpp` + resources).
Tests link the layer they exercise through `gambasse_add_test()` in
`tests/CMakeLists.txt` instead of listing sources.

## QML and C++

C++ types reach QML by declarative registration in the `Gambasse` module, never
by context properties or objects injected from `main.cpp`:

- Screen controllers use `QML_ELEMENT`. A view that owns its controller
  instantiates it (`Root.qml` creates `PatientController`); controllers created
  by another one (the history controllers) add `QML_UNCREATABLE`.
- Application-wide services are `QML_SINGLETON` with a static `create()`
  (`InterfaceSettings`). Qt prefers a default constructor over `create()`, so
  such a class must not be default-constructible.
- Views declare typed properties (`property PatientController controller`),
  never `var`, so `qmllint` checks every access.
- `main.cpp` only opens the database, handles the diagnostic flags and loads
  `App`; no wiring between C++ objects and views lives there.

## OpenSpec

OpenSpec is the default workflow for changes. For each change: create
`proposal.md` and `design.md` (plus `tasks.md` and required `specs/` deltas),
then STOP until the user approves proposal and design. Do not start
implementing the tasks until then. Leave a compiling build for visual
verification and archive only on user confirmation.

This is not mandatory. If the programmer explicitly states in a change request
that OpenSpec should not be used (or that the change should be made directly),
skip the OpenSpec workflow and implement the change directly.

Artifact language: prose in `proposal.md`, `design.md`, `tasks.md` and
`specs/*.md` (motivation, decisions, requirement/scenario descriptions, etc.)
is written in Spanish, except the fixed labels imposed by the OpenSpec skills,
which stay in English as-is (`Why`, `What Changes`, `Capabilities`, `Impact`,
`Context`, `Goals`/`Non-Goals`, `Decisions`, `Risks / Trade-offs`, `Migration
Plan`, `Open Questions`, `Purpose`, `ADDED`/`MODIFIED`/`REMOVED`/`RENAMED
Requirements`, `Requirement:`, `Scenario:`, `WHEN`/`THEN`/`AND`,
`SHALL`/`MUST`). Code identifiers (classes, methods, packages) also stay in
English inside the Spanish text. This is independent of the GitHub issues
language (always English, see below).

## Persistent compatibility

The SQLite schema is owned and versioned by this project through embedded
migration patches (`resources/bd/`). Table names keep the `b0N_` ordering prefix
and are English; column names are English `snake_case` without the `b0N_` prefix.
Keep `config.ini` keys/values, date formats, existing photo filename formats, and
the domain values `Home` and `Muller` stable. Never edit an applied patch; add a
new one. Enable `PRAGMA foreign_keys = ON` per connection. Bind optional QString
values to `TEXT NOT NULL` fields with `nonNull()`.

## Language and files

Use English for source, identifiers, comments, CMake, scripts, technical docs,
commits, and pull requests. GitHub issues and milestones: titles and
descriptions always in English. OpenSpec artifacts and user conversation remain in
Spanish. UI text must use `qsTr()` in QML and `tr()` in C++; English is the
source language and the Spanish and Portuguese catalogs must preserve their
current displayed text. Text uses LF;
never line-ending-normalize binaries such as `.qm`, images, or databases.

## C++ includes

Every `#include` uses angle brackets, never double quotes: Qt and system headers
(`<QtTest>`, `<QSqlDatabase>`) and project headers resolved from the `src`
include root (`<Paths.h>`, `<data/common/Database.h>`,
`<ui/controller/PatientController.h>`,
`<ui/filter/ColumnFilterProxy.h>`). Never use relative
include paths such as `"../Paths.h"`. The only exception is the Qt AUTOMOC
generated file for a `Q_OBJECT` defined in a `.cpp`, which stays double-quoted
(`#include "test_schemamigrator.moc"`). CMake must expose `src` as the include
root of the application and the tests so angle includes resolve. The QML type
registration Qt generates includes each registered header by its bare file
name, so `GambasseQml` also adds `src/ui` and `src/ui/controller` as private
include directories; project code never relies on them.

## Deployment notes

Keep a 1008×561 base UI with elastic layouts. The Windows launcher starts the Qt
application in `lib/` using `--base <root>`. `windeployqt` must get
`--qmldir qml` so the Qt Quick modules ship. Windows GUI diagnostics must be piped
or redirected because the process has no standard console.

The Linux package (`deploy-linux.sh`, a tar.gz) has no launcher: the executable
sits at the root with RUNPATH `$ORIGIN/lib`, and Qt libraries, plugins and QML
modules live in `lib/`, so `basePath()` is the extracted directory. It must never
ship a `database.db` or `config.ini`. A QML module the views start importing must
also be added to the QML copy list in the script (the Basic style is the only
Controls style shipped). Verify a package with `--check-db` under
`QT_QPA_PLATFORM=offscreen` and an empty environment (`env -i`).

## Working preferences

- Conversation with the user in Spanish.
- Prefer durable context in repo files (this AGENTS.md) over internal agent
  memory, so it survives clones/moves of the repository.
- This file is versioned in the repository as durable context. `openspec/`,
  `.opencode/`, and `.claude/` stay private working material: they are
  git-ignored and NOT published. Do not reference them from published files
  (README, ...). The only public project description is the
  README — update it when the plan changes visibly.

## GitHub workflow (issues, branches, PRs)

There is no `main`/`master`. The long-lived, default branch is
`develop/vX.Y.Z` (for example `develop/v1.0.0`), which holds the version
currently under development. Releasing cuts `release/vX.Y.Z` from it and tags
`vX.Y.Z`; hotfixes bump the patch digit. When a new cycle starts,
`develop/vX.Y.Z` is created from the released tag and becomes the default
branch. The program version lives as the single source of truth in
`CMakeLists.txt` (`project(... VERSION ...)`) and must match the branch suffix;
the Linux CI (`ci-linux.yml`) enforces it on `develop/**` and `release/**`
pushes.

Each OpenSpec change is tracked on GitHub with this cycle:

1. OpenSpec proposal approved by the user.
2. GitHub issue for the change, linking its `openspec/changes/<name>/`
   folder (plus its milestone once milestones exist; the Projects board stays
   light: Todo / In progress / Done). While active, the change folder is named
   `is<n>-<slug>` after its issue; the date prefix is added only when the
   change is archived.
3. Branch created from the issue (Development panel → "Create a branch";
   name like `change/is<n>-<slug>`) starting from `develop/vX.Y.Z`.
4. Implementation on the branch + push (pushes are done by the user; the
   agent has no SSH access to `origin` from its shell). The agent never
   commits on its own: work stays uncommitted on the branch until the user
   validates it (including visual verification); commit only after the user
   explicitly confirms.
5. PR toward `develop/vX.Y.Z` with `Closes #<n>` in the description → the Linux
   CI (`ci-linux.yml`) validates the PR → user reviews the diff.
6. Squash merge as the norm (one change = one clean commit on the development
   branch). Exception: PRs whose intermediate commits have standalone value
   (e.g. massive deletions separated from new code) → normal merge.
7. The last commit on the branch may be the archiving of the change (only
   after user confirmation), so merged PR = closed issue (automatic via
   `Closes`) = archived change.
8. After archiving a change, always review its Non-Goals section. For each
   line leaving useful pending work, create a follow-up issue explaining the
   pending scope, linking the archived change, and keeping the relation to
   the original Non-Goal.
