// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_VANADIS_MICRO_OP
#define _H_SST_VANADIS_MICRO_OP

namespace SST {
namespace Vanadis {

enum MicroOpType {
	INT_S_ALU,
	FP_S_ALU,
	BRANCH,
	NO_OP,
	NOT_DECODED
};

class VanadisMicroOp {

	protected:
		uint64_t pc;
		uint32_t seq;

	public:
		VanadisMicroOp(uint64_t pc, uint32_t seq);
		uint64_t getProgramCounter();
		uint32_t getSequenceNumber();

		virtual MicroOpType getMicroOpType() = 0;

};

}
}

#endif
