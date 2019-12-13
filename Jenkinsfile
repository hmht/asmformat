pipeline {
	agent none

	stages {
		stage('Build') {
			parallel {
				stage('Windows build') {
					agent { label "windows" }
					steps { bat "make" }
				}
				stage('Linux build') {
					agent { label "linux" }
					steps { sh "make" }
				}
			}
		}
        }
}
