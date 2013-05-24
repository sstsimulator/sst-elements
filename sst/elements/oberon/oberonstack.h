
#ifndef _H_OBERON_STACK
#define _H_OBERON_STACK

#include <stdint.h>
#include <vector>

namespace SST {
namespace Oberon {

class OberonStackEntry {
	public:
		OberonStackEntry(string func, uint32_t func_address);
		string& getFunctionName();
		uint32_t getInstructionAddress();

	protected:
		string function_name;
		uint32_t instr_address;
}

class OberonStack {
	public:
		OberonStack(uint32_t stack_limit);
		void push(OberonStackEntry entry);
		OberonStackEntry pop();

	protected:
		uint32_t stack_limit;
		vector<OberonStackEntry> stack;
}

}
}

#endif
