/* cpsp_invoker forks child processes to execute actual cpsp code, thereby 
 * separating the server instance from scripting code. Since windows doesn't
 * have fork, this functionality must be encapsulated in a separate executable.
 * cpsp_worker.c defines this executable. This code is only compiled on 
 * windows. It sets up the environment and runs the cpsp_worker_run() 
 * function from cpsp_invoker. 
 */
#include <stdio.h>
#include <stdlib.h>
#include "cpsp.h"
#include "cpsp_invoker.h"

#define CPSP_BIN "CPSP_BIN"
#define DOC_ROOT "DOC_ROOT"
#define PATH "PATH"

static char *bin_path = "";
static char *doc_root = ".";
static char *path = "";

int main(int argc, char *argv[], char **env)
{
	int i;
	char *p; 

	HANDLE in, out;
	int fd_in = atoi(argv[1]);
	int fd_out = atoi(argv[2]);
	
	in = (void *) fd_in;
	out = (void *) fd_out;

	for (i = 0; p = env[i]; i++)
	{
		int len;
	   
		len = strlen(CPSP_BIN);
		if (strncmp(p, CPSP_BIN, len) == 0)
		{
			bin_path = &p[len + 1];
			continue;
		}

		len = strlen(DOC_ROOT);
		if (strncmp(p, DOC_ROOT, len) == 0)
		{
			doc_root = &p[len + 1];
			continue;
		}

		len = strlen(PATH);
		if (strncmp(p, PATH, len) == 0)
		{
			path = &p[len + 1];
			continue;
		}
	}

	if (path) _putenv(path);

	cpsp_init(1, bin_path, doc_root);

	cpsp_worker_run(in, out); /* dummy file descriptor */
}

