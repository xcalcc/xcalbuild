#!/bin/bash

# This requires to have a properly configured build dir.

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

cd $DIR/../../build
PATH=$PATH:`pwd`/install/bin
ctest -V

if [ "$1" = "--clang" ];
then
    COV_EXEC=--gcov-executable="llvm-cov-10 gcov"
else
    COV_EXEC=
fi
# exclude taskflow and all test files except for copmiler-profile-tester.
gcovr -r ../ . -e ../src/include/taskflow -e '../src/.+/.*test\.cpp' $COV_EXEC
