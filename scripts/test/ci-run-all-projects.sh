#!/bin/bash

API_IP=$1
API_PORT=$2
XVSA_IMG=$3

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TESTS_DIR=${DIR}/../../tests
ALL_PROJECTS=`ls ${TESTS_DIR}/projects`

mkdir -p $TESTS_DIR/logs
for project in $ALL_PROJECTS; do
    ${DIR}/ci-run-one-project.sh ${project} $API_IP $API_PORT $XVSA_IMG || echo "Failed to run ${project}."
done
