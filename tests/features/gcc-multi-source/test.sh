#!/bin/bash

rm -rf out
mkdir out

# Note that using gnu-cc in this case is actually not correct,
# just to test multi-file handling.
$1 -i $2 -o out --debug --profile gnu-cc -- make

mkdir -p out/preprocess
tar -C out/preprocess -xzf out/preprocess.tar.gz

EC=0

if [[ "`find out/preprocess -name *.i | wc -l`" -ne "1" ]]; then
    echo ".i file count mismatch"
    EC=1
fi

if [[ "`find out/preprocess -name *.ii | wc -l`" -ne "1" ]]; then
    echo ".ii file count mismatch"
    EC=1
fi

if [[ "$EC" -eq "0" ]]; then
    echo "passed"
    rm -rf a.out out
fi

exit $EC