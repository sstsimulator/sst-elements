
#ifndef _H_VANADIS_INSTRUCTION
#define _H_VANADIS_INSTRUCTION

#include <sst/core/output.h>

#include "decoder/visaopts.h"
#include "inst/vinsttype.h"
#include "inst/regfile.h"

namespace SST {
namespace Vanadis {

class VanadisInstruction {
public:
	VanadisInstruction(
		const uint64_t ins_id,
		const uint64_t address,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t c_phys_int_reg_in,
		const uint16_t c_phys_int_reg_out,
		const uint16_t c_isa_int_reg_in,
		const uint16_t c_isa_int_reg_out,
		const uint16_t c_phys_fp_reg_in,
		const uint16_t c_phys_fp_reg_out,
		const uint16_t c_isa_fp_reg_in,
		const uint16_t c_isa_fp_reg_out) :
		id(ins_id),
		ins_address(address),
		hw_thread(hw_thr),
		isa_options(isa_opts),
		count_phys_int_reg_in(c_phys_int_reg_in),
		count_phys_int_reg_out(c_phys_int_reg_out),
		count_isa_int_reg_in(c_isa_int_reg_in),
		count_isa_int_reg_out(c_isa_int_reg_out),
		count_phys_fp_reg_in(c_phys_fp_reg_in),
		count_phys_fp_reg_out(c_phys_fp_reg_out),
		count_isa_fp_reg_in(c_isa_fp_reg_in),
		count_isa_fp_reg_out(c_isa_fp_reg_out)
	{

		phys_int_regs_in       = new uint16_t[ count_phys_int_reg_in  ];
		phys_int_regs_out      = new uint16_t[ count_phys_int_reg_out ];
		isa_int_regs_in        = new uint16_t[ count_isa_int_reg_in   ];
		isa_int_regs_out       = new uint16_t[ count_isa_int_reg_out  ];

		phys_fp_regs_in        = new uint16_t[ count_phys_fp_reg_in  ];
		phys_fp_regs_out       = new uint16_t[ count_phys_fp_reg_out ];
		isa_fp_regs_in         = new uint16_t[ count_isa_fp_reg_in   ];
		isa_fp_regs_out        = new uint16_t[ count_isa_fp_reg_out  ];

		trapError = false;
		hasExecuted = false;
		hasIssued = false;
		hasRegistersAllocated = false;
		enduOpGroup = false;
		isFrontOfROB = false;
	}

	virtual ~VanadisInstruction() {
		delete[] phys_int_regs_in;
		delete[] phys_int_regs_out;
		delete[] isa_int_regs_in;
		delete[] isa_int_regs_out;
		delete[] phys_fp_regs_in;
		delete[] phys_fp_regs_out;
		delete[] isa_fp_regs_in;
		delete[] isa_fp_regs_out;
	}

	uint16_t countPhysIntRegIn()  const { return count_phys_int_reg_in;  }
	uint16_t countPhysIntRegOut() const { return count_phys_int_reg_out; }
	uint16_t countISAIntRegIn()   const { return count_isa_int_reg_in;  }
	uint16_t countISAIntRegOut()  const { return count_isa_int_reg_out; }

	uint16_t countPhysFPRegIn()   const { return count_phys_fp_reg_in;  }
	uint16_t countPhysFPRegOut()  const { return count_phys_fp_reg_out; }
	uint16_t countISAFPRegIn()    const { return count_isa_fp_reg_in;  }
	uint16_t countISAFPRegOut()   const { return count_isa_fp_reg_out; }
	
	uint16_t* getPhysIntRegIn()  { return phys_int_regs_in; }
	uint16_t* getPhysIntRegOut() { return phys_int_regs_out; }
	uint16_t* getISAIntRegIn()   { return isa_int_regs_in; }
	uint16_t* getISAIntRegOut()  { return isa_int_regs_out; }

	uint16_t* getPhysFPRegIn()   { return phys_fp_regs_in; }
	uint16_t* getPhysFPRegOut()  { return phys_fp_regs_out; }
	uint16_t* getISAFPRegIn()    { return isa_fp_regs_in; }
	uint16_t* getISAFPRegOut()   { return isa_fp_regs_out; }

	uint16_t getPhysIntRegIn(const uint16_t index)  { return phys_int_regs_in[index]; }
	uint16_t getPhysIntRegOut(const uint16_t index) { return phys_int_regs_out[index]; }
	uint16_t getISAIntRegIn(const uint16_t index)   { return isa_int_regs_in[index]; }
	uint16_t getISAIntRegOut(const uint16_t index)  { return isa_int_regs_out[index]; }

	uint16_t getPhysFPRegIn(const uint16_t index)   { return phys_fp_regs_in[index]; }
	uint16_t getPhysFPRegOut(const uint16_t index)  { return phys_fp_regs_out[index]; }
	uint16_t getISAFPRegIn(const uint16_t index)    { return isa_fp_regs_in[index]; }
	uint16_t getISAFPRegOut(const uint16_t index)   { return isa_fp_regs_out[index]; }

	void setPhysIntRegIn(const uint16_t index, const uint16_t reg)  { phys_int_regs_in[index] = reg; }
	void setPhysIntRegOut(const uint16_t index, const uint16_t reg) { phys_int_regs_out[index] = reg; }
	void setPhysFPRegIn(const uint16_t index, const uint16_t reg)   { phys_fp_regs_in[index] = reg; }
	void setPhysFPRegOut(const uint16_t index, const uint16_t reg)  { phys_fp_regs_out[index] = reg; }

	virtual VanadisInstruction* clone() = 0;

	uint64_t getID() const { return id; }
	void markEndOfMicroOpGroup() { enduOpGroup = true; }
	bool endsMicroOpGroup() const { return enduOpGroup; }
	bool trapsError() const { return trapError; }

	uint64_t getInstructionAddress() const { return ins_address; }
	uint32_t getHWThread() const { return hw_thread; }

	virtual const char* getInstCode() const = 0;
	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s", getInstCode());
	}
	virtual VanadisFunctionalUnitType getInstFuncType() const = 0;
	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) = 0;
	virtual void print( SST::Output* output ) {
		output->verbose(CALL_INFO, 8, 0, "%s", getInstCode());
	}
	virtual bool performDeleteAtFuncUnit() const { return false; }

	// Is the instruction predicted (speculation point).
	// for normal instructions this is false
	// but branches and jumps will get predicted
	virtual bool isSpeculated() const { return false; }

	bool completedExecution() const { return hasExecuted; }
	bool completedIssue() const { return hasIssued; }
	bool completedRegisterAllocation() const { return hasRegistersAllocated; }

	void markExecuted() { hasExecuted = true; }
	void markIssued() { hasIssued = true; }
	void markRegistersAllocated() { hasRegistersAllocated = true; }

	bool checkFrontOfROB() { return isFrontOfROB; }
	void markFrontOfROB() { isFrontOfROB = true; }
	void setID( const uint64_t new_id ) { id = new_id; }
protected:
	void flagError() { trapError = true; }

	uint64_t id;
	const uint64_t ins_address;
	const uint32_t hw_thread;

	const uint16_t count_phys_int_reg_in;
	const uint16_t count_phys_int_reg_out;
	const uint16_t count_isa_int_reg_in;
	const uint16_t count_isa_int_reg_out;

	const uint16_t count_phys_fp_reg_in;
	const uint16_t count_phys_fp_reg_out;
	const uint16_t count_isa_fp_reg_in;
	const uint16_t count_isa_fp_reg_out;

	uint16_t* phys_int_regs_in;
	uint16_t* phys_int_regs_out;
	uint16_t* isa_int_regs_in;
	uint16_t* isa_int_regs_out;

	uint16_t* phys_fp_regs_in;
	uint16_t* phys_fp_regs_out;
	uint16_t* isa_fp_regs_in;
	uint16_t* isa_fp_regs_out;

	bool trapError;
	bool hasExecuted;
	bool hasIssued;
	bool hasRegistersAllocated;
	bool enduOpGroup;
	bool isFrontOfROB;

	const VanadisDecoderOptions* isa_options;
};

}
}

#endif
