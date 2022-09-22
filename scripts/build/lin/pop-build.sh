#!/bin/bash
#
# PoP build script for xcalbuild

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $REPO_ROOT/build

PWD=`pwd`
XCALBUILD=$PWD/install/bin/xcalbuild

mkdir pop-output

${XCALBUILD} -i . -o pop-output -p "make clean" --profile clang -- make
