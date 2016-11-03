// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
