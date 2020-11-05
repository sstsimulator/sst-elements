
#ifndef _H_VANADIS_FP_WIDTH
#define _H_VANADIS_FP_WIDTH

namespace SST {
namespace Vanadis {

enum VanadisFPWidth {
        VANADIS_WIDTH_F32,
        VANADIS_WIDTH_F64
};

enum VanadisFPRegisterFormat {
	VANADIS_FORMAT_FP32,
	VANADIS_FORMAT_FP64,
	VANADIS_FORMAT_INT32,
	VANADIS_FORMAT_INT64
};

}
}

#endif
