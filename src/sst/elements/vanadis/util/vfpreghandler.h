
#ifndef _H_VANADIS_FP_REGISTER_HANDLER
#define _H_VANADIS_FP_REGISTER_HANDLER

#include <cstring>

namespace SST {
namespace Vanadis {

template<typename T>
T combineFromRegisters( VanadisRegisterFile* regFile, uint16_t r_left, uint16_t r_right ) {
	T tmp_64 = 0.0;
	char* tmp_64_ptr = (char*) &tmp_64;

	uint32_t left_32 = regFile->getFPReg<uint32_t>( r_left );
	std::memcpy( tmp_64_ptr, &left_32, sizeof( left_32 ) );

	uint32_t right_32 = regFile->getFPReg<uint32_t>( r_right );
	std::memcpy( tmp_64_ptr + sizeof( left_32 ), &right_32, sizeof( right_32 ) );

	return tmp_64;
};

template<typename T>
void fractureToRegisters( VanadisRegisterFile* regFile, uint16_t r_left, uint16_t r_right, T value ) {

	char* value_ptr = (char*) &value;

	uint32_t left_val = 0;
	std::memcpy( (char*) &left_val, value_ptr, sizeof( left_val ) );

	uint32_t right_val = 0;
	std::memcpy( (char*) &right_val, value_ptr + sizeof( left_val ), sizeof( right_val ) );

	regFile->setFPReg<uint32_t>( r_left,  left_val  );
	regFile->setFPReg<uint32_t>( r_right, right_val );

};

}
}

#endif
