#!/bin/bash
#
# Local build script for XCalbuild
# Assuming local installation of build tools and vcpkg.
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $REPO_ROOT
if [ ! -d build ];
then
    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_EXE_LINKER_FLAGS="-static" \
        -DCMAKE_BUILD_TYPE=debug \
        ..
else
    cd build
fi

PWD=`pwd`
BUILD_TRACER=$PWD/build-tracer
LIBEAR=$PWD/libear.so

if [ -f "$BUILD_TRACER" ]; then
    # PoP, also to generate compile_commands.json for intellisense.
    $BUILD_TRACER -b $LIBEAR -c clang-10,clang++-10 -- make -j4
else
    make -j4
fi

cp src/linux/bear/src/build-tracer .
cp src/linux/bear/src/libear.so .
chmod a+rwx build-tracer
chmod a+rwx libear.so
cp src/xcalbuild/xcalbuild .
