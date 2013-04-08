// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _baseins_H
#define _baseins_H

#include <cstdio>
#include <cstdint>

#include <boost/utility.hpp>

namespace SST {
namespace Verona {

class Instruction {

	private:
		uint32_t instruction;
		const uint32_t OP_CODE_MASK = BOOST_BINARY_UL( 00000000000000000000000001111111 );

	public:
		Instruction(uint32_t ins);
		~Instruction();
		int getOpCode();

}

}
}
#endif
