#include <stdlib.h>
#include <stdio.h>
#include "tlvl.h"
#include <execinfo.h>

#define PAGE_SIZE 4096

void* tlvl_malloc(size_t size) {
	if(size == 0) {
		printf("ZERO BYTE MALLOC\n");
		void* bt_entries[64];
		int entries = backtrace(bt_entries, 64);
		backtrace_symbols_fd(bt_entries, entries, 1);
		exit(-1);
	}
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
