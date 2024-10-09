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

#include <stdio.h>
#include <unistd.h>
#include <syscall.h>

pid_t gettid() {
    return syscall(SYS_gettid);
}

int main( int argc, char* argv[] ) {

    printf("%s() pid=%d tid=%d ppid=%d pgid=%d\n",__func__, getpid(), gettid(), getppid(), getpgid(0));

    pid_t pid = fork();

    if ( 0 == pid ) {
    	printf("child: pid=%d tid=%d ppid=%d pgid=%d\n",getpid(), gettid(), getppid(), getpgid(0));
    } else {
        printf("parent: new child=%d\n",pid);
    }
} 
