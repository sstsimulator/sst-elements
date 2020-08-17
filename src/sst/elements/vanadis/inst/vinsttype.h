
#ifndef _H_VANADIS_INST_CLASS
#define _H_VANADIS_INST_CLASS

namespace SST {
namespace Vanadis {

enum VanadisFunctionalUnitType {
	INST_INT_ARITH,
	INST_INT_DIV,
	INST_FP_ARITH,
	INST_FP_DIV,
	INST_LOAD,
	INST_STORE,
	INST_BRANCH,
	INST_SPECIAL,
	INST_FENCE,
	INST_NOOP
};

const char* funcTypeToString( VanadisFunctionalUnitType unit_type ) {
switch( unit_type) {
case INST_INT_ARITH:
	return "INT_ARITH";
case INST_INT_DIV:
	return "INT_DIV";
case INST_FP_ARITH:
	return "FP_ARITH";
case INST_FP_DIV:
	return "FP_DIV";
case INST_LOAD:
	return "LOAD";
case INST_STORE:
	return "STORE";
case INST_BRANCH:
	return "BRANCH";
case INST_SPECIAL:
	return "SPECIAL";
case INST_FENCE:
	return "FENCE";
case INST_NOOP:
	return "NOOP";
}
};

}
}

#endif
