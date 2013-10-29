#ifndef _H_ARIEL_CPU
#define _H_ARIEL_CPU

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

#include <stdint.h>

#include <string>

#include "arielmemmgr.h"
#include "arielcore.h"

namespace SST {
namespace ArielComponent {

class ArielCPU : public SST::Component {

	public:
		ArielCPU(ComponentId_t id, Params& params);
		~ArielCPU();
		virtual void setup() {}
        virtual void finish() {}
        virtual bool tick( SST::Cycle_t );
        int forkPINChild(const char* app, char** args);
        	
    private:
    	SST::Output* output;
    	ArielMemoryManager* memmgr;
    	ArielCore** cpu_cores;
    	SST::Link** cpu_to_cache_links;
    	
    	uint32_t core_count;
		uint32_t memory_levels;
		
		uint64_t* page_sizes;
		uint64_t* page_counts;
		
		char* named_pipe_base;
		
		int* pipe_fds;
		
		bool stopTicking;

};

}
}

#endif
