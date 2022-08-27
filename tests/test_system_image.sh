#!/bin/bash -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BASEDIR=$(dirname ${SCRIPT_DIR})

mkdir -p ${BASEDIR}/build/roots
mkdir -p ${BASEDIR}/build/data
function pkgupd() {
    echo "executing ${@}"
    DEBUG=1 ${BASEDIR}/build/pkgupd ${@}    \
        dir.build=${BASEDIR}/build/temp     \
        dir.pkgs=${BASEDIR}/build/pkgs      \
        dir.src=${BASEDIR}/build/src        \
        dir.root=${BASEDIR}/build/roots     \
        dir.data=${BASEDIR}/build/data      \
        dir.repo=${BASEDIR}/build/repo      \
        mirrors=https://storage.rlxos.dev/testing, 
}

pkgupd sync

pkgupd build ${BASEDIR}/tests/rlxos-core.yml
[ -e ${BASEDIR}/build/pkgs/testing/rlxos-core-2200.pkg ] || {
    exit 1
}