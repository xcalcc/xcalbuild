pipeline {
	agent {
       node {
         label "${AGENT}"
       }
    }

	environment {
		api_ip = "$API_IP"
		api_port = "$API_PORT"
		xvsa_image = "$IMAGE_NAME"
	}

    stages {
        stage('Build') {
            steps {
                sh 'bash scripts/build/lin/ci-build.sh'
            }
        }
		stage('Test') {
			steps {
				echo 'Unit Testing for CI...'
				sh 'bash scripts/test/ci-run-all-projects.sh $api_ip $api_port $xvsa_image'
			}
		}
    }
}
