#!/bin/bash

rm -rf out
mkdir out

$1 -i $2 -o out --debug --profile gnu-cc -- make

mkdir -p out/preprocess
tar -C out/preprocess -xzf out/preprocess.tar.gz

# Make sure default language std is passed.
grep "c_scan_options=-std" out/preprocess/a.out.dir/xcalibyte.properties
grep "cxx_scan_options=-std" out/preprocess/a.out.dir/xcalibyte.properties
