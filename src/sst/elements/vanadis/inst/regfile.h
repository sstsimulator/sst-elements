

#ifndef _H_VANADIS_REG_FILE
#define _H_VANADIS_REG_FILE

#include <sst/core/output.h>

#include "decoder/visaopts.h"

namespace SST {
namespace Vanadis {

class VanadisRegisterFile {

public:
	VanadisRegisterFile(
		const uint32_t thr,
		const VanadisDecoderOptions* decoder_ots,
		const uint16_t int_regs,
		const uint16_t fp_regs
		) : 	hw_thread(thr),
			count_int_regs(int_regs),
			count_fp_regs(fp_regs),
			decoder_opts(decoder_ots) {

		// Registers are always 64-bits
		int_reg_storage = new char[ 8 * int_regs ];
		fp_reg_storage  = new char[ 8 * fp_regs  ];

		for(int i = 0; i < (8 * int_regs); ++i) {
			int_reg_storage[i] = 0;
		}

		for(int i = 0; i < (8 * fp_regs); ++i) {
			fp_reg_storage[i] = 0;
		}
	}

	~VanadisRegisterFile() {
		delete[] int_reg_storage;
		delete[] fp_reg_storage;
	}

	const VanadisDecoderOptions* getDecoderOptions() const {
		return decoder_opts;
	}
 
	char* getIntReg( const uint16_t reg ) {
		return int_reg_storage + (8 * reg);
	}

	char* getFPReg( const uint16_t reg ) {
		return fp_reg_storage + (8 * reg);
	}

	void setIntReg( const uint16_t reg, const uint64_t val ) {
		if( reg != decoder_opts->getRegisterIgnoreWrites() ) {
			*((uint64_t*) &int_reg_storage[8*reg]) = val;
		}
	}

	void setIntReg( const uint16_t reg, const int64_t val ) {
		if( reg != decoder_opts->getRegisterIgnoreWrites() ) {
			*((int64_t*) &int_reg_storage[8*reg]) = val;
		}
	}

	void setFPReg( const uint16_t reg, const double val ) {
		if( reg != decoder_opts->getRegisterIgnoreWrites() ) {
			*((double*) &fp_reg_storage[8*reg]) = val;
		}
	}

	void setFPReg( const uint16_t reg, const float val ) {
		if( reg != decoder_opts->getRegisterIgnoreWrites() ) {
			*((float*) &fp_reg_storage[8*reg]) = val;
		}
	}

	uint32_t getHWThread() const { return hw_thread; }
	uint16_t countIntRegs() const { return count_int_regs; }
	uint16_t countFPRegs() const { return count_fp_regs; }

	void print(SST::Output* output) {
		output->verbose(CALL_INFO, 8, 0, "Integer Registers:\n");

		for( uint16_t i = 0; i < count_int_regs; ++i ) {
			printRegister(output, true, i);
		}

		output->verbose(CALL_INFO, 8, 0, "Floating Point Registers:\n");

		for( uint16_t i = 0; i < count_int_regs; ++i ) {
			printRegister(output, false, i);
		}
	}

private:
	void printRegister(SST::Output* output, bool isInt, uint16_t reg) {
		char* ptr = NULL;

		if( isInt ) {
			ptr = getIntReg(reg);
		} else {
			ptr = getFPReg(reg);
		}

		char* val_string = new char[65];
		val_string[64] = '\0';
		int index = 0;

		const long long int v = ((long long int*) ptr)[0];

		for( unsigned long long int i = 1L << 63L; i > 0; i = i / 2) {
			val_string[index++] = (v&i) ? '1' : '0';
		}

		output->verbose(CALL_INFO, 8, 0, "R[%5" PRIu16 "]: %s\n", reg, val_string);
		delete[] val_string;
	}

	const uint32_t hw_thread;
	const uint16_t count_int_regs;
	const uint16_t count_fp_regs;
	const VanadisDecoderOptions* decoder_opts;

	char* int_reg_storage;
	char* fp_reg_storage;
};

}
}

#endif