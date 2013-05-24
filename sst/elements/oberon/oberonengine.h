
#ifndef _H_OBERON_ENGINE
#define _H_OBERON_EGNGINE

namespace SST {
namespace Oberon {

#define OBERON_INS_HALT      0

#define OBERON_INS_ADD 	     1
#define OBERON_INS_SUB       2
#define OBERON_INS_MUL       3
#define OBERON_INS_DIV       4

#define OBERON_INS_EQ		 16
#define OBERON_INS_LT		 17
#define OBERON_INS_GT        18
#define OBERON_INS_LTE       19
#define OBERON_INS_GTE       20

#define OBERON_INS_NOT		 32
#define OBERON_INS_AND		 33
#define OBERON_INS_OR        34

#define OBERON_INS_POP	     64
#define OBERON_INS_PUSH      64

#define OBERON_INS_CALL_BUILTIN 255

#define OBERON_BUILTIN_PRINT 16384


class OberonEngine {

	public:
		OberonEngine(uint32_t stackAllocation,
			uint32_t initialPC,
			void* binary);
		OberonEvent generateNextEvent();
		bool isHalted();
		
	private:
		OberonExpressionStack exprStack;
		
		bool engineHalted;
		uint32_t pc;
		uint32_t stackptr;
		void* programBinary;
		void* stack;

}


}
}

#endif