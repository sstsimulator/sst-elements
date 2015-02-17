
#ifndef _H_SST_ARIEL_TRACE_GEN
#define _H_SST_ARIEL_TRACE_GEN

#include <sst/core/module.h>

namespace SST {
namespace ArielComponent {

typedef enum {
	READ,
	WEITE
} ArielTraceEntryOperation;

class ArielTraceGenerator : public Module {

public:
	ArielTraceGenerator();
	~ArielTraceGenerator();
	void publishEntry(const uint64_t picoS,
		const uint64_t physAddr,
		const ArielTraceEntryOperation op);
	void setCoreID(uint32_t coreID);

};

}
}

#endif
