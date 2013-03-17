
#include "physreg.h"

VeronaRegisterGroup::VeronaRegisterGroup(unsigned int reg_count,
	unsigned int reg_width_bytes, 
	bool reg_zero_is_zero, string group_name) {

	zero_reg_is_zero     = reg_zero_is_zero;
	register_count       = reg_count;
	register_width_bytes = reg_width_bytes;
	reg_group_name       = group_name;
	
	// Create a large chunk of memory for *all* variables, preset values to zero
	void* register_contents = malloc(reg_count * reg_width_bytes);
	memset( register_contents, 0, reg_count * reg_width_bytes );
	
	// Create an array to map the chunk to individual registers
	register_group = (void**) malloc( sizeof(void*) * reg_count );
	
	// Map chunk to individual registers
	for(unsigned int i = 0; i < reg_count; i++) {
		register_group[i] = &register_contents[i * reg_width_bytes];
	}
}

void VeronaRegisterGroup::PrintRegisterGroup() {
	std::cout << "--- Register Group: " << group_name << " ---" << std::endl;
	std::cout << std::endl;
	std::cout << "Register Count: " << register_count << " / Width bytes: " 
		<< reg_width_bytes << std::endl << std::endl;
	
	uint8_t* content_ptr;
	const unsigned int max_digits = (unsigned int) log10((double) register_count);
	
	for(unsigned int i = 0; i < register_count; i++) {
		unsigned int digits_reg_num = (unsigned int) log10((double) i);
	
		for(unsigned int j = 0; j = (max_digits - digits_reg_num); j++) {
			std::cout << " ";
		}
		
		std::cout << i << " [ ";
		
		content_ptr = (uint8_t*) register_group[i];
		
		for(unsigned int j = 0; j < register_width_bytes; j++) {
			std::cout << content_ptr[j];
		}
		
		std::cout << " ]" << std::endl;
	}

	std::cout << "--- End of Register Group ---" << std::endl;
}

void* VeronaRegisterGroup::GetRegisterContents(unsigned int reg_num) {
	// Make sure we are getting a permitted register
	assert( reg_num >= 0 && reg_num < register_count );
	
	// Return the requests register contents
	return register_group[ reg_num ];
}

void VeronaRegisterGroup::internal_copy(const void* source, void* dest, 
	const unsigned int length) {
	
	uint8_t* source_c = (uint8_t*) source;
	uint8_t* dest_c   = (uint8_t*) dest;
	
	for(unsigned int i = 0; i < length; i++) {
		dest_c[i] = source_c[i];
	}
}
