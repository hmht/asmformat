pipeline {
	agent none

	stages {
		stage('Build') {
			parallel {
				/* HOW do people manage to compile on windows!??
				stage('Windows build') {
					agent { label "windows" }
				} */
				stage('Linux build') {
					agent { label "linux" }
					steps { sh "make" }
				}
			}
		}
        }
}
