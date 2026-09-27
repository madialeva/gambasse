#!/usr/bin/env bash

set -euo pipefail

# Windows Qt/MinGW build and packaging script.
# Output: a self-contained deploy/ directory with the layout described in the
# README: the Qt-free launcher at the root and the Qt application (with its
# DLLs and plugins) in lib/.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_DIR="$SCRIPT_DIR/build"
DEPLOY_DIR="$SCRIPT_DIR/deploy"
APP_LAUNCHER="gambasse.exe"
APP_QT="Gambasse.exe"
APP_DEPLOY_PATH="$DEPLOY_DIR/$APP_LAUNCHER"
APP_LIB_PATH="$DEPLOY_DIR/lib/$APP_LAUNCHER"

if ! command -v cmake >/dev/null 2>&1; then
  echo "[ERROR] cmake is not in PATH"
  exit 1
fi

if ! command -v windeployqt >/dev/null 2>&1; then
  echo "[ERROR] windeployqt is not in PATH"
  echo "        Add <Qt>/bin to PATH before running this script."
  exit 1
fi

echo "[1/4] Configuring CMake (Release)..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "[2/4] Building project..."
cmake --build "$BUILD_DIR"

if [[ ! -f "$BUILD_DIR/$APP_QT" ]]; then
  echo "[ERROR] Qt executable not found: $BUILD_DIR/$APP_QT"
  exit 1
fi
if [[ ! -f "$BUILD_DIR/$APP_LAUNCHER" ]]; then
  echo "[ERROR] Launcher not found: $BUILD_DIR/$APP_LAUNCHER"
  exit 1
fi

echo "[3/4] Preparing deploy directory..."
rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/lib"

cp "$BUILD_DIR/$APP_LAUNCHER" "$APP_DEPLOY_PATH"
cp "$BUILD_DIR/$APP_QT" "$APP_LIB_PATH"

echo "[4/4] Copying Qt dependencies (windeployqt)..."
# --qmldir scans the QML sources so the Qt Quick modules they import ship too.
windeployqt --release --no-translations --qmldir "$SCRIPT_DIR/qml" "$APP_LIB_PATH"

# MinGW runtime, if windeployqt did not copy it.
MINGW_BIN="$(dirname "$(command -v g++)")"
for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
  if [[ ! -f "$DEPLOY_DIR/lib/$dll" && -f "$MINGW_BIN/$dll" ]]; then
    cp "$MINGW_BIN/$dll" "$DEPLOY_DIR/lib/"
  fi
done

if [[ -f "$SCRIPT_DIR/config.ini" ]]; then
  cp "$SCRIPT_DIR/config.ini" "$DEPLOY_DIR/config.ini"
else
  echo "[WARN] config.ini not found; copy one next to the launcher."
fi

if [[ -f "$SCRIPT_DIR/database.db" ]]; then
  cp "$SCRIPT_DIR/database.db" "$DEPLOY_DIR/database.db"
fi

echo ""
echo "Deployment created at: $DEPLOY_DIR"
echo "Main content:"
echo "  - $APP_DEPLOY_PATH"
echo "  - $APP_LIB_PATH"
echo ""
echo "Ready to package and deploy."
