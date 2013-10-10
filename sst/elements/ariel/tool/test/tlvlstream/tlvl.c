#include <stdlib.h>
#include "tlvl.h"

#define PAGE_SIZE 4096

void* tlvl_malloc(size_t size) {
	return malloc(size);
}

void  tlvl_free(void* ptr) {
	free(ptr);
}
