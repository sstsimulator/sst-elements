
#ifndef _H_OBERON_BOOLEAN_VALUE
#define _H_OBERON_BOOLEAN_VALUE

namespace SST {
namespace Oberon {

class OberonBooleanStackValue : OberonExpressionStackValue {

	public:
		OberonBooleanStackValue(bool val);

	private:
		bool value;

}

}
}

#endif
