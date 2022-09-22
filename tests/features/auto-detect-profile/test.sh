#!/bin/bash

rm -rf out
mkdir out

# Fully matched GNU
$1 -i $2 -o out --debug --local_log -- make target_gnu
grep "To use fully matched toolchain profile: gnu-cc" out/xcalbuild.log

# Fully matched clang
$1 -i $2 -o out --debug --local_log -- make target_clang
grep "To use fully matched toolchain profile: clang" out/xcalbuild.log

# Fully matched arm
$1 -i $2 -o out --debug --local_log -- make target_arm
grep "To use fully matched toolchain profile: arm-gcc" out/xcalbuild.log

# Partially matched
$1 -i $2 -o out --debug --local_log -- make target_partial
grep "To use the best partially matched (3/4) toolchain profile: clang" out/xcalbuild.log
