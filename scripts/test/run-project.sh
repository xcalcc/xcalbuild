#!/bin/bash

# This script is design to set-up and run a build and analysis in
# a docker. You are likely not looking for this script if you are
# wanting to run a project from a host machine.

set -e
PROJECT=$1
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TESTS_DIR=${DIR}/../../tests
XCALBUILD="${DIR}/../../build/install/bin/xcalbuild"

cd $TESTS_DIR/projects/$PROJECT/repo
CWD=`pwd`
build_cmd=`cat ../build-cmd`
prebuild_cmd=`cat ../prebuild-cmd`
source ../env.sh
source ../configure.sh

echo "${XCALBUILD} -i . -o ${CWD}/.. -p \"${prebuild_cmd}\" --profile ${PROFILE} --tracing_method ${TRACING} -- ${build_cmd}"
/usr/bin/time -f xcalbuild_time@%E ${XCALBUILD} --debug -i ${CWD} -o ${CWD}/.. -p "${prebuild_cmd}" --profile ${PROFILE} --tracing_method ${TRACING} -- ${build_cmd} 2>&1 | tee ../build.log
