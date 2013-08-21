
#ifndef _SST_OBERON_ENGINE
#define _SST_OBERON_ENGINE

namespace SST {
namespace Oberon {

class OberonEngine {

	public:
		void generateNextEvent();

	private:
		uint32_t pc;
		OberonModel model;
		OberonExpressionStack exprStack;
		bool isHalted;

}

}
}

#endif
