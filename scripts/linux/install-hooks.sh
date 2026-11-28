#!/bin/sh
# Install the config-drift warning as post-merge and post-checkout hooks.
# Hooks go in the shared hooks dir, so they cover every worktree. A hook that
# this script did not write is left alone.
set -eu

source="$(cd "$(dirname "$0")" && pwd)/hooks/keyflow-config-drift"
hooks="$(git rev-parse --path-format=absolute --git-common-dir)/hooks"
hooks="$(git config --get core.hooksPath || echo "$hooks")"
mkdir -p "$hooks"

for name in post-merge post-checkout; do
    target="$hooks/$name"
    if [ -e "$target" ] && ! grep -q 'keyflow-config-drift' "$target"; then
        echo "skip $target: an existing hook is there; add a call to $source yourself" >&2
        continue
    fi
    cp "$source" "$target"
    chmod 0755 "$target"
    echo "installed $target"
done
