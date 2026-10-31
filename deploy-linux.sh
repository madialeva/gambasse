#!/usr/bin/env bash

set -euo pipefail

# Linux Qt build and packaging script.
# Output: a self-contained deploy-linux/gambasse-<version>-linux-x86_64/ directory
# and its .tar.gz: the Qt application at the root and the Qt libraries, plugins
# and QML modules in lib/, found through RUNPATH (no launcher needed). Data
# (config.ini, database.db, fotos/) lives at the root, as in the Windows layout.
#
# It builds with the release preset (CMakePresets.json): GCC 16 and Qt 6.12 from
# ~/software (scripts/build-gcc.sh, scripts/setup-qt.sh). The plugins and QML
# modules come from the same Qt the build used, and GCC 16's libstdc++, newer than
# the system's, travels in lib/. Requires cmake, ninja, patchelf, ldd and objdump.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_DIR="$SCRIPT_DIR/build-deploy"
DEPLOY_DIR="$SCRIPT_DIR/deploy-linux"
APP="Gambasse"

VERSION="$(sed -n 's/^project(Gambasse VERSION \([0-9][0-9.]*\).*/\1/p' "$SCRIPT_DIR/CMakeLists.txt")"
if [[ -z "$VERSION" ]]; then
  echo "[ERROR] Could not read the version from CMakeLists.txt"
  exit 1
fi
PKG_NAME="gambasse-$VERSION-linux-x86_64"
PKG_DIR="$DEPLOY_DIR/$PKG_NAME"
LIB_DIR="$PKG_DIR/lib"

for tool in cmake ninja patchelf ldd objdump; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "[ERROR] $tool is not in PATH"
    exit 1
  fi
done

# System libraries that every desktop provides and that must not travel with
# the package: they depend on the graphics driver or on the running system.
# Everything else (Qt, ICU, xcb helpers, GCC 16's libstdc++, ...) is bundled.
# libgcc_s stays out: GCC 16's needs glibc 2.35, and nothing shipped asks the
# system's for more than GCC_4.3.0.
SYSTEM_LIBS='^(ld-linux.*|libc|libm|libdl|libpthread|librt|libutil|libresolv|libanl|libnsl|libgcc_s|libGL|libGLX|libGLdispatch|libOpenGL|libEGL|libGLESv2|libgbm|libdrm|libX11|libX11-xcb|libxcb|libXau|libXdmcp|libfontconfig|libfreetype|libharfbuzz|libglib-2\.0|libgobject-2\.0|libgio-2\.0|libgmodule-2\.0|libdbus-1|libsystemd|libselinux|libudev|libasound)\.so'

echo "[1/6] Configuring CMake (release preset)..."
# --fresh: a cache from another compiler or Qt would win over the preset.
(cd "$SCRIPT_DIR" && cmake --preset release --fresh)

# The Qt the build found (Qt6_DIR is <prefix>/lib/cmake/Qt6).
QT_CMAKE_DIR="$(sed -n 's/^Qt6_DIR:PATH=//p' "$BUILD_DIR/CMakeCache.txt")"
QMAKE="$(cd "$QT_CMAKE_DIR/../../.." && pwd)/bin/qmake"
if [[ ! -x "$QMAKE" ]]; then
  echo "[ERROR] qmake not found next to the Qt of the build: $QMAKE"
  exit 1
fi
QT_PLUGINS="$("$QMAKE" -query QT_INSTALL_PLUGINS)"
QT_QML="$("$QMAKE" -query QT_INSTALL_QML)"

echo "[2/6] Building project..."
(cd "$SCRIPT_DIR" && cmake --build --preset release)

if [[ ! -f "$BUILD_DIR/$APP" ]]; then
  echo "[ERROR] Executable not found: $BUILD_DIR/$APP"
  exit 1
fi

echo "[3/6] Preparing package directory..."
rm -rf "$PKG_DIR" "$DEPLOY_DIR/$PKG_NAME.tar.gz"
mkdir -p "$LIB_DIR/plugins" "$LIB_DIR/qml" "$PKG_DIR/fotos"
cp "$BUILD_DIR/$APP" "$PKG_DIR/$APP"

echo "[4/6] Copying Qt plugins and QML modules..."
# Only what the application uses: the xcb platform (plus offscreen for
# diagnostics), SVG and JPEG images, the SQLite driver and the input contexts.
copy_plugin() {
  local rel="$1"
  mkdir -p "$LIB_DIR/plugins/$(dirname "$rel")"
  cp -L "$QT_PLUGINS/$rel" "$LIB_DIR/plugins/$rel"
}
for rel in platforms/libqxcb.so platforms/libqoffscreen.so \
           platforminputcontexts/libcomposeplatforminputcontextplugin.so \
           platforminputcontexts/libibusplatforminputcontextplugin.so \
           imageformats/libqjpeg.so imageformats/libqsvg.so iconengines/libqsvgicon.so \
           sqldrivers/libqsqlite.so; do
  copy_plugin "$rel"
done
cp -aL "$QT_PLUGINS/xcbglintegrations" "$LIB_DIR/plugins/"

# QtQuick: root module files plus the sub-modules the views import (the
# application selects the Basic style, so the other Controls styles stay out).
# Dialogs brings the photo file chooser; without a platform theme it is the
# Qt Quick one (Dialogs/quickimpl), which needs Qt.labs.folderlistmodel.
cp -a "$QT_QML/QtQml" "$LIB_DIR/qml/"
mkdir -p "$LIB_DIR/qml/QtQuick/Controls"
find "$QT_QML/QtQuick" -maxdepth 1 -type f -exec cp -aL {} "$LIB_DIR/qml/QtQuick/" \;
for sub in Dialogs Layouts Templates Window; do
  cp -a "$QT_QML/QtQuick/$sub" "$LIB_DIR/qml/QtQuick/"
done
find "$QT_QML/QtQuick/Controls" -maxdepth 1 -type f -exec cp -aL {} "$LIB_DIR/qml/QtQuick/Controls/" \;
for sub in Basic impl; do
  cp -a "$QT_QML/QtQuick/Controls/$sub" "$LIB_DIR/qml/QtQuick/Controls/"
done
mkdir -p "$LIB_DIR/qml/Qt/labs"
cp -a "$QT_QML/Qt/labs/folderlistmodel" "$LIB_DIR/qml/Qt/labs/"

cat > "$PKG_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = .
Libraries = lib
Plugins = lib/plugins
QmlImports = lib/qml
EOF

echo "[5/6] Bundling shared libraries (ldd)..."
# ldd runs on the originals so Qt kits that locate their libraries through
# their own RUNPATH resolve too. ldd is transitive: one pass covers everything.
deps_file="$(mktemp)"
trap 'rm -f "$deps_file"' EXIT
collect_deps() {
  local out
  out="$(ldd "$1")"
  if grep -q 'not found' <<<"$out"; then
    echo "[ERROR] Unresolved dependency for $1:"
    grep 'not found' <<<"$out"
    exit 1
  fi
  awk '/=>/ && $3 ~ /^\// {print $1, $3}' <<<"$out" >>"$deps_file"
}
collect_deps "$BUILD_DIR/$APP"
while IFS= read -r so; do
  rel="${so#"$LIB_DIR/plugins/"}"
  collect_deps "$QT_PLUGINS/$rel"
done < <(find "$LIB_DIR/plugins" -name '*.so')
while IFS= read -r so; do
  rel="${so#"$LIB_DIR/qml/"}"
  collect_deps "$QT_QML/$rel"
done < <(find "$LIB_DIR/qml" -name '*.so')

bundled=0
while read -r soname path; do
  if [[ "$soname" =~ $SYSTEM_LIBS ]]; then
    continue
  fi
  if [[ ! -f "$LIB_DIR/$soname" ]]; then
    cp -L "$path" "$LIB_DIR/$soname"
    bundled=$((bundled + 1))
  fi
done < <(sort -u "$deps_file")
echo "      $bundled libraries bundled"

# Make every ELF object look for its libraries in lib/, wherever it sits.
set_runpath() {
  local file="$1" dir rel
  dir="$(dirname "$file")"
  rel="$(realpath --relative-to="$dir" "$LIB_DIR")"
  if [[ "$rel" == "." ]]; then
    patchelf --set-rpath '$ORIGIN' "$file"
  else
    patchelf --set-rpath "\$ORIGIN/$rel" "$file"
  fi
}
patchelf --set-rpath '$ORIGIN/lib' "$PKG_DIR/$APP"
while IFS= read -r so; do
  set_runpath "$so"
done < <(find "$LIB_DIR" -type f \( -name '*.so' -o -name '*.so.*' \))

echo "[6/6] Creating archive..."
if [[ -f "$SCRIPT_DIR/packaging/linux/LEEME-linux.txt" ]]; then
  cp "$SCRIPT_DIR/packaging/linux/LEEME-linux.txt" "$PKG_DIR/LEEME-linux.txt"
else
  echo "[WARN] packaging/linux/LEEME-linux.txt not found; the package has no instructions."
fi
find "$PKG_DIR" -type d -exec chmod 755 {} +
find "$PKG_DIR" -type f -exec chmod 644 {} +
chmod 755 "$PKG_DIR/$APP"
tar -C "$DEPLOY_DIR" --owner=0 --group=0 --numeric-owner -czf "$DEPLOY_DIR/$PKG_NAME.tar.gz" "$PKG_NAME"

# Minimum glibc: the highest GLIBC symbol version required by anything shipped.
min_glibc="$(find "$PKG_DIR" -type f \( -name '*.so' -o -name '*.so.*' -o -name "$APP" \) -print0 \
  | xargs -0 -n1 objdump -T 2>/dev/null | grep -o 'GLIBC_[0-9.]*' | sort -V | tail -1)"

echo ""
echo "Package created at: $DEPLOY_DIR/$PKG_NAME.tar.gz"
echo "Directory:          $PKG_DIR"
echo "Qt used:            $("$QMAKE" -query QT_VERSION) ($("$QMAKE" -query QT_INSTALL_PREFIX))"
echo "Requires:           ${min_glibc:-unknown} or newer on the target system"
