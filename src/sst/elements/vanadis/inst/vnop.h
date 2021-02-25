// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NOP
#define _H_VANADIS_NOP

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisNoOpInstruction : public VanadisInstruction {
public:
	VanadisNoOpInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0) {}

	VanadisNoOpInstruction* clone() {
		return new VanadisNoOpInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_NOOP;
	}

	virtual const char* getInstCode() const {
		return "NOP";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "NOP");
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

};

}
}

#endif
