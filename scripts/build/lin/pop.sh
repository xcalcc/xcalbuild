#!/bin/bash
#
# Product on Product
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $SCRIPT_DIR
SCRIPT_HASH=`sha1sum build.sh | awk '{ print substr($1, 0, 7); }'`
DOCKER_HASH=`sha1sum Dockerfile | awk '{ print substr($1, 0, 7); }'`
DOCKER_TAG="xcalbuild-build-$DOCKER_HASH-$SCRIPT_HASH"

echo "Running as user: $USER ($UID) on host $HOSTNAME with shell $SHELL"

build_cmds="bash scripts/build/lin/pop-build.sh"

docker run --rm \
    --user $UID \
    -v $REPO_ROOT:/home/xcalbuild \
    -w /home/xcalbuild \
    $DOCKER_TAG \
    ${build_cmds}