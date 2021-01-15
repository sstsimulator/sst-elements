
#ifndef _H_VANADIS_BRANCH_UNIT
#define _H_VANADIS_BRANCH_UNIT

#include <sst/core/subcomponent.h>

#include <unordered_map>
#include <list>
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchUnit : public SST::SubComponent {

public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisBranchUnit)

	SST_ELI_DOCUMENT_PARAMS(
	)

	SST_ELI_DOCUMENT_STATISTICS(
	)

	VanadisBranchUnit( ComponentId_t id, Params& params ) : SubComponent(id) {}
	virtual ~VanadisBranchUnit() {}

	virtual void push( const uint64_t ins_addr, const uint64_t pred_addr ) = 0;
	virtual uint64_t predictAddress( const uint64_t addr ) = 0;
	virtual bool contains( const uint64_t addr ) = 0;

};

}
}

#endif
