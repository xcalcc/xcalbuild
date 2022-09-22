#!/bin/bash
#
# Dev build script for XCalbuild
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
REPO_ROOT=$SCRIPT_DIR/../../../

cd $SCRIPT_DIR
SCRIPT_HASH=`sha1sum build.sh | awk '{ print substr($1, 0, 7); }'`
DOCKER_HASH=`sha1sum Dockerfile | awk '{ print substr($1, 0, 7); }'`
DOCKER_TAG="xcalbuild-build-$DOCKER_HASH-$SCRIPT_HASH"

echo "Running as user: $USER ($UID) on host $HOSTNAME with shell $SHELL"
echo "Checker for Docker tag $DOCKER_TAG"
if ! docker image ls | grep $DOCKER_TAG; then
    echo "Building Docker $DOCKER_TAG"
    docker build . -t "$DOCKER_TAG"
fi

build_cmds="bash scripts/build/lin/build.sh -dev"

docker run --rm \
    --user $UID \
    -v $REPO_ROOT:/home/xcalbuild \
    -w /home/xcalbuild \
    $DOCKER_TAG \
    ${build_cmds}
