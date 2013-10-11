#include <stdlib.h>
#include <stdio.h>
#include "tlvl.h"

#define PAGE_SIZE 4096

void* tlvl_malloc(size_t size) {
	return malloc(size);
}

void  tlvl_free(void* ptr) {
	free(ptr);
}

void tlvl_memcpy(void* dest, void* src, size_t length) {
	printf("Performing a TLVL memcpy...\n");
	size_t i;

	char* dest_c = (char*) dest;
	char* src_c  = (char*) src;

	for(i = 0; i < length; i++) {
		dest_c[i] = src_c[i];
	}
}
