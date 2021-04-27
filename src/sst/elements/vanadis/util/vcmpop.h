// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_COMPARE_OP
#define _H_VANADIS_COMPARE_OP

#include "inst/vcmptype.h"
#include "inst/regfile.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

inline void register_compare_values_print_msg(uint32_t line, const char* file, const char* func, SST::Output* output, int32_t left_value, int32_t right_value) {
	output->verbose(line, file, func, 16, 0, "---> reg-val: %" PRId32 " reg-val: %" PRId32 "\n", left_value, right_value );
}

inline void register_compare_values_print_msg(uint64_t line, const char* file, const char* func, SST::Output* output, int64_t left_value, int64_t right_value) {
	output->verbose(line, file, func, 16, 0, "---> reg-val: %" PRId64 " reg-val: %" PRId64 "\n", left_value, right_value );
}

inline void register_compare_values_print_msg(uint32_t line, const char* file, const char* func, SST::Output* output, uint32_t left_value, uint32_t right_value) {
	output->verbose(line, file, func, 16, 0, "---> reg-val: %" PRIu32 " reg-val: %" PRIu32 "\n", left_value, right_value );
}

inline void register_compare_values_print_msg(uint64_t line, const char* file, const char* func, SST::Output* output, uint64_t left_value, uint64_t right_value) {
	output->verbose(line, file, func, 16, 0, "---> reg-val: %" PRIu64 " reg-val: %" PRIu64 "\n", left_value, right_value );
}


template<typename T>
bool registerCompareValues( VanadisRegisterCompareType compareType,
	VanadisRegisterFile* regFile,
	VanadisInstruction* ins,
	SST::Output* output,
	T left_value, T right_value ) {

    register_compare_values_print_msg(CALL_INFO, output, left_value, right_value );

	bool compare_result = false;

	switch( compareType ) {
	case REG_COMPARE_EQ:
		{
			compare_result = (left_value) == (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: equal     / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	case REG_COMPARE_NEQ:
		{
			compare_result = (left_value) != (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: not-equal / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	case REG_COMPARE_LT:
		{
			compare_result = (left_value) < (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	case REG_COMPARE_LTE:
		{
			compare_result = (left_value) <= (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: less-than-eq / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	case REG_COMPARE_GT:
		{
			compare_result = (left_value) > (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	case REG_COMPARE_GTE:
		{
			compare_result = (left_value) >= (right_value);
			output->verbose(CALL_INFO, 16, 0, "-----> compare: greater-than-eq / result: %s\n", (compare_result ? "true" : "false") );
		}
		break;
	default:
		{
			output->verbose(CALL_INFO, 16, 0, "-----> Unknown comparison operation at instruction: 0x%llx\n", ins->getInstructionAddress());
			ins->flagError();
		}
		break;
	}

	return compare_result;
};

template<typename T>
bool registerCompare( VanadisRegisterCompareType compareType,
	VanadisRegisterFile* regFile,
	VanadisInstruction* ins,
	SST::Output* output,
	uint16_t left, uint16_t right ) {

	const T left_value  = regFile->getIntReg<T>( left  );
	const T right_value = regFile->getIntReg<T>( right );

	return registerCompareValues<T>( compareType, regFile, ins, output, left_value, right_value );
};

template<typename T>
bool registerCompareImm( VanadisRegisterCompareType compareType,
	VanadisRegisterFile* regFile,
	VanadisInstruction* ins,
	SST::Output* output,
	uint16_t left, T right_value ) {

	const T left_value  = regFile->getIntReg<T>( left  );

	return registerCompareValues<T>( compareType, regFile, ins, output, left_value, right_value );
};

}
}

#endif
