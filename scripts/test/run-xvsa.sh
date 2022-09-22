#!/bin/bash
set -e

# This script requires a running Xcalscan docker stack on the following IP.
# Make sure this not a loopback.
# Preferabbly not on the same machine to avoid interferering with performance testing.

# The server is running on sony.
API_IP=$2
API_PORT=$3

# use server username and password
USERNAME=xx
PASSWORD=xx
#XVSA_IMG=sony:5000/xcal.xvsa:1.1
XVSA_IMG=$4

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

TOKEN="$( python3 ${DIR}/get-token.py -i http://${API_IP}:${API_PORT} -u ${USERNAME} -p ${PASSWORD} )"
#echo $TOKEN

WD=`dirname $1`
PP_TGZ=`basename $1`
PP_INPUT=${PP_TGZ##*.}

cd ${WD}
CWD=`pwd`

rm -rf preprocess
mkdir preprocess
cd preprocess && tar -xzf ../${PP_TGZ} && cd -
/usr/bin/time -f xvsa_time@%E docker run -u $UID --rm -v ${CWD}:/scandata -w /scandata ${XVSA_IMG} xvsa_scan preprocess -token "${TOKEN}" -server ${API_IP}@${API_PORT} || echo "Analysis Failed."
