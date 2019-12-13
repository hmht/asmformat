pipeline {
	agent none

	stages {
		stage('Build') {
			parallel {
				stage('Windows build') {
					agent { label "windows && make && gcc" }
					steps { bat "make" }
				}
				stage('Linux build') {
					agent { label "linux && make && gcc" }
					steps { sh "make" }
				}
			}
		}
        }
}
