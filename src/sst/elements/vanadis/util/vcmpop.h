
#ifndef _H_VANADIS_COMPARE_OP
#define _H_VANADIS_COMPARE_OP

#include "inst/vcmptype.h"
#include "inst/regfile.h"

#include <cinttypes>
#include <cstdint>

namespace SST {
namespace Vanadis {

template<typename T>
bool registerCompareValues( VanadisRegisterCompareType compareType,
	VanadisRegisterFile* regFile,
	VanadisInstruction* ins,
	SST::Output* output,
	T left_value, T right_value ) {

	output->verbose(CALL_INFO, 16, 0, "---> reg-val: %" PRId64 " reg-val: %" PRId64 "\n", left_value, right_value );

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
