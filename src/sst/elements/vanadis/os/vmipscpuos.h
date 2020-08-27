
#ifndef _H_VANADIS_MIPS_CPU_OS
#define _H_VANADIS_MIPS_CPU_OS

#include "os/vcpuos.h"

namespace SST {
namespace Vanadis {

class VanadisMIPSOSHandler : public VanadisCPUOSHandler {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisMIPSOSHandler,
		"vanadis",
		"VanadisMIPSOSHandler",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Provides SYSCALL handling for a MIPS-based decoding core",
		SST::Vanadis::VanadisCPUOSHandler
	)

	VanadisMIPSOSHandler( ComponentId_t id, Params& params ) :
		VanadisCPUOSHandler(id, params) {

	}

	virtual ~VanadisMIPSOSHandler() {}

	virtual void handleSysCall( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable ) {
		output->verbose(CALL_INFO, 8, 0, "System Call\n");
	}

	virtual void handleReturn( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable, VanadisSysCallResponse* resp ) {
		output->verbose(CALL_INFO, 8, 0, "System Call Return\n");
	}

protected:

};

}
}

#endif
