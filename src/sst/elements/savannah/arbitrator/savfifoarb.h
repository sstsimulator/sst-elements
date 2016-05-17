
#ifndef _H_SST_SAVANNAH_IN_ORDER_ARB
#define _H_SST_SAVANNAH_IN_ORDER_ARB

#include <sst/core/output.h>
#include <sst/core/params.h>

namespace SST {
namespace Savannah {

class SavannahInOrderArbitrator : public SavannahIssueArbitrator {
public:
	SavannahInOrderArbitrator(Component* comp, Params& params) :
		SavannahIssueArbitrator(comp, params) {

		const int verbose = params.find_integer("verbose");
		output = new SST::Output("SavannahFIFOArb[@p:@l]: ",
			verbose, 0, SST::Output::STDOUT);

		maxIssuePerCycle = params.find_integer("max_issue_per_cycle");
	}

	~SavannahInOrderArbitrator() {
		delete output;
	}

	void issue(std::queue<SavannahRequestEvent*>& q, MemBackend* backend) {

	}

private:
	uint32_t maxIssuePerCycle;
	Output* output;

};

}
}

#endif
