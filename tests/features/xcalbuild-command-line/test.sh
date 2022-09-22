#!/bin/bash

rm -rf bin
mkdir bin
ln -s $1 bin/xcalbuild
cp -R $2/profiles .

EC_BAD_CMD_LINE=2
EC_BAD_CONFIG=4
EC_BAD_PROFILE=5

bin/xcalbuild --help
bin/xcalbuild --version

echo "Bad cmd line."
EC=`$1 || echo $?`
if [[ "${EC}" -ne "${EC_BAD_CMD_LINE}" ]]; then
    exit 1
fi

EC=`$1 -i . || echo $?`
if [[ "${EC}" -ne "${EC_BAD_CMD_LINE}" ]]; then
    exit 1
fi

EC=`$1 -i . -o . || echo $?`
if [[ "${EC}" -ne "${EC_BAD_CMD_LINE}" ]]; then
    exit 1
fi

EC=`bin/xcalbuild -i . -o . -x -- ls || echo $?`
if [[ "${EC}" -ne "${EC_BAD_CMD_LINE}" ]]; then
    exit 1
fi

EC=`bin/xcalbuild -i . -o . -- ls || echo $?`
if [[ "${EC}" -ne "${EC_BAD_CONFIG}" ]]; then
    exit 1
fi

echo "Bad profile."
touch config
EC=`bin/xcalbuild -i . -o . --profile gnu-cc -- ls || echo $?`
rm -rf config
if [[ "${EC}" -ne "${EC_BAD_PROFILE}" ]]; then
    exit 1
fi
