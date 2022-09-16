#!/bin/sh

workspaceFolder="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

${workspaceFolder}/build/pkgupd  $@             \
    dir.root=${workspaceFolder}/build/root/     \
    dir.data=${workspaceFolder}/build/data/     \
    dir.pkgs=${workspaceFolder}/build/pkgs/     \
    dir.cache=${workspaceFolder}/build/cache    \
    dir.repo=${workspaceFolder}/build/repo/     \
    mirors=https://storage.rlxos.dev/testing