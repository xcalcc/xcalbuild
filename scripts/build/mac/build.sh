#!/bin/bash
#
# Build script for xcalbuild
#
#
# Author: suitianxiang
# Date: 08 July 2022

#$1 is build version, its value is -release/-dev. $2 is vcpkg path which depends on the vcpkg path installed in the machine. 

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $REPO_ROOT
if [ "$1" = "-dev" ];
then
    if [ ! -d build ];
    then
        mkdir build
        cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=$2/scripts/buildsystems/vcpkg.cmake \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
            -DCMAKE_BUILD_TYPE=debug \
            -DCMAKE_INSTALL_PREFIX=install \
            ..
    else
        cd build
    fi
else
    rm -rf build
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=$2/scripts/buildsystems/vcpkg.cmake \
        -DCMAKE_INSTALL_PREFIX=install \
        ..
fi

if [ "$1" = "-dev" ];
then
	make install -j4
else
	make install
fi
