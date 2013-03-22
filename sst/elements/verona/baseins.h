#ifndef _baseins_H
#define _baseins_H

#include <cstdio>
#include <cstdint>

#include <boost/utility.hpp>

class Instruction {

	private:
		uint32_t instruction;
		const uint32_t OP_CODE_MASK = BOOST_BINARY_UL( 00000000000000000000000001111111 );

	public:
		Instruction(uint32_t ins);
		~Instruction();
		int getOpCode();

}

#endif
