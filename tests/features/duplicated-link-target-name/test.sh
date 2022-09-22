#!/bin/bash

rm -rf out
mkdir out
$1 -i $2 -o out --debug -p "make clean" -- make

mkdir -p out/preprocess
tar -C out/preprocess -xzf out/preprocess.tar.gz

EC=0

# Check renames
for target in a.lib a.lib.1 a.lib.2 b.lib b.lib.1 c.lib a.out
do
    if [[ -d "out/preprocess/${target}.dir" ]]; then
        echo "got ${target}"
    else
        echo "missing ${target}"
        EC=1
    fi
done

# Check dependencies are correct.
grep "dependencies=a.lib.2 a.lib.1 b.lib.1 a.lib b.lib c.lib" out/preprocess/a.out.dir/xcalibyte.properties || EC=1

if [[ "$EC" -eq "0" ]]; then
    echo "passed"
    rm -rf a.out out
fi

exit $EC