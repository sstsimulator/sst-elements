
#ifndef _H_VANADIS_MIPS_INST_LOADER
#define _H_VANADIS_MIPS_INST_LOADER

#include "decoder/vinsloader.h"

namespace SST {
namespace Vanadis {

class VanadisMIPSInstructionLoader : public VanadisInstructionLoader {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API( SST::Vanadis::VanadisMIPSInstructionLoader )
	

	VanadisMIPSInstructionLoader( ComponentId_t& id, Params& params ) :
		VanadisInstructionLoader(id, params) {

	}

	void hasEntryForAddress( const uint64_t ip ) const {
		return instr_map.find(ip) != instr_map.end();
	}

	

protected:
	

};

}
}

#endif
