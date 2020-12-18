
#ifndef _H_VANADIS_SIGN_UTILS
#define _H_VANADIS_SIGN_UTILS

#include <cinttypes>
#include <cstdint>

#define VANADIS_1BYTE_SIGN_MASK  0x80
#define VANADIS_2BYTE_SIGN_MASK  0x8000
#define VANADIS_4BYTE_SIGN_MASK  0x80000000

#define VANADIS_EXTEND_1BYTE_SET 0xFFFFFFFFFFFFFF00
#define VANADIS_EXTEND_2BYTE_SET 0xFFFFFFFFFFFF0000
#define VANADIS_EXTEND_4BYTE_SET 0xFFFFFFFF00000000

#define VANADIS_4BYTE_EXTRACT    0x000000000000FFFF

namespace SST {
namespace Vanadis {

uint64_t vanadis_sign_extend( const uint8_t value ) {
	uint64_t value_64 = (uint64_t) value;

	if( (VANADIS_1BYTE_SIGN_MASK & value) != 0 ) {
		value_64 = value_64 | VANADIS_EXTEND_1BYTE_SET;
	}

	return value_64;
};

uint64_t vanadis_sign_extend( const uint16_t value ) {
	uint64_t value_64 = (uint64_t) value;

	if( (VANADIS_2BYTE_SIGN_MASK & value) != 0 ) {
		value_64 = value_64 | VANADIS_EXTEND_2BYTE_SET ;
	}

	return value_64;
};

uint64_t vanadis_sign_extend( const uint32_t value ) {
	uint64_t value_64 = (uint64_t) value;

	if( (VANADIS_4BYTE_SIGN_MASK & value) != 0 ) {
		value_64 = value_64 | VANADIS_EXTEND_4BYTE_SET;
	}

	return value_64;
};

int64_t vanadis_sign_extend_offset_16( const uint32_t value ) {
	int64_t value_64 = (value & VANADIS_4BYTE_EXTRACT);

	if( (value_64 & VANADIS_2BYTE_SIGN_MASK) != 0 ) {
		value_64 |= VANADIS_EXTEND_2BYTE_SET;
	}

	return value_64;
};

int64_t vanadis_sign_extend_offset_16_and_shift( const uint32_t value, const uint32_t shift ) {
	int64_t value_64 = vanadis_sign_extend_offset_16( value );
	value_64 = value_64 << shift;

	return value_64;
};

}
}

#endif
