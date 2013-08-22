
#ifndef SST_OBERON_I64_VALUE
#define SST_OBERON_I64_VALUE

#include <stdint.h>

#include "obexprval.h"

namespace SST {
namespace Oberon {

class OberonI64ExprValue : public OberonExpressionValue {

	private:
		int64_t value;

	public:
		OberonI64ExprValue(int64_t v);
		int64_t getAsInt64();
		double  getAsFP64();

};

}
}

#endif
