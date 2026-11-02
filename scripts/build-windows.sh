#!/usr/bin/env bash
# Builds the self-contained executable for 64-bit Windows (README "Windows build"):
# Gambasse from the windows preset (CMakePresets.json), cross-built on Linux with Qt,
# its plugins and QML modules and the C++ runtime linked into the executable, which is
# copied without symbols to build-windows/dist/Gambasse.exe. Then checks that the
# executable imports only DLLs of Windows: it fails, naming them, on any DLL that is
# not in the list below. Last, when Wine is installed, a smoke test in an empty
# directory and a temporary Wine prefix, with no display: --check-db creates and
# migrates a database (the SQLite driver and the embedded patches).
#
#   scripts/build-windows.sh                       build, copy, check and try it
#   scripts/build-windows.sh --check <executable>  only check <executable>
#
# On demand only: the native build, the tests, the Linux package and the CI do not
# need it. The cross compiler and Qt for Windows are toolchain-cpp's gcc-mingw and
# qt-mingw components, installed first (../toolchain-cpp/install.sh gcc-mingw qt-mingw),
# with the desktop kit (qt) as the host of Qt's tools.
#
# Variables:
#   SOFTWARE_DIR   toolchain-cpp's installation directory  [../toolchain-cpp/software]
#   GCC_VERSION    release of the cross compiler           [16.2.0]
#   QT_VERSION     Qt release                              [6.12.0]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SOFTWARE_DIR="${SOFTWARE_DIR:-$ROOT/../toolchain-cpp/software}"
GCC_VERSION="${GCC_VERSION:-16.2.0}"
QT_VERSION="${QT_VERSION:-6.12.0}"

TOOLS="$SOFTWARE_DIR/gcc-$GCC_VERSION-mingw/bin/x86_64-w64-mingw32"

# The DLLs of Windows the executable may import, in lower case: all of them in
# System32 of Windows 10 1809, the oldest Windows Qt 6.12 supports, with the first
# Windows that has them according to Microsoft's API documentation. The same list as
# guita's. A DLL that is not here fails the check until it is reviewed and added
WINDOWS_DLLS=(
    # The kernel, the user interface and the shell (Windows 2000 / XP)
    kernel32.dll ntdll.dll user32.dll gdi32.dll advapi32.dll shell32.dll shlwapi.dll
    ole32.dll oleaut32.dll comdlg32.dll imm32.dll version.dll winmm.dll setupapi.dll
    userenv.dll wtsapi32.dll netapi32.dll authz.dll dbghelp.dll mscms.dll
    # Theming and the desktop window manager (XP, Vista), DPI awareness (8.1)
    uxtheme.dll dwmapi.dll shcore.dll
    # Graphics: OpenGL (Windows 95), Direct3D 9 (XP), DXGI (Vista), Direct3D 11 and
    # DirectWrite (7), Direct3D 12 (10)
    opengl32.dll d3d9.dll dxgi.dll d3d11.dll dwrite.dll d3d12.dll
    # Network: Winsock (95, 2000), IP helper (2000), WinHTTP (2000 SP3)
    ws2_32.dll wsock32.dll iphlpapi.dll winhttp.dll
    # Security and TLS: SSPI and Schannel (2000), certificates (2000), CNG (Vista)
    secur32.dll crypt32.dll bcrypt.dll ncrypt.dll
    # API sets: WinRT strings and activation, and WaitOnAddress (8)
    api-ms-win-core-winrt-l1-1-0.dll api-ms-win-core-winrt-string-l1-1-0.dll
    api-ms-win-core-synch-l1-2-0.dll
    # The Universal C Runtime, part of Windows since 10
    ucrtbase.dll
)

usage() {
    echo "usage: $0 [--check <executable>]" >&2
    exit 2
}

# allowed <dll>: whether <dll> is in WINDOWS_DLLS or is an API set of the UCRT
allowed() {
    local dll="${1,,}" known
    [[ "$dll" == api-ms-win-crt-*.dll ]] && return 0
    for known in "${WINDOWS_DLLS[@]}"; do
        [[ "$dll" == "$known" ]] && return 0
    done
    return 1
}

# check <executable>: the DLLs it imports (objdump -p of the cross compiler), each of
# them in the list
check() {
    local exe="$1" dlls problems=() dll
    [[ -x "$TOOLS-objdump" ]] ||
        { echo "error: no cross objdump in $TOOLS-objdump: run ../toolchain-cpp/install.sh gcc-mingw" >&2; return 1; }
    dlls="$(LC_ALL=C "$TOOLS-objdump" -p "$exe" | awk '/DLL Name:/ { print $3 }' | sort -fu)"
    [[ -n "$dlls" ]] || { echo "error: $exe imports no DLL: is it a Windows executable?" >&2; return 1; }
    while read -r dll; do
        allowed "$dll" || problems+=("$dll")
    done <<<"$dlls"

    if ((${#problems[@]})); then
        echo "error: $exe imports DLLs that are not of Windows:" >&2
        printf '  %s\n' "${problems[@]}" >&2
        return 1
    fi
    echo ">> $exe imports only these DLLs of Windows:"
    # shellcheck disable=SC2001 # indents every line of a multi-line text
    sed 's/^/     /' <<<"$dlls"
}

# smoke_test <executable>: under Wine, if installed, a copy in an empty directory, with
# a temporary prefix and home (never the user's) and the offscreen platform. The output
# goes to a pipe, so the application writes it there instead of to a console
smoke_test() {
    local exe="$1" tmp dir out status=0
    if ! command -v wine >/dev/null 2>&1; then
        echo "warning: wine is not installed: the smoke test was skipped" >&2
        return 0
    fi
    tmp="$(mktemp -d)"
    dir="$tmp/app"
    mkdir -p "$dir" "$tmp/home"
    cp "$exe" "$dir/Gambasse.exe"
    # No Mono or Gecko: a new prefix would offer to install them
    out="$(env -i PATH="$PATH" HOME="$tmp/home" WINEPREFIX="$tmp/prefix" WINEDEBUG=-all \
        WINEDLLOVERRIDES="mscoree,mshtml=" QT_QPA_PLATFORM=offscreen \
        wine "$dir/Gambasse.exe" --check-db 2>&1)" || true
    if [[ "$out" != *"PATIENTS=0"* || ! -f "$dir/database.db" ]]; then
        echo "error: --check-db under Wine did not create the database:" >&2
        # shellcheck disable=SC2001 # indents every line of a multi-line text
        sed 's/^/  /' <<<"$out" >&2
        status=1
    fi
    env -i PATH="$PATH" HOME="$tmp/home" WINEPREFIX="$tmp/prefix" wineserver -k 2>/dev/null || true
    env -i PATH="$PATH" HOME="$tmp/home" WINEPREFIX="$tmp/prefix" wineserver -w 2>/dev/null || true
    rm -rf "$tmp"
    ((status == 0)) || return "$status"
    echo ">> Smoke test under Wine passed: database created and migrated"
}

case "${1:-}" in
    "") [[ $# -eq 0 ]] || usage ;;
    --check) [[ $# -eq 2 ]] || usage; check "$2"; exit ;;
    *) usage ;;
esac

# The components for Windows of toolchain-cpp and the host kit, by files each installs,
# among them the modules and plugins Gambasse takes from the kit for Windows
missing=()
for needed in "gcc-$GCC_VERSION-mingw/toolchain.cmake" \
    "Qt/$QT_VERSION-mingw/lib/cmake/Qt6/qt.toolchain.cmake" \
    "Qt/$QT_VERSION-mingw/lib/cmake/Qt6Sql" \
    "Qt/$QT_VERSION-mingw/plugins/sqldrivers/libqsqlite.a" \
    "Qt/$QT_VERSION-mingw/plugins/platforms/libqwindows.a" \
    "Qt/$QT_VERSION-mingw/lib/cmake/Qt6Svg" \
    "Qt/$QT_VERSION-mingw/lib/cmake/Qt6Quick" \
    "Qt/$QT_VERSION/gcc_64/bin/qmake"; do
    [[ -e "$SOFTWARE_DIR/$needed" ]] || missing+=("$SOFTWARE_DIR/$needed")
done
if ((${#missing[@]})); then
    echo "error: the components for Windows of toolchain-cpp are not installed in" \
         "$SOFTWARE_DIR; missing:" >&2
    printf '  %s\n' "${missing[@]}" >&2
    echo "install them with: ../toolchain-cpp/install.sh gcc qt gcc-mingw qt-mingw" >&2
    exit 1
fi

cd "$ROOT"
# --fresh: a cache from another compiler or Qt would win over the preset
cmake --preset windows --fresh
cmake --build --preset windows

DIST="$ROOT/build-windows/dist"
mkdir -p "$DIST"
cp build-windows/Gambasse.exe "$DIST/Gambasse.exe"
"$TOOLS-strip" "$DIST/Gambasse.exe"
check "$DIST/Gambasse.exe"
smoke_test "$DIST/Gambasse.exe"
echo ">> Self-contained Windows executable in $DIST/Gambasse.exe ($(du -h "$DIST/Gambasse.exe" | cut -f1))"
