
#ifndef _H_VANADIS_OP_SYS
#define _H_VANADIS_OP_SYS

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include "inst/isatable.h"
#include "inst/regfile.h"
#include "os/vosresp.h"

namespace SST {
namespace Vanadis {

class VanadisCPUOSHandler : public SST::SubComponent {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisCPUOSHandler)

	VanadisCPUOSHandler( ComponentId_t id, Params& params ) :
		SubComponent(id) {

		const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
		output = new SST::Output("[os]: ", verbosity, 0, Output::STDOUT);
	}

	virtual ~VanadisCPUOSHandler() {
		delete output;
	}

	virtual void handleSysCall( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable ) = 0;
	virtual void handleReturn( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable, VanadisSysCallResponse* resp )  = 0;

protected:
	SST::Output* output;

};

}
}

#endif
