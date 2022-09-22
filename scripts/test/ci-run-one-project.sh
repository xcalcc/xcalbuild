#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TESTS_DIR=${DIR}/../../tests

PROJECT=$1
API_IP=$2
API_PORT=$3
XVSA_IMG=$4

mkdir -p $TESTS_DIR/logs

echo "==============="
echo ${PROJECT}
echo "==============="
echo ""
pushd $TESTS_DIR/projects/${PROJECT}
rm -rf preprocess*
# Load env
source env.sh
# Clone repo
if [[ -d repo ]]; then
    echo "Using existing repo."
    pushd $TESTS_DIR/projects/${PROJECT}/repo
    git reset --hard ${REVISION}
    git clean -fdx
    popd
else
    git clone -n ${REPO_URL} repo
    pushd $TESTS_DIR/projects/${PROJECT}/repo
    git checkout ${REVISION}
    popd
fi
# Run build/pp
#docker build -t ${PROJECT} .
if [ -f ./image.conf ]; then
	TESTENV_IMG=`cat ./image.conf`
	docker run -u $UID -e USER=scripts -e HOME=/work/tests/projects/${PROJECT} --cap-add sys_ptrace --rm --volume $DIR/../../:/work -w /work/tests/projects/${PROJECT}/repo ${TESTENV_IMG} bash /work/scripts/test/run-project.sh ${PROJECT} 2>&1 | tee ${TESTS_DIR}/logs/${PROJECT}-build.log
	# Run xvsa
	${DIR}/analyse-preprocessed.sh ${TESTS_DIR}/projects/${PROJECT} $API_IP $API_PORT $XVSA_IMG
	# Copy other related logs
	cp -r .scan_log $TESTS_DIR/logs/${PROJECT}-scan-log
	cp xcalbuild.log $TESTS_DIR/logs/${PROJECT}-xcalbuild.log || echo "No xcalbuild.log"
	cp -r scan_result $TESTS_DIR/logs/${PROJECT}-scan-result || echo "No analysis result"
	popd
fi
