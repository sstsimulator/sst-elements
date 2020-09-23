
#ifndef _H_VANADIS_LSQ_BASE
#define _H_VANADIS_LSQ_BASE

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/subcomponent.h>
#include <sst/core/output.h>

#include "inst/vload.h"
#include "inst/vstore.h"
#include "inst/regfile.h"

#include <cinttypes>
#include <cstdint>
#include <vector>

using namespace SST::Interfaces;

namespace SST {
namespace Vanadis {

class VanadisLoadStoreQueue : public SST::SubComponent {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API( SST::Vanadis::VanadisLoadStoreQueue )

	SST_ELI_DOCUMENT_PARAMS(
		{ "verbose", 		"Set the verbosity of output for the LSQ", 	"0" },
	)

	VanadisLoadStoreQueue( ComponentId_t id, Params& params ) :
		SubComponent( id ) {

		uint32_t verbosity = params.find<uint32_t>("verbose");
		output = new SST::Output( "[lsq]: ", verbosity, 0, SST::Output::STDOUT );
	}

	virtual ~VanadisLoadStoreQueue() {
		delete output;
	}

	void setRegisterFiles( std::vector<VanadisRegisterFile*>* reg_f ) {
		registerFiles = reg_f;
	}

	virtual bool storeFull() = 0;
	virtual bool loadFull()  = 0;

	virtual size_t storeSize() = 0;
	virtual size_t loadSize()  = 0;

	virtual void push( VanadisStoreInstruction* store_me ) = 0;
	virtual void push( VanadisLoadInstruction* load_me   ) = 0;

	virtual void tick( uint64_t cycle ) = 0;

	virtual void processIncomingDataCacheEvent( SimpleMem::Request* ev ) = 0;
	virtual void clearLSQByThreadID( const uint32_t thread ) = 0;

	virtual void init( unsigned int phase ) = 0;
	virtual void setInitialMemory( const uint64_t address, std::vector<uint8_t>& payload ) = 0;

protected:
	std::vector<VanadisRegisterFile*>* registerFiles;
	SST::Output* output;

};

}
}

#endif
