# Gambasse — Developer Documentation

Technical documentation for building, testing, packaging, and maintaining
Gambasse. For an end-user overview, see [README.md](../README.md)
([Español](../README_es.md)); for project architecture rules and workflow conventions, see [AGENTS.md](../AGENTS.md). All
paths and commands below are relative to the repository root.

---

## Branching and versioning

The default branch is `develop/vX.Y.Z` (currently `develop/v1.0.0`), the version
under development; there is no `main`/`master`. Releasing cuts `release/vX.Y.Z`
and tags `vX.Y.Z`, and hotfixes bump the patch digit. The program version is
declared once in `CMakeLists.txt` and checked against the branch name by the CI.
Feature work happens on short-lived `change/is<n>-<slug>` branches that PR into
`develop/vX.Y.Z`.

## Configuration and database

Settings live in `config.ini` beside the executable (or in the directory passed
with `--base <directory>`):

- `idioma/codigo`: active interface language (`pt` by default; `pt`, `es`, `en`).
- `tema/modo`: active UI theme (`claro` by default; `claro`, `oscuro`).
- `letra/pixeles`: shared letter size in pixels for zoomed history and
  consultation windows (`13` by default, clamped between `13` and `26`, i.e.
  up to 2× the 13 px base layout).
- `database/path`: SQLite database path (`database.db` by default; relative paths
  resolve from the base path). `database/user` and `database/password` are
  fixed connection credentials for now; the stock SQLite driver ignores them,
  but the connection is ready for a future first-run setup.

The SQLite file is created on first run if it does not exist, and its schema is
kept up to date automatically. Schema changes ship as embedded SQL patches in
`resources/bd/`, named `<x.y.z.n>_<slug>.sql` (program version plus a sequence
that restarts for every version). Pending patches are applied in order on
startup, each inside a transaction, and recorded in the `b00_bd_migrations`
table. A legacy database whose tables still use the Spanish names and has no
migration history is baselined without re-running the initial schema, so its
data is preserved.

## Requirements

- [toolchain-cpp](https://github.com/madialeva/toolchain-cpp) `v0.2.0`, cloned
  next to this repository (`../toolchain-cpp`). It builds and installs, without
  root, in its own `software/` directory, the toolchain of the project:
  - `gcc`: GCC 16.2.0 built from the GNU sources (C++26);
  - `qt`: the Qt 6.12.0 desktop kit, with the `qmllint`/`qmlformat` tools;
  - `qt-static` (optional, see Static build): a static Qt 6.12.0 for the
    self-contained executable.

  What each component needs from the system (disk, time, development
  packages) is in toolchain-cpp's README and `docs/toolchain.md`. The same
  clone serves the maintainer's other projects.
- CMake 3.25+ and Ninja.
- Linux deployment: `patchelf`, `ldd` and `objdump` besides the tools above.
- Windows executable (optional, see Windows build): toolchain-cpp's
  `gcc-mingw` (the GCC 16.2.0 cross compiler for Windows) and `qt-mingw` (a
  static Qt 6.12.0 for Windows).

The CI builds with the same GCC (the `gcc:16.2.0-trixie` image, checked
against the pinned toolchain-cpp) and the same Qt, which it installs with
toolchain-cpp at the same version.

## Linux development

```sh
# once: toolchain-cpp next to this repository, with the native components
git clone --branch v0.2.0 https://github.com/madialeva/toolchain-cpp.git ../toolchain-cpp
../toolchain-cpp/install.sh gcc qt

cmake --preset native
cmake --build --preset native
ctest --preset native
./build-linux/Gambasse
```

The `native` preset (`CMakePresets.json`) builds in Release in `build-linux`
with the toolchain in `../toolchain-cpp/software`; the configure step stops at
once with any compiler older than GCC 16. With toolchain-cpp somewhere else, a
`CMakeUserPresets.json` (ignored by git) changes those paths, and
`SOFTWARE_DIR` those of the scripts. The executables find GCC 16's `libstdc++`
through their run path, with no `LD_LIBRARY_PATH`. The executable uses
`config.ini` and `database.db` beside itself by default; `--base <directory>`
overrides it. The database file is created and migrated on first run.

## QML UI flow

The declarative UI lives in `qml/` as the `Gambasse` module (`qt_add_qml_module`):
shell (`Root.qml`, `Theme.qml` singleton, `TitleBar.qml`, `ResizeHandles.qml`)
plus the `qml/components/` custom control library, the `qml/dialogs/` dialogs
and the screens under `qml/windows/`, started by `App.qml`. History and
consultation windows derive from `qml/windows/FormWindow.qml`, which scales its
13 px base `stage` as a whole (via `ZoomController`, magnifier buttons in the
title bar, or `Ctrl` + `+`, `Ctrl` + `-`, `Ctrl` + `0`) so controls keep their
positions without exceeding the screen work area.

C++ only decides and persists: thin controllers in `src/ui/`, registered in the
same module (`QML_ELEMENT`, `QML_SINGLETON`), expose the `logic/` services and
the models to the views as typed QML types. Keep every file formatted and
lint-clean:

```sh
../toolchain-cpp/software/Qt/6.12.0/gcc_64/bin/qmlformat -i qml/<file>.qml
cmake --build --preset native --target all_qmllint   # must print no warning
```

The self-contained executables link the QML modules the views import
(found by `qmlimportscanner`). QML development needs no install beyond the
requirements above; the Qt VS Code extension covers editing and previewing.

## Windows build

Windows is built from Linux only: the `windows` preset cross-builds a
self-contained `Gambasse.exe` with toolchain-cpp's `gcc-mingw` cross compiler
and its static Qt for Windows (`qt-mingw`), with the desktop kit as the host
for Qt's tools. Qt, its plugins (Windows platform, offscreen, SQLite,
JPEG/SVG), the QML modules and the C++ runtime are linked into the executable:
there is no launcher, `lib/`, or DLL beside it. Configuring for Windows
without the preset, or with the MinGW of the Qt kits, fails.

It is built on demand on the developer's machine only, like the static Linux
executable: the CI, the tests and the Linux package do not use it.

```sh
../toolchain-cpp/install.sh gcc-mingw qt-mingw   # once: cross compiler, Qt for Windows
scripts/build-windows.sh                         # build-windows/dist/Gambasse.exe
```

The script builds with the `windows` preset (`build-windows`, Release, no
tests), copies the executable without symbols to
`build-windows/dist/Gambasse.exe` and fails, naming them, if it imports a DLL
that is not part of Windows 10 1809 (the oldest Windows Qt 6.12 supports), and
if it does not embed its icon and version information.
When Wine is installed, it then tries a copy in an empty directory with a
temporary Wine prefix: `--check-db` must create and migrate the database;
without Wine it skips that test with a warning.
`scripts/build-windows.sh --check <executable>` runs only the DLL and resource
checks.

`Gambasse.exe` takes 54 MB and needs Windows 10 1809 or later, with no Qt,
MinGW or Visual C++ runtime installed. It keeps `config.ini`, `database.db`
and `fotos/` beside itself, or in the directory given with `--base`, and
creates the database on first run. The photo chooser is the native Windows
dialog. The Explorer and the shortcuts show the application's logo, and the
file properties show its name and version, which come from `CMakeLists.txt`.

The icon is `resources/img/logo.ico`, committed so that the build needs no
rasterizer. When `resources/img/logo.svg` changes, regenerate it with
`scripts/make-windows-icon.py`, which needs `rsvg-convert` and Pillow.

To update an installation made with an older package (a `gambasse.exe`
launcher at the root and the application in `lib/`): back up the folder, copy
`Gambasse.exe` over the root `gambasse.exe`, delete `lib/` and open the
application. The database, `config.ini` and `fotos/` stay where they were,
and the schema is migrated if needed. To go back, restore the whole folder
from the backup.

## Linux deployment

`deploy-linux.sh` builds with the `release` preset (GCC 16.2 and Qt 6.12 from
toolchain-cpp, see Requirements) and creates
`deploy-linux/gambasse-<version>-linux-x86_64.tar.gz` (the version comes from
`CMakeLists.txt`) plus the extracted directory beside it. The plugins and QML
modules come from the same Qt the build used. It needs `cmake`, `ninja`,
`patchelf`, `ldd` and `objdump`.

```text
gambasse-<version>-linux-x86_64/
├── Gambasse           the application (RUNPATH $ORIGIN/lib)
├── qt.conf            plugin and QML paths relative to lib/
├── LEEME-linux.txt    end-user instructions (ES/PT/EN)
├── fotos/
└── lib/
    ├── Qt libraries and their third-party dependencies
    ├── plugins/       xcb, offscreen, SQLite, JPEG/SVG, input contexts
    └── qml/           QtQml, the QtQuick modules the views import and
                       Qt.labs.folderlistmodel (photo file chooser)
```

The executable sits at the root, so the base path is the extracted directory
and `config.ini`, `database.db` and `fotos/` live there, as beside the Windows
executable. The package ships no database or `config.ini`; the
application creates and migrates the database on first run. The archive keeps
the executable bit, so users only extract it and double-click `Gambasse`.

The graphics stack (OpenGL/EGL, X11/xcb core, fontconfig, FreeType, HarfBuzz,
GLib, D-Bus), glibc and `libgcc_s` are not bundled; every desktop provides
them. GCC 16's `libstdc++` is bundled in `lib/`, because the systems' is older.
The minimum target system is set by the highest glibc version the package
needs: the script prints it at the end (glibc 2.34, which covers Ubuntu 22.04+,
Debian 12+, Fedora 35+ and RHEL 9+).

## Static build

`scripts/build-static.sh` builds a self-contained executable: a single file
with Qt, its plugins (xcb, offscreen, SQLite, JPEG/SVG), the QML modules and
GCC 16's C++ runtime linked statically. It can be copied to another Linux
machine and run without installing Qt or anything from toolchain-cpp. It runs
on any x86-64 CPU, including those older than `x86-64-v2`, such as an Intel
Core 2 or an AMD Phenom. It is built on demand on the developer's machine only:
the CI, the tests and the Linux package do not use it.

```sh
../toolchain-cpp/install.sh qt-static   # once: the static Qt kit
scripts/build-static.sh                 # build-static/dist/Gambasse
```

The script builds with the `static` preset (`build-static`, Release, no
tests), copies the executable without symbols to `build-static/dist/Gambasse`
and fails if it carries a run path, needs a shared library of Qt or of the C++
runtime, or one from toolchain-cpp, or declares that it needs more than the
x86-64 baseline CPU. Then it tries a copy in an empty directory and
environment: `--check-db` must create and migrate the database.
`scripts/build-static.sh --check <executable>` runs only the check.

Like the package, the executable keeps `config.ini`, `database.db` and
`fotos/` beside itself, and it creates the database on first run. It takes
50 MB without symbols. Only libraries of the operating system stay dynamic:
glibc 2.34 or later, OpenGL/EGL, X11/xcb and xkbcommon, fontconfig, freetype
and zlib. On a minimal Ubuntu 24.04 these packages are enough (Mesa renders in
software when there is no GPU driver):

```text
libx11-6 libx11-xcb1 libxcb1 libxcb-glx0 libxcb-cursor0 libxcb-icccm4
libxcb-util1 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0
libxcb-shm0 libxcb-sync1 libxcb-xfixes0 libxcb-render0 libxcb-shape0
libxcb-xkb1 libxkbcommon0 libxkbcommon-x11-0 libegl1 libglx0 libopengl0
libgl1-mesa-dri libglx-mesa0 libegl-mesa0 libfreetype6 libfontconfig1 zlib1g
fonts-dejavu-core
```

The executable is built without ICU, which the application does not need: it
sorts the patient list with an order of its own (see Internationalization), the
same as the development build's on every system.

## Diagnostics

- `--check-db` prints `PATIENTS=<count>`.
- `--crud-selftest` executes CRUD in a transaction and rolls it back.

## Internationalization

English is the source language; the interface can be switched between Spanish,
Portuguese and English. Spanish and Portuguese catalogs are embedded from
`translations/gambasse_es.qm` and `translations/gambasse_pt.qm`. Regenerate
them after editing TS files:

```sh
lrelease translations/gambasse_es.ts -qm translations/gambasse_es.qm
lrelease translations/gambasse_pt.ts -qm translations/gambasse_pt.qm
```

A new text is translated in the same change that adds it. The
`translationcatalogs` test fails when a catalog has unfinished or empty entries
or loses a `%n` placeholder, and when a `.qm` does not match its `.ts`.

The calendar of the date fields follows the interface language too, with the
month and day names of Qt's locale data, whatever the system's language:
Portuguese takes the conventions of Guinea-Bissau (`pt_GW`), Spanish those of
Spain (`es_ES`) and English those of the United Kingdom (`en_GB`), so the week
always starts on Monday. The fields keep their `dd/MM/yyyy` dates in every
language.

The patient list is sorted alphabetically in the interface language, the same
in every build and on every system: case does not count, an accented letter
sorts with its base letter (`CESAR CÉSAR CUNHA`), and `Ñ` is a letter of its
own after `N` in Spanish and an `N` in Portuguese and English. The application
does this with its own comparison (`src/logic/TextOrder.cpp`), on Qt's Unicode
tables, because Qt's collation follows the system's and, without ICU, as in the
self-contained executables, ignores the locale it is given. Changing the
language sorts the list again and keeps the selected patient.

The windows use fixed coordinates, so a longer text may not fit. The
`everyTextShowsWhole` test of each screen fails when a text cannot be read whole
in one of the three languages.
Those measures hold on every system because the font is embedded in the
application, and the tests run with no system font.

Qt's own strings that the interface shows, those of the Linux photo file
chooser (side bar places, buttons, filter), follow the interface language
too. They live in `translations/qtstrings_<es|pt|en>.ts`, written by hand
because `lupdate` does not scan Qt, and are compiled and embedded like the
others. Qt's own catalogs are not used: they leave the chooser untranslated in
Spanish and are Brazilian in Portuguese.

## Licensing notes for distributions

See [LICENSE.md](../LICENSE.md) and the `License` section in
[README.md](../README.md). When distributing the self-contained Linux or
Windows executables (which link Qt statically under LGPL v3), ship the LGPL
text and Qt's notices alongside the binary, and preserve the embedded font
notice (`resources/fonts/OFL.txt` for Noto Sans under SIL OFL 1.1).
