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
        FILE_NAME="xcalbuild-mac-${ID}.zip"
        ARTIFACTS_PATH="${WORKDIR}/artifacts/"
        XCALBUILD_NEXUS_REPO_ADDRESS_SH="${XCALBUILD_NEXUS_REPO_ADDRESS_SH ? XCALBUILD_NEXUS_REPO_ADDRESS_SH : 'http://10.10.4.154:8081/repository/xcalbuild-mac/'}"
        XCALBUILD_NEXUS_REPO_ADDRESS_SZ="${XCALBUILD_NEXUS_REPO_ADDRESS_SZ ? XCALBUILD_NEXUS_REPO_ADDRESS_SZ : 'http://10.10.2.141:8081/repository/xcalbuild-mac/'}"
    }

    stages {
        stage('build xcalbuild') {
            steps {
                sh 'echo "Running as user: $USER ($UID) on host $HOSTNAME with shell $SHELL"'
                sh 'echo "Current branch is $(git rev-parse --abbrev-ref HEAD)"'

                sh 'bash ${WORKDIR}/scripts/build/mac/build.sh $VERSION $VCPKG_PATH'
            }
        }
        stage('modify profile') {
            steps {
                sh '''
                    sed -i "" -e 's%"cPrependPreprocessingOptions": \\[\\]%"cPrependPreprocessingOptions": \\["-U__BLOCKS__"\\]%' -e 's%"cxxPrependPreprocessingOptions": \\[\\]%"cxxPrependPreprocessingOptions": \\["-U__BLOCKS__"\\]%' ${WORKDIR}/profiles/gnu-cc/gcc.json
                '''
            }
        }
        stage('unit test') {
            steps {
                sh 'bash ${WORKDIR}/scripts/test/run-builtin-tests.sh'
            }
        }
        stage('integration test') {
            steps {
                sh 'echo "later will enable integration tests"'
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
