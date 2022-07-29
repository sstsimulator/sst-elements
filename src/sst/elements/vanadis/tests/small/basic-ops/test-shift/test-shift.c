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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

void print_shift_u64( uint64_t value, uint64_t shift ) {
	const uint64_t result = value << shift;
	printf("value: %" PRIu64 " << %" PRIu64 " = %" PRIu64 "\n",
		value, shift, result);
}

void print_shift_d64( int64_t value, int64_t shift ) {
	const int64_t result = value << shift;
	printf("value: %" PRId64 " << %" PRId64 " = %" PRId64 "\n",
		value, shift, result);
}

void print_rshift_u64( uint64_t value, uint64_t shift ) {
	const uint64_t result = value >> shift;
	printf("value: %" PRIu64 " >> %" PRIu64 " = %" PRIu64 "\n",
		value, shift, result);
}

void print_rshift_d64( int64_t value, int64_t shift ) {
	const int64_t result = value >> shift;
	printf("value: %" PRId64 " >> %" PRId64 " = %" PRId64 "\n",
		value, shift, result);
}

void print_shift_u32( uint32_t value, uint32_t shift ) {
	const uint32_t result = value << shift;
	printf("value: %" PRIu32 " << %" PRIu32 " = %" PRIu32 "\n",
		value, shift, result);
}

void print_shift_d32( int32_t value, int32_t shift ) {
	const int32_t result = value << shift;
	printf("value: %" PRId32 " << %" PRId32 " = %" PRId32 "\n",
		value, shift, result);
}

void print_rshift_u32( uint32_t value, uint32_t shift ) {
	const uint32_t result = value >> shift;
	printf("value: %" PRIu32 " >> %" PRIu32 " = %" PRIu32 "\n",
		value, shift, result);
}

void print_rshift_d32( int32_t value, int32_t shift ) {
	const int32_t result = value >> shift;
	printf("value: %" PRId32 " >> %" PRId32 " = %" PRId32 "\n",
		value, shift, result);
}

int main( int argc, char* argv[] ) {
	const uint64_t value = 1;

	for( uint64_t s = 1; s < 64; s++ ) {
		print_shift_u64( value, s );
	}

	for( int64_t s = 1; s < 64; s++ ) {
		print_shift_d64( value, s );
	}

	for( int64_t s = 1; s < 64; s++ ) {
		print_rshift_u64( (1ULL << 63ULL), s );
	}

	for( int64_t s = 1; s < 64; s++ ) {
		print_rshift_d64( INT64_MIN, s );
	}

	const uint32_t value32 = 1;

	for( uint32_t s = 1; s < 32; s++ ) {
		print_shift_u32( value32, s );
	}

	for( int32_t s = 1; s < 32; s++ ) {
		print_shift_d32( value32, s );
	}

	for( uint32_t s = 1; s < 32; s++ ) {
		print_rshift_u32( (1ULL << 31UL), s );
	}

	for( int32_t s = 1; s < 32; s++ ) {
		print_rshift_d32( INT32_MIN, s );
	}

	return 0;
}
