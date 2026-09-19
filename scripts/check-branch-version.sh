#!/usr/bin/env sh
# Verify that the current branch's version suffix matches the project version.
# Only develop/<version> and release/<version> branches are checked; other
# branches (for example change/is<n>-<slug>) are ignored.
set -eu

branch="${1:-}"
case "$branch" in
    develop/*) expected="${branch#develop/}" ;;
    release/*) expected="${branch#release/}" ;;
    *)
        echo "Not a develop/ or release/ branch ('$branch'); skipping version check."
        exit 0
        ;;
esac
expected="${expected#v}"

actual="$(grep -oE 'project\(Gambasse VERSION [0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt \
    | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')"

if [ "$expected" != "$actual" ]; then
    echo "Branch version '$expected' does not match PROJECT_VERSION '$actual'." >&2
    exit 1
fi

echo "Branch version matches PROJECT_VERSION ($actual)."
