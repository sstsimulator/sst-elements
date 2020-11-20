
#ifndef _H_VANADIS_COMPARE_TYPE
#define _H_VANADIS_COMPARE_TYPE

namespace SST {
namespace Vanadis {

enum VanadisRegisterCompareType {
        REG_COMPARE_EQ,
        REG_COMPARE_LT,
        REG_COMPARE_LTE,
        REG_COMPARE_GT,
        REG_COMPARE_GTE,
        REG_COMPARE_NEQ
};

const char* convertCompareTypeToString( VanadisRegisterCompareType cType ) {
	switch( cType ) {
	case REG_COMPARE_EQ:
		return "EQ";
	case REG_COMPARE_LT:
		return "LT";
	case REG_COMPARE_LTE:
		return "LTE";
	case REG_COMPARE_GT:
		return "GT";
	case REG_COMPARE_GTE:
		return "GTE";
	case REG_COMPARE_NEQ:
		return "NEQ";
	}

	return "";
};

}
}

#endif
