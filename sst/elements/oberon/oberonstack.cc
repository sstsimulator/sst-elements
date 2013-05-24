
#include "oberonstack.h"

using namespace SST;
using namespace SST::Oberon;

OberonStackEntry::OberonStackEntry(string func, uint32_t func_addr) :
	function_name(func)
	instr_address(func_addr)
{
}

OberonStack::OberonStack(uint32_t limit) :
	stack(16)
	stack_limit(limit);
{
}
