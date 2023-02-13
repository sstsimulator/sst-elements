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
#include <execinfo.h>

#include "tb_header.h"

#define PAGE_SIZE 4096

void ariel_enable() {
    printf("\nStatic defination of Ariel_enable is being called\n");
}

void ariel_fence() {
    printf("\nStatic defination of Ariel_fence is being called\n");
}

void mlm_set_pool(int level) {

}

void* mlm_malloc(size_t size, int level) {
	if(size == 0) {
		printf("ZERO BYTE MALLOC\n");
		void* bt_entries[64];
		int entries = backtrace(bt_entries, 64);
		backtrace_symbols_fd(bt_entries, entries, 1);
		exit(-1);
	}

#ifdef mlm_DEBUG
	printf("Performing a mlm Malloc for size %llu\n", size);
#endif

	return malloc(size);
}

void  mlm_free(void* ptr) {
	free(ptr);
}

mlm_Tag mlm_memcpy(void* dest, void* src, size_t length) {
//	printf("Performing a mlm memcpy...\n");
	size_t i;

	char* dest_c = (char*) dest;
	char* src_c  = (char*) src;

	for(i = 0; i < length; i++) {
		dest_c[i] = src_c[i];
	}

	return 0;
}

void mlm_waitComplete(mlm_Tag in) {
	return;
}
    
void start_RTL_sim(RTL_shmem_info* shmem) {     
    printf("\nStatic defination of start_RTL_sim is being called\n"); 
    return;
}

void update_RTL_sig(RTL_shmem_info* shmem) {     
    printf("\nStatic defination of update_RTL_signals in being called\n");
    return; 
}
