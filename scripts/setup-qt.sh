#!/usr/bin/env bash
# Installs in the user's directory (no sudo) the Qt of the project: the Qt
# $QT_VERSION desktop kit (linux_gcc_64), through aqtinstall in its own virtual
# environment. The same version and directory as in the maintainer's other Qt
# projects, so one installation serves all.
#
# The default modules are enough: Qt Quick Dialogs and Qt.labs.folderlistmodel come
# with qtdeclarative, and the SQLite driver with qtbase. Nothing is done when the
# kit is already installed, whatever extra modules it has.
#
# System requirements: python3 with the venv module.
#
# Variables:
#   SOFTWARE_DIR   root of the local dependencies  [~/software]
#   QT_VERSION     Qt release                      [6.12.0]
set -euo pipefail

SOFTWARE_DIR="${SOFTWARE_DIR:-$HOME/software}"
QT_VERSION="${QT_VERSION:-6.12.0}"
QT_DIR="$SOFTWARE_DIR/Qt"
KIT="$QT_DIR/$QT_VERSION/gcc_64"
AQT="$SOFTWARE_DIR/aqt-venv/bin/aqt"

if [[ -x "$KIT/bin/qmake" ]]; then
    echo ">> Qt $QT_VERSION already installed in $KIT"
    exit 0
fi

mkdir -p "$SOFTWARE_DIR"
cd "$SOFTWARE_DIR"                     # aqtinstall writes aqtinstall.log to the cwd

if [[ ! -x "$AQT" ]]; then
    python3 -m venv "$SOFTWARE_DIR/aqt-venv"
    "$SOFTWARE_DIR/aqt-venv/bin/pip" install -q --upgrade pip aqtinstall
fi

"$AQT" install-qt linux desktop "$QT_VERSION" linux_gcc_64 -O "$QT_DIR"
echo ">> Qt $QT_VERSION installed in $KIT"
