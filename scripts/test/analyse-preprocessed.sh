#!/bin/bash

API_IP=$2
API_PORT=$3
XVSA_IMG=$4

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

project_dir=$1
project=`basename ${project_dir}`

source ${project_dir}/env.sh

# Run xvsa
${DIR}/run-xvsa.sh ${project_dir}/preprocess.tar.gz $API_IP $API_PORT $XVSA_IMG 2>&1 | tee ${project_dir}/../../logs/${project}-analysis.log
