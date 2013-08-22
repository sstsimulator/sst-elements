
#ifndef SST_OBERON_EXPR_VALUE_BASE
#define SST_OBERON_EXPR_VALUE_BASE

namespace SST {
namespace Oberon {

class OberonExpressionValue {

	public:
		virtual int64_t getAsInt64() = 0;
		virtual double  getAsFP64()  = 0;

};

}
}

#endif
