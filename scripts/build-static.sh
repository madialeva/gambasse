#!/usr/bin/env bash
# Builds the self-contained Linux executable (README "Static build"): Gambasse from
# the static preset (CMakePresets.json), with Qt, its plugins and QML modules and the
# C++ runtime linked into the executable, which is copied without symbols to
# build-static/dist/Gambasse. Then checks that the executable needs only libraries of
# the operating system: it fails, naming them, if it carries a run path, or needs a
# shared library of Qt or the C++ runtime, or one from the local dependencies or
# this repository. Last, a smoke test in an empty directory and environment, with
# no display: --check-db creates and migrates a database (the SQLite driver and the
# embedded patches) and --svg2png renders the logo (the SVG plugin).
#
#   scripts/build-static.sh                       build, copy, check and try it
#   scripts/build-static.sh --check <executable>  only check <executable>
#
# On demand only: the native build, the tests, the Linux package and the CI do not
# need it. The static Qt comes from scripts/build-static-deps.sh, which runs first.
#
# Variables:
#   SOFTWARE_DIR   root of the local dependencies  [~/software]
#   QT_VERSION     static Qt release               [6.12.0]
set -euo pipefail

SOFTWARE_DIR="${SOFTWARE_DIR:-$HOME/software}"
QT_VERSION="${QT_VERSION:-6.12.0}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

usage() {
    echo "usage: $0 [--check <executable>]" >&2
    exit 2
}

# check <executable>: the run path and the shared libraries it needs, directly
# (readelf) and once resolved (ldd)
check() {
    local exe="$1" problems=() dynamic needed lib line path
    dynamic="$(LC_ALL=C readelf -d "$exe")"
    while read -r line; do
        [[ -n "$line" ]] && problems+=("$line")
    done < <(sed -n -E 's/.*\((RUNPATH|RPATH)\).*\[(.*)\]/\1 \2/p' <<<"$dynamic")
    needed="$(sed -n -E 's/.*\(NEEDED\).*\[(.*)\]/\1/p' <<<"$dynamic")"
    while read -r lib; do
        [[ "$lib" =~ ^lib(Qt6|icu|stdc\+\+|gcc_s) ]] && problems+=("needs $lib")
    done <<<"$needed"
    while read -r line; do
        lib="${line%% *}"
        if [[ "$line" == *"=> not found"* ]]; then
            problems+=("$lib not found")
        elif [[ "$line" == *" => /"* ]]; then
            path="${line#* => }"
            path="${path%% (*}"
            [[ "$path" == "$SOFTWARE_DIR"/* || "$path" == "$ROOT"/* ]] && problems+=("$lib from $path")
        fi
    done < <(LC_ALL=C ldd "$exe" | sed 's/^[[:space:]]*//')

    if ((${#problems[@]})); then
        echo "error: $exe is not self-contained:" >&2
        printf '  %s\n' "${problems[@]}" >&2
        return 1
    fi
    echo ">> $exe needs only these libraries of the system:"
    sed 's/^/     /' <<<"$needed"
}

# smoke_test <executable>: a copy in an empty directory, with an empty environment
# and the offscreen platform
smoke_test() {
    local exe="$1" dir out
    dir="$(mktemp -d)"
    trap 'rm -rf "$dir"' RETURN
    cp "$exe" "$dir/Gambasse"
    out="$(env -i QT_QPA_PLATFORM=offscreen "$dir/Gambasse" --check-db 2>&1)" || true
    if [[ "$out" != *"PATIENTS=0"* || ! -f "$dir/database.db" ]]; then
        echo "error: --check-db did not create the database:" >&2
        sed 's/^/  /' <<<"$out" >&2
        return 1
    fi
    out="$(env -i QT_QPA_PLATFORM=offscreen "$dir/Gambasse" --svg2png :/img/logo.svg "$dir/logo.png" 64 2>&1)" || true
    if [[ ! -s "$dir/logo.png" ]]; then
        echo "error: --svg2png did not render the logo:" >&2
        sed 's/^/  /' <<<"$out" >&2
        return 1
    fi
    echo ">> Smoke test passed: database created and migrated, SVG rendered"
}

case "${1:-}" in
    "") [[ $# -eq 0 ]] || usage ;;
    --check) [[ $# -eq 2 ]] || usage; check "$2"; exit ;;
    *) usage ;;
esac

# The modules Gambasse takes from the shared kit (scripts/build-static-deps.sh)
qt_dir="$SOFTWARE_DIR/Qt/$QT_VERSION-static"
for marker in lib/cmake/Qt6Sql plugins/sqldrivers/libqsqlite.a lib/cmake/Qt6Svg lib/cmake/Qt6Quick; do
    if [[ ! -e "$qt_dir/$marker" ]]; then
        echo "error: the static Qt in $qt_dir is missing or lacks $marker: run" \
             "scripts/build-static-deps.sh first" >&2
        exit 1
    fi
done

cd "$ROOT"
# --fresh: a cache from another compiler or Qt would win over the preset
cmake --preset static --fresh
cmake --build --preset static

DIST="$ROOT/build-static/dist"
mkdir -p "$DIST"
cp build-static/Gambasse "$DIST/Gambasse"
strip "$DIST/Gambasse"
check "$DIST/Gambasse"
smoke_test "$DIST/Gambasse"
echo ">> Self-contained executable in $DIST/Gambasse ($(du -h "$DIST/Gambasse" | cut -f1))"
