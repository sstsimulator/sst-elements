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

#ifndef _H_VANADIS_INSTRUCTION_DECODE_FAULT
#define _H_VANADIS_INSTRUCTION_DECODE_FAULT

#include "inst/vinst.h"
#include "decoder/visaopts.h"
#include "inst/vinsttype.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionDecodeFault : public VanadisInstruction {
public:
	VanadisInstructionDecodeFault(
		const uint64_t address,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts) :
		VanadisInstruction( address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0 ) {

		trapError = true;
	}

	virtual VanadisInstruction* clone() {
		return new VanadisInstructionDecodeFault( ins_address, hw_thread, isa_options );
	}

	virtual const char* getInstCode() const {
		return "DECODE_FAULT";
	}

	virtual void printToBuffer( char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "%s", "DECODE_FAULT" );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FAULT;
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {

	}

};

}
}

#endif
