
#ifndef _loadMemory_h
#define _loadMemory_h

#include <mem/physical.hh>
#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

namespace SST {
namespace M5 {


void loadMemory( std::string name, ::PhysicalMemory* realMemory, 
                    const SST::Params& params );

}
}
#endif
