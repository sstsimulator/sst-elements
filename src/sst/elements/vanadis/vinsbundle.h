
#ifndef _H_VANADIS_INST_BUNDLE
#define _H_VANADIS_INST_BUNDLE

#include <vector>
#include <cstdint>
#include <cinttypes>

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle {

public:
	VanadisInstructionBundle( const uint64_t addr ) : ins_addr(addr) {
		inst_bundle.reserve(1);
	}

	~VanadisInstructionBundle() {
		clear();
	}

	void clear() {
		for( VanadisInstruction* next_ins : inst_bundle ) {
			delete next_ins;
		}

		inst_bundle.clear();
	}

	uint32_t getInstructionCount() const {
		return inst_bundle.size();
	}

	void addInstruction( VanadisInstruction* newIns ) {
//		inst_bundle.push_back(newIns->clone() );
		inst_bundle.push_back(newIns->clone() );
	}

	VanadisInstruction* getInstructionByIndex( const uint32_t index ) {
//		return inst_bundle[index]->clone();
		return inst_bundle[index];
	}

	uint64_t getInstructionAddress() const {
		return ins_addr;
	}

private:
	const uint64_t ins_addr;
	std::vector<VanadisInstruction*> inst_bundle;
};

}
}

#endif
