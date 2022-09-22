#!/bin/bash

EXIT_FAILURE=1
EXIT_TEST_FAILURE=201

echo "Bad command lines."
EC=`$1 -- -x || echo $?`
if [[ "${EC}" -ne "${EXIT_FAILURE}" ]]; then
    exit 1
fi

EC=`$1 -- -p $2/profile.json || echo $?`
if [[ "${EC}" -ne "${EXIT_FAILURE}" ]]; then
    exit 1
fi

EC=`$1 -- -p $2/xxx -t $2/tests.json || echo $?`
if [[ "${EC}" -ne "${EXIT_FAILURE}" ]]; then
    exit 1
fi

EC=`$1 -- -p $2/profile.json -t $2/xxx || echo $?`
if [[ "${EC}" -ne "${EXIT_FAILURE}" ]]; then
    exit 1
fi

echo "Correct invocations."
$1 -- --help
$1 -- --version
$1 -- -p $2/profile.json -c
$1 -- -p $2/profile.json -c -t $2/tests.json
$1 -- -p $2/profile.json -t $2/tests.json

echo "Invalid profile."
EC=`$1 -- -p $2/invalid-profile.json -c || echo $?`
if [[ "${EC}" -ne "${EXIT_TEST_FAILURE}" ]]; then
    echo $EC
    exit 1
fi

echo "Bad tests."
EC=`$1 -- -p $2/profile.json -t $2/bad-tests.json || echo $?`
if [[ "${EC}" -ne "201" ]]; then
    exit 1
fi

echo "Passed."