
#include "physreg.h"

VeronaRegisterGroup::VeronaRegisterGroup(int reg_count, int reg_width_bytes, 
	bool reg_zero_is_zero) {
	
	zero_reg_is_zero = reg_zero_is_zero;
	register_count   = reg_count;
	register_width_bytes = reg_width_bytes;	
}

void VeronaRegisterGroup::internal_copy(const void* source, void* dest, const int length) {
	uint8_t* source_c = (uint8_t*) source;
	uint8_t* dest_c   = (uint8_t*) dest;
	
	for(int i = 0; i < length; i++) {
		dest_c[i] = source_c[i];
	}
}
