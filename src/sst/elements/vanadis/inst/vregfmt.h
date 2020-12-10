
#ifndef _H_VANADIS_REG_FORMAT
#define _H_VANADIS_REG_FORMAT

namespace SST {
namespace Vanadis {

enum VanadisRegisterFormat {
	VANADIS_FORMAT_FP32,
	VANADIS_FORMAT_FP64,
	VANADIS_FORMAT_INT32,
	VANADIS_FORMAT_INT64
};

const char* registerFormatToString( const VanadisRegisterFormat fmt ) {
	switch( fmt ) {
	case VANADIS_FORMAT_FP32:   return "F32";
	case VANADIS_FORMAT_FP64:   return "F64";
	case VANADIS_FORMAT_INT32:  return "I32";
	case VANADIS_FORMAT_INT64:  return "I64";
	default:
		return "UNK";
	}
};

}
}

#endif
