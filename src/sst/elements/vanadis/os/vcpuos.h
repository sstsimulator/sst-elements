
#ifndef _H_VANADIS_OP_SYS
#define _H_VANADIS_OP_SYS

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include <functional>

#include "inst/isatable.h"
#include "inst/regfile.h"
#include "os/vosresp.h"
#include "inst/vsyscall.h"

namespace SST {
namespace Vanadis {

class VanadisCPUOSHandler : public SST::SubComponent {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisCPUOSHandler)

	VanadisCPUOSHandler( ComponentId_t id, Params& params ) :
		SubComponent(id) {

		const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
		output   = new SST::Output("[os]: ", verbosity, 0, Output::STDOUT);

		regFile  = nullptr;
		isaTable = nullptr;
		hw_thr   = 0;
		core_id  = 0;
	}

	virtual ~VanadisCPUOSHandler() {
		delete output;
	}

	void setCoreID( const uint32_t newCoreID ) { core_id = newCoreID; }
	void setHWThread( const uint32_t newThr ) { hw_thr = newThr; }
	void setRegisterFile( VanadisRegisterFile* newFile ) { regFile = newFile; }
	void setISATable( VanadisISATable* newTable ) { isaTable = newTable; }

	virtual void handleSysCall( VanadisSysCallInstruction* syscallIns ) = 0;
//	virtual void handleReturn( const uint32_t thr, VanadisRegisterFile* regFile, VanadisISATable* isaTable, VanadisSysCallResponse* resp )  = 0;

	virtual void registerReturnCallback( std::function<void(uint32_t)>& new_call_back ) {
		returnCallbacks.push_back( new_call_back );
	}

protected:
	SST::Output* output;
	std::vector< std::function<void(uint32_t)> > returnCallbacks;
	uint32_t core_id;
	uint32_t hw_thr;

	VanadisRegisterFile* regFile;
	VanadisISATable* isaTable;

};

}
}

#endif
