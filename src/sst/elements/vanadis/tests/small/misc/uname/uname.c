// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sys/utsname.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

	struct utsname name;

	uname(&name);

    printf("sizeof(struct utsname) %zu\n",sizeof(struct utsname));
	printf("%s\n%s\n%s\n%s\n%s\n", name.sysname, name.nodename, name.release, name.version, name.machine );
}
