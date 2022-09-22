pipeline {
    agent {
          node {
            label "${AGENT}"
          }
        }
    environment {
        WORKDIR=pwd()
        DATETIME = sh(returnStdout: true, script: 'date +%Y-%m-%d').trim()
        UID=sh(script: "id -u ${USER}", returnStdout: true).trim()
        ID="$XCALBUILD_BRANCH-$DATETIME"
        FILE_NAME="xcalbuild-linux-${ID}.zip"
        ARTIFACTS_PATH="${WORKDIR}/artifacts/"
        BUILD_SCRIPT_DIR="${WORKDIR}/script/build/lin/"
        DOCKER_TAG="hub.xcalibyte.co/sdlc/xcalbuildbuilder:1.0"
        XCALBUILD_NEXUS_REPO_ADDRESS_SH="${XCALBUILD_NEXUS_REPO_ADDRESS_SH ? XCALBUILD_NEXUS_REPO_ADDRESS_SH : 'http://10.10.4.154:8081/repository/xcalbuild/'}"
        XCALBUILD_NEXUS_REPO_ADDRESS_SZ="${XCALBUILD_NEXUS_REPO_ADDRESS_SZ ? XCALBUILD_NEXUS_REPO_ADDRESS_SZ : 'http://10.10.2.141:8081/repository/xcalbuild/'}"
        BUILD_SCRIPT="bash scripts/build/lin/build.sh"
    }

    stages {
        stage('build xcalbuild') {
            steps {
                sh 'cd ${SCRIPT_DIR}'
                sh 'echo "Running as user: $USER ($UID) on host $HOSTNAME with shell $SHELL"'
                sh 'echo "Checker for Docker tag $DOCKER_TAG"'
                sh 'echo "Current branch is $(git rev-parse --abbrev-ref HEAD)"'

                sh '''
                    docker run --rm \
                      --user $UID \
                      -v ${WORKDIR}:/home/xcalbuild \
                      -w /home/xcalbuild \
                      ${DOCKER_TAG} \
                      ${BUILD_SCRIPT}
                '''
            }
        }
        stage('package xcalbuild') {
            steps {
                sh '''
                    cd "${WORKDIR}/build/install"
                    echo "current path $(pwd)"
                    zip -r ${FILE_NAME} *
                    mkdir -p ${ARTIFACTS_PATH}
                    cp ${FILE_NAME} ${ARTIFACTS_PATH}${FILE_NAME}
                '''
            }
        }
    }

    post ('push to nexus') {
        success {
            sh'''#!/bin/bash
                set +e
                echo "sending package to $XCALBUILD_NEXUS_REPO_ADDRESS_SH"
                curl -v -u $NEXUS_REPO_USER:$NEXUS_REPO_PSW --upload-file "${ARTIFACTS_PATH}${FILE_NAME}" $XCALBUILD_NEXUS_REPO_ADDRESS_SH
                echo "sending package to $XCALBUILD_NEXUS_REPO_ADDRESS_SH"
                curl -v -u $NEXUS_REPO_USER:$NEXUS_REPO_PSW --upload-file "${ARTIFACTS_PATH}${FILE_NAME}" $XCALBUILD_NEXUS_REPO_ADDRESS_SZ
            '''
        }
    }
}