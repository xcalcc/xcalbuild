#!/bin/bash
#
# Build script for xcalbuild
#
# Should be run inside the build environment Docker container, not locally
#
# If you are looking for a script to run, you likely want
# ci-build.sh.
#
# Author: Dean McGregor
# Date: 17 Jun 2020

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $REPO_ROOT
if [ "$1" = "-dev" ];
then
    if [ ! -d build ];
    then
        mkdir build
        cd build
        cmake -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
            -DBUILD_SHARED_LIBS=OFF -DCMAKE_EXE_LINKER_FLAGS="-static" \
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
    cmake -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_EXE_LINKER_FLAGS="-static" \
        -DCMAKE_INSTALL_PREFIX=install \
        ..
fi

PWD=`pwd`
BUILD_TRACER=$PWD/install/bin/unix-tracer
LIBEAR=$PWD/install/lib64/libtracer-preload.so

if [ "$1" = "-dev" ];
then
	make install -j4
else
	make install
fi
