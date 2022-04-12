// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
//#include <malloc.h>


//#define BUF_SIZE 1024*1024
#define BUF_SIZE 1

typedef struct {
    int val;
} NicCmd;

void freeCmd( NicCmd* cmd );

int main( int argc, char* argv[] ) {

	setvbuf(stdout, NULL, _IONBF, 0);

	printf("%s() enter\n",__func__);
	uint32_t* buf = malloc( BUF_SIZE * sizeof(uint32_t) );
	printf("%s() 4 %p\n",__func__,buf);
	//printf("%s() %p\n",__func__,malloc( 1024*1024 * sizeof(uint32_t) ));
	printf("%s() 64 %p\n",__func__,malloc( 64 ));
	printf("%s() 1 %p\n",__func__,malloc( 1 ));
	printf("%s() 64 %p\n",__func__,malloc( 64 ));
	printf("%s() 4K %p\n",__func__,malloc( 1024 * 4 ));
	printf("%s() 64 %p\n",__func__,malloc( 64 ));
	free( buf );
	printf("%s() leave\n",__func__);
}

void freeCmd( NicCmd* cmd ) {
    free( cmd );
}

