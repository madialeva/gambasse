<p align="center">
  <img src="resources/img/logo.svg" alt="Gambasse" width="128" height="128" />
</p>

<h1 align="center">Gambasse</h1>

<p align="center">
  <a href="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml"><img src="https://github.com/madialeva/gambasse/actions/workflows/ci-linux.yml/badge.svg" alt="ci-linux" /></a>
  <a href="LICENSE.md"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License" /></a>
  <a href="#"><img src="https://img.shields.io/badge/c%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17" /></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6-41CD52?logo=qt&logoColor=white" alt="Qt 6" /></a>
  <a href="#"><img src="https://img.shields.io/badge/CMake-3.21-064F8C?logo=cmake&logoColor=white" alt="CMake" /></a>
  <a href="#"><img src="https://img.shields.io/badge/GCC-11-003057?logo=gcc&logoColor=white" alt="GCC 11" /></a>
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
  <strong>Qt Widgets</strong> for the UI and <strong>SQLite</strong> for storage, a
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
| ⬜ | Histories |
| ⬜ | Consultations |
| ⬜ | Interactive photo management |
| ⬜ | USB backup |

## Requirements

- GCC 11+ (C++17).
- Qt 6 Core, Gui, Widgets and Sql, with the SQLite driver.
- CMake 3.21+ and Ninja.
- Windows deployment: a Qt MinGW kit, `windeployqt`, and MinGW runtime DLLs.

## Linux development

```sh
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux
./build-linux/Gambasse
```

When Qt is not discoverable, add
`-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64`. The executable uses `config.ini`
and `database.db` beside itself by default; `--base <directory>` overrides it.

## Windows development and deployment

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

## Diagnostics

- `--check-db` prints `PATIENTS=<count>`.
- `--svg2png <input.svg> <output.png> <height>` rasterizes an SVG.
- `--crud-selftest` executes CRUD in a transaction and rolls it back.

## Internationalization

English is the source language. Spanish and Portuguese catalogs are embedded from
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
