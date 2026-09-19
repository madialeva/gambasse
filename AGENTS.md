# AGENTS.md — DispensarioQt

## Project

DispensarioQt is the Qt Widgets/C++17 rewrite of a clinical-record and
nursing-consultation application for an NGO in Guinea-Bissau. The production
.NET application is the functional reference. Both use the same SQLite database,
so persistent contracts take precedence over naming consistency.

The reference is a VB.NET WinForms application on .NET Framework 4.5, fully
operational on real PCs. A local copy lives in `tmp/dispensario/` (git-ignored,
so it may be absent after a fresh clone) — consult it for every piece of Qt
business logic and every screen still to be developed: `src/Dispensario/Forms/`
(including the six `FrmHistoria*`/`FrmConsulta*` child forms),
`src/Dispensario/ORM/` (data access per area), and
`src/Dispensario/Utilidades/` (shared framework, NOT to be ported: Qt covers
those services natively).

Use Qt Widgets, CMake, Ninja, and a MinGW/GCC Qt kit for Windows deployment. Do
not introduce QML, MSVC, PostgreSQL, or new production dependencies without an
approved OpenSpec change.

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

Never change SQLite tables or columns (`b01_*`, `b03_*` through `b08_*`), SQL
literals, `config.ini` keys/values, date formats, existing photo filename formats,
or domain values `Home` and `Muller`. Enable `PRAGMA foreign_keys = ON` per
connection. Bind optional QString values to `TEXT NOT NULL` fields with
`nonNull()`.

## Language and files

Use English for source, identifiers, comments, CMake, scripts, technical docs,
commits, and pull requests. GitHub issues and milestones: titles and
descriptions always in English. OpenSpec artifacts and user conversation remain in
Spanish. UI text must use `tr()`; English is the source language and the Spanish
and Portuguese catalogs must preserve their current displayed text. Text uses LF;
never line-ending-normalize binaries such as `.qm`, images, or databases.

## Deployment notes

Keep a 1008×561 base UI with elastic layouts. The Windows launcher starts the Qt
application in `lib/` using `--base <root>`. `QToolBar::addWidget()` visibility is
controlled by its returned `QAction`. Windows GUI diagnostics must be piped or
redirected because the process has no standard console.

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
