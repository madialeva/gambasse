<p align="center">
  <img src="resources/img/logo.svg" alt="Gambasse" width="128" height="128" />
</p>

<h1 align="center">Gambasse</h1>

<p align="center">
  <a href="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml"><img src="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml/badge.svg" alt="ci-linux" /></a>
  <a href="LICENSE.md"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License" /></a>
  <a href="#"><img src="https://img.shields.io/badge/c%2B%2B-26-00599C?logo=cplusplus&logoColor=white" alt="C++26" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6.12-41CD52?logo=qt&logoColor=white" alt="Qt 6.12" /></a>
  <a href="#"><img src="https://img.shields.io/badge/CMake-3.25-064F8C?logo=cmake&logoColor=white" alt="CMake" /></a>
  <a href="#"><img src="https://img.shields.io/badge/GCC-16-003057?logo=gcc&logoColor=white" alt="GCC 16" /></a>
  <a href="#"><img src="https://img.shields.io/badge/SQLite-003B57?logo=sqlite&logoColor=white" alt="SQLite" /></a>
</p>

<p align="left">
  Gambasse is a clinical-record and consultation application for NGOs or any health professional
  working with patients in remote, humble areas where there is no population census
  and people lack identity documents. Since there is no official ID to locate a
  patient, the app records who each patient lives with so they can be traced and
  found later, and the medical history also captures social context such as whether
  they live with domestic animals or have treated running water, as well as birth
  dates when known — in many cases there are no birth certificates and people do not
  know when, or even if, they were born. Built in <strong>C++</strong> with
  <strong>Qt Quick (QML)</strong> for the UI and <strong>SQLite</strong> for storage, a
  choice that lets it run on less powerful computers while keeping excellent
  performance.
</p>

---

## Status

| Status | Feature |
|:---:|---|
| ✅ | Splash screen |
| ✅ | Patient master-detail screen |
| ✅ | Column filtering |
| ✅ | Patient CRUD |
| ✅ | Language/theme selection |
| ✅ | Windows launcher |
| ✅ | Linux package (tar.gz) |
| ✅ | Automatic database creation and schema migration |
| ✅ | Histories |
| ✅ | Consultations |
| ✅ | Interactive photo management |
| ⬜ | USB backup |

## Branching and versioning

The default branch is `develop/vX.Y.Z` (currently `develop/v1.0.0`), the version
under development; there is no `main`/`master`. Releasing cuts `release/vX.Y.Z`
and tags `vX.Y.Z`, and hotfixes bump the patch digit. The program version is
declared once in `CMakeLists.txt` and checked against the branch name by the CI.
Feature work happens on short-lived `change/is<n>-<slug>` branches that PR into
`develop/vX.Y.Z`.

## Database

The SQLite file is created on first run if it does not exist, and its schema is
kept up to date automatically. The file path comes from `config.ini`
(`database/path`, `database.db` by default). `database/user` and
`database/password` are fixed connection credentials for now; the stock SQLite
driver ignores them, but the connection is ready for a future first-run setup.

Schema changes ship as embedded SQL patches in `resources/bd/`, named
`<x.y.z.n>_<slug>.sql` (program version plus a sequence that restarts for every
version). Pending patches are applied in order on startup, each inside a
transaction, and recorded in the `b00_bd_migrations` table. A legacy database
whose tables still use the Spanish names and has no migration history is
baselined without re-running the initial schema, so its data is preserved.

## Requirements

- GCC 16.2.0 built from the GNU sources (C++26): `scripts/build-gcc.sh`
  installs it, without root, in `~/software/gcc-16.2.0` (it needs the system
  compiler, about 10 GB of free disk and from half an hour to an hour).
- Qt 6.12.0 desktop: `scripts/setup-qt.sh` installs it, without root, in
  `~/software/Qt/6.12.0/gcc_64` through aqtinstall (it needs `python3` with
  `venv`). The base kit has all the project uses, including the
  `qmllint`/`qmlformat` tools.
- CMake 3.25+ and Ninja.
- Linux deployment: `patchelf`, `ldd` and `objdump` besides the tools above.
- Windows deployment: pending on the move to GCC 16.2 and Qt 6.12 (issue
  #60); until then the configure step rejects the MinGW of the Qt kits.

Both scripts do nothing when the toolchain is already installed, so several
projects can share it. The CI builds with the same GCC (the `gcc:16.2.0-trixie`
image, checked against `scripts/build-gcc.sh`) and the same Qt.

## Linux development

```sh
scripts/build-gcc.sh && scripts/setup-qt.sh   # once
cmake --preset native
cmake --build --preset native
ctest --preset native
./build-linux/Gambasse
```

The `native` preset (`CMakePresets.json`) builds in Release in `build-linux`
with the toolchain in `~/software`; the configure step stops at once with any
compiler older than GCC 16. The executables find GCC 16's `libstdc++` through
their run path, with no `LD_LIBRARY_PATH`. The executable uses `config.ini`
and `database.db` beside itself by default; `--base <directory>` overrides it.
The database file is created and migrated on first run.

## QML UI flow

The declarative UI lives in `qml/` as the `Gambasse` module (`qt_add_qml_module`):
shell (`Root.qml`, `Theme.qml` singleton, `TitleBar.qml`, `ResizeHandles.qml`)
plus the `qml/components/` custom control library and the screens under
`qml/windows/`, started by `App.qml`. C++ only decides and persists: thin
controllers in `src/ui/`, registered in the same module (`QML_ELEMENT`,
`QML_SINGLETON`), expose the `logic/` services and the models to the views as
typed QML types. Keep every file formatted and lint-clean:

```sh
~/software/Qt/6.12.0/gcc_64/bin/qmlformat -i qml/<file>.qml
cmake --build --preset native --target all_qmllint   # must print no warning
```

Windows deployment adds the QML runtime with
`windeployqt --qmldir qml/ ...`. QML development needs no install beyond the
requirements above; the Qt VS Code extension covers editing and previewing.

## Windows development and deployment

> The Windows build has not moved to GCC 16.2 and Qt 6.12 yet (issue #60): the
> configure step rejects the MinGW of the Qt kits. The instructions below
> describe the previous toolchain, and Windows packages come from releases
> before that move.

Use a shell where Qt MinGW, Ninja, CMake, and the Qt `bin` directory are on
`PATH`, then configure as above with `-DCMAKE_PREFIX_PATH="<Qt-prefix>"`.

`deploy-windows.bat` and `deploy-windows.sh` create this self-contained Windows
layout:

```text
<destination>/
├── gambasse.exe       Qt-free launcher
├── config.ini
├── database.db
├── fotos/
└── lib/
    ├── gambasse.exe
    ├── Qt and MinGW runtime DLLs
    └── Qt plugins
```

The launcher starts `lib/gambasse.exe --base <destination>`, keeping Qt DLLs
beside the application while data remains at the distribution root.

## Linux deployment

`deploy-linux.sh` builds with the `release` preset (GCC 16.2 and Qt 6.12 from
`~/software`, see Requirements) and creates
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

There is no launcher: the executable sits at the root, so the base path is
the extracted directory and `config.ini`, `database.db` and `fotos/` live
there, as on Windows. The package ships no database or `config.ini`; the
application creates and migrates the database on first run. The archive keeps
the executable bit, so users only extract it and double-click `Gambasse`.

The graphics stack (OpenGL/EGL, X11/xcb core, fontconfig, FreeType, HarfBuzz,
GLib, D-Bus), glibc and `libgcc_s` are not bundled; every desktop provides
them. GCC 16's `libstdc++` is bundled in `lib/`, because the systems' is older.
The minimum target system is set by the highest glibc version the package
needs: the script prints it at the end (glibc 2.34, which covers Ubuntu 22.04+,
Debian 12+, Fedora 35+ and RHEL 9+).

## Diagnostics

- `--check-db` prints `PATIENTS=<count>`.
- `--svg2png <input.svg> <output.png> <height>` rasterizes an SVG.
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

## License

Released under the **MIT License**. You are free to use, modify, and distribute
this software, including in proprietary projects, provided you retain the
original copyright notice. See [LICENSE.md](LICENSE.md) for details.
