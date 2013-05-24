
#ifndef _H_OBERON_BOOLEAN_VALUE
#define _H_OBERON_BOOLEAN_VALUE

#include <stdint.h>

namespace SST {
namespace Oberon {

class OberonIntegerStackValue : OberonExpressionStackValue {

	public:
		OberonIntegerStackValue(int64_t val);

	private:
		int64_t value;

}

}
}

#endif