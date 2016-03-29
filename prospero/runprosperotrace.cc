
#include <sst_config.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vector>
#include <string>

int main(int argc, char* argv[]) {

	char* pinToolMarker = const_cast<char*>("-t");
	char* doubleDash = const_cast<char*>("--");

	bool foundMarker = false;
	std::vector<char*> prosParams;
	std::vector<char*> appParams;
	char* appPath;

	char* toolPath = (char*) malloc(sizeof(char) * PATH_MAX);

#ifdef SST_COMPILE_MACOSX
	sprintf(toolPath, "%s/libexec/prosperotrace.dylib", SST_INSTALL_PREFIX);
#else
	sprintf(toolPath, "%s/libexec/prosperotrace.so", SST_INSTALL_PREFIX);
#endif

	appParams.push_back(const_cast<char*>(PINTOOL_EXECUTABLE));
	appParams.push_back(pinToolMarker);
	appParams.push_back(toolPath);

	for(int i = 1; i < argc; i++) {
		if(foundMarker) {
			appParams.push_back(argv[i]);
		} else {
			if(std::strcmp(argv[i], doubleDash) == 0) {
				foundMarker = true;

				if(i == ( argc - 1 )) {
					fprintf(stderr, "Found -- marker, but no application path in options\n");
					exit(-1);
				} else {
					appParams.push_back(argv[i]);
					appParams.push_back(argv[i+1]);
					appPath = argv[i+1];

					i++;
				}
			} else {
				prosParams.push_back(argv[i]);
			}
		}
	}

	if(! foundMarker) {
		fprintf(stderr, "Error: did not find the -- marker which separates out the application and profiling options\n");
		exit(-1);
	}

	printf("===============================================================\n");
	printf("Prospero Memory Tracing Tool\n");
	printf("Part of the Structural Simulation Toolkit (SST)\n");
	printf("===============================================================\n");
	printf("\n");

	for(auto nextStr = prosParams.begin(); nextStr != prosParams.end(); nextStr++) {
		printf("P: %s\n", *nextStr);
	}

	printf("Prospero will run the following for tracing:\n");
	for(auto nextStr = appParams.begin(); nextStr != appParams.end(); nextStr++) {
		printf("%s ", *nextStr);
	}

	printf("\n");

	pid_t childProcess;
	childProcess = fork();

	if(childProcess < 0) {
		perror("fork");
		fprintf(stderr, "Error: Failed to launch the child process.\n");
		exit(-1);
	}

	if(childProcess != 0) {
		// The process has been launched and we need to wait for it
		int processStat = 0;
		pid_t childRC = waitpid(childProcess, &processStat, 0);

		printf("\n");
		printf("===============================================================\n");

		if( childRC > 0 ) {
			printf("Child process(es) for profiling completed.\n");
		} else {
			fprintf(stderr, "Error: Unable to start PIN.\n");
			perror("waitpid");
			exit(-1);
		}

	} else {
		appParams.push_back(NULL);

		char** paramsArray = (char**) malloc(sizeof(char*) * appParams.size());
		for(auto i = 0; i < appParams.size(); i++) {
			paramsArray[i] = appParams[i];
		}

		int executeRC = execvp(PINTOOL_EXECUTABLE, paramsArray);
		printf("Executing application returns %d.\n", executeRC);
	}

	return 0;
}

