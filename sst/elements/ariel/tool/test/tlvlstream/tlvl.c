// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdlib.h>
#include <stdio.h>
#include "tlvl.h"
#include <execinfo.h>

#define PAGE_SIZE 4096

void tlvl_set_pool(int level) {

}

void* tlvl_malloc(size_t size, int level) {
	if(size == 0) {
		printf("ZERO BYTE MALLOC\n");
		void* bt_entries[64];
		int entries = backtrace(bt_entries, 64);
		backtrace_symbols_fd(bt_entries, entries, 1);
		exit(-1);
	}

#ifdef TLVL_DEBUG
	printf("Performing a TLVL Malloc for size %llu\n", size);
#endif

	return malloc(size);
}

void  tlvl_free(void* ptr) {
	free(ptr);
}

tlvl_Tag tlvl_memcpy(void* dest, void* src, size_t length) {
//	printf("Performing a TLVL memcpy...\n");
	size_t i;

	char* dest_c = (char*) dest;
	char* src_c  = (char*) src;

	for(i = 0; i < length; i++) {
		dest_c[i] = src_c[i];
	}

	return 0;
}

void tlvl_waitComplete(tlvl_Tag in) {
	return;
}
