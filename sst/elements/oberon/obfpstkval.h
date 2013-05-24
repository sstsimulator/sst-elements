
#ifndef _H_OBERON_DOUBLE_STACK_VALUE
#define _H_OBERON_DOUBLE_STACK_VALUE

namespace SST {
namespace Oberon {

class OberonDoubleStackValue : OberonExpressionStackValue {

	public:
		OberonDoubleStackValue(double val);

	private:
		double value;

}

}
}

#endif
