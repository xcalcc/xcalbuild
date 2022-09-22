#!/bin/bash

rm -rf out
mkdir out

$1 -i $2 -o out --debug -- make

mkdir -p out/preprocess
tar -C out/preprocess -xzf out/preprocess.tar.gz

# Make sure default language std is passed.
grep "a\.h" out/source_files.json
grep "b\.h" out/source_files.json
grep -V "<built-in>" out/source_files.json
