# AGENTS.md — Gambasse

## Project

Gambasse is a clinical-record and nursing-consultation application for an
NGO in Guinea-Bissau. It replaced an earlier desktop application, now obsolete:
all its features were migrated, except its USB backup, deliberately dropped. The
SQLite schema is owned by this project, and a legacy database with Spanish table
and column names is still supported through the embedded migration patches (see
Persistent compatibility below).

Use Qt Quick (QML), CMake (3.25+) with the presets of `CMakePresets.json`,
Ninja, C++26, GCC 16.2.0 built from the GNU sources and Qt 6.12.0. The project
builds none of its toolchain: it comes from
[toolchain-cpp](https://github.com/madialeva/toolchain-cpp) `v0.2.0`, cloned
next to the repository (`../toolchain-cpp`), with its components `gcc qt` (the
native build, the tests, the Linux package and the CI), `qt-static` (the
self-contained Linux executable) and `gcc-mingw qt-mingw` (the cross compiler
and the static Qt of the Windows executable), installed in
`../toolchain-cpp/software`. The presets name them from `${sourceParentDir}`
and the scripts from `SOFTWARE_DIR`, with no environment variables. Qt is 6.12 everywhere (CMake
requires it; the CI installs it with the same component): never add code paths
or workarounds for other Qt versions. The configure step rejects any compiler
older than GCC 16, the MinGW of the Qt kits included: Windows is only built
from Linux, with the cross compiler. Moving to another toolchain-cpp version
changes together `TOOLCHAIN_CPP_REF` in the CI, the prefixes of the presets if
they change (a MAJOR version of toolchain-cpp), the CI image tag if it brings
another GCC (the CI checks the image against toolchain-cpp's `versions.sh`),
the README and this file. Do not introduce Qt Widgets, MSVC, PostgreSQL, or
new production dependencies without an approved OpenSpec change.

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
└── ui/                      bridge to QML: interface settings, product and build
    │                        information, shared formats
    ├── controller/          one controller per screen (patients, histories)
    └── filter/              view helpers (proxy models)
qml/
├── App.qml                  entry point: splash, then the main shell
├── Root.qml, Theme.qml, TitleBar.qml, ...   shell and theme singleton
├── components/              Gx* custom control library
├── dialogs/                 dialogs of the shell and the screens
└── windows/                 screens
```

Dependencies flow `qml -> ui -> logic -> data`; `logic/` uses only QtCore, the
QtGui image classes (`QImage`, `QImageReader`, `QImageWriter`, painting on a
`QImage`; never windows) and `data/`, never Qt Quick or `ui/`, so it can be
tested without a GUI. QML only
presents: business rules (patient writes, photo files, clinical availability,
language/theme persistence) live in `logic/`, and the `ui/` controllers expose
them to the views.

CMake mirrors the layers: `GambasseCore` (static, `data/` + `logic/`, QtCore,
QtSql and QtGui for the image classes only), `GambasseQml` (the `Gambasse` QML module: the `qml/` views plus
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
- QML is lint-clean and formatted with the Qt 6.12 tools: `cmake --build
  --preset native --target all_qmllint` must print no warning (the CI fails on
  any), and `../toolchain-cpp/software/Qt/6.12.0/gcc_64/bin/qmlformat` must
  leave every file unchanged.

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
source language. The Portuguese catalog is European Portuguese (the norm used in
Guinea-Bissau, 1990 spelling agreement), with titles in sentence case and
drop-down options in capitals as in the other languages; never carry over the
Brazilian, Spanish or Galician forms of the original application. A new text is
translated in both catalogs in the same change: `test_translationcatalogs` fails
on unfinished or empty entries, lost `%n` placeholders and a `.qm` that does not
match its `.ts`. The windows use fixed coordinates, so the `everyTextShowsWhole`
test of each screen fails when a text cannot be read whole in one of the three
languages. Qt's own strings the interface shows (the Linux file chooser)
are translated by hand in `translations/qtstrings_<es|pt|en>.ts`, English
included so the system-language Qt catalog never answers; add any new one the
views come to show. Text uses LF; never line-ending-normalize binaries such as
`.qm`, images, or databases.

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

Keep a 1008×561 base UI with elastic layouts. On Windows the distribution is
the self-contained `Gambasse.exe` alone, with `config.ini`, `database.db` and
`fotos/` beside it: there is no launcher, `lib/` or `windeployqt`, and the
Windows build without `GAMBASSE_STATIC` is rejected at configure time.
`--base <directory>` stays as a general option. Windows GUI diagnostics must be
piped or redirected because the process has no standard console.

In the Linux package (`deploy-linux.sh`, a tar.gz) the executable sits at the
root with RUNPATH `$ORIGIN/lib`, and Qt libraries, plugins and QML
modules live in `lib/`, so `basePath()` is the extracted directory. It must never
ship a `database.db` or `config.ini`. A QML module the views start importing must
also be added to the QML copy list in the script (the Basic style is the only
Controls style shipped). Verify a package with `--check-db` under
`QT_QPA_PLATFORM=offscreen` and an empty environment (`env -i`).

The self-contained executable (`scripts/build-static.sh`, the `static` preset
and the `GAMBASSE_STATIC` option) is built on demand only, on the developer's
machine. It is not part of the checks of every change, nor of the CI: a change
is done without building it, unless it touches the static build itself. It
links the static Qt kit of toolchain-cpp's `qt-static` component
(`../toolchain-cpp/software/Qt/6.12.0-static`), one kit for every project. A Qt
module or plugin the application starts using is added to the component's list
in a change of toolchain-cpp, and to the plugin list of `GAMBASSE_STATIC` in
`CMakeLists.txt` (which, like the package, links no platform theme), without
having to build the kit in the same change.

The Windows executable (`scripts/build-windows.sh`, the `windows` preset and
`GAMBASSE_STATIC` with the cross compiler) is built on demand only too, under
the same rule. It cross-builds `build-windows/dist/Gambasse.exe` with
toolchain-cpp's `gcc-mingw` and `qt-mingw` (`Qt/6.12.0-mingw`, with the
desktop kit as host) and checks that it imports only the DLLs of its list of
Windows DLLs, the same list as guita's. A Qt module or plugin the application
starts using also goes to `qt-mingw`'s list in toolchain-cpp and to the
Windows plugin list of `GAMBASSE_STATIC`. The script's smoke test runs
`--check-db` under Wine, offscreen, with a temporary prefix and home. For
visual checks the executable runs under Wine on `Xvfb :99`, never on the
user's desktop, with a temporary prefix (never `~/.wine`) without its colour
profiles: Wine 10.6 lacks `mscms.dll.WcsCreateIccProfile`, and Qt aborts on
start without that workaround. Wine writes a profile for the X display on each
graphical run, so `windows/system32/spool/drivers/color/` of the prefix is
emptied before every launch. With `QSG_RHI_BACKEND=opengl` the windows are
repainted normally but the splash is never drawn; with Direct3D 11, Qt's
default, the splash shows, but guita found that windows stop repainting after
their first frame (Wine's presentation without a GPU, not the application):
the splash and the rest are checked in separate runs, and on a real Windows.

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
   agent has no SSH access to `origin` from its shell). Commits are partial,
   one per task group of `tasks.md`, so that no commit is huge and the branch
   history follows the tasks:
   - The agent commits a group as soon as all its tasks are done and
     `ctest --preset native` passes; the commit carries the group's code,
     tests and documentation. `openspec/` is not versioned, so the planning
     artifacts and the `tasks.md` checkboxes stay out of the commits.
   - The title starts with the issue as `[is<n>]`, so the commits of an
     issue are easy to follow later, then an English summary of the group
     and its number, for example
     `[is65] Take the toolchain from toolchain-cpp v0.2.0 (group 1)`.
     The body ends with `Refs #<n>` so GitHub links the commit to the issue.
   - Nothing outside the task groups is committed without the user's
     confirmation, and the change is not archived nor sent to a PR until the
     user validates it, including visual verification.
   - Commits and PRs carry no co-authorship or AI attribution: no
     `Co-Authored-By` trailers, no "Generated with" lines.
5. Once the user validates the change, and before any PR, the agent archives
   it on the same branch: syncs its delta specs into `openspec/specs/` and
   moves it to `openspec/changes/archive/YYYY-MM-DD-is<n>-<slug>/`. Whatever
   the archiving changes outside `openspec/` is the last commit of the branch
   (`[is<n>] Archive the <slug> change`); when nothing does, there is no such
   commit. Then it reviews the change's Non-Goals: for each line leaving
   useful pending work, it creates a follow-up issue explaining the pending
   scope, linking the archived change, and keeping the relation to the
   original Non-Goal.
6. The user pushes and opens the PR toward `develop/vX.Y.Z` with `Closes #<n>`
   in the description → the Linux CI (`ci-linux.yml`) validates the PR → the
   user reviews the diff. Before the PR, the user can also run the CI by hand
   on the branch (Actions → ci-linux → Run workflow, or `gh workflow run
   ci-linux.yml --ref <branch>`); a manual run, like a PR, skips the version
   check, which only runs on pushes.
7. Merge commit as the norm (no squash): the per-group commits land on the
   development branch as they are, so its history keeps following the tasks
   of each change, and the merge commit marks where the change ends. Merged
   PR = closed issue (automatic via `Closes`) = archived change.
