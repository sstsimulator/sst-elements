
#ifndef _H_SST_ARIEL_TRACE_GEN
#define _H_SST_ARIEL_TRACE_GEN

#include <sst/core/module.h>

namespace SST {
namespace ArielComponent {

typedef enum {
	READ,
	WRITE
} ArielTraceEntryOperation;

class ArielTraceGenerator : public Module {

public:
	ArielTraceGenerator() {}
	~ArielTraceGenerator() {}
	virtual void publishEntry(const uint64_t picoS,
		const uint64_t physAddr,
		const uint32_t reqLength,
		const ArielTraceEntryOperation op) = 0;
	virtual void setCoreID(uint32_t coreID) = 0;

};

}
}

#endif
