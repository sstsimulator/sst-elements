
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char **environ;

int main(int argc, char* argv[]) {

	printf("SST Environment Printer\n");

	int next_env_index = 0;

	while( NULL != environ[next_env_index] ) {
		char* key   = strtok(environ[next_env_index], "=");
		char* value = strtok(NULL, "=");

		printf("%5d %30s = \"%s\"\n", next_env_index, key, value);
		next_env_index++;
	}

	return 0;
}
