
#ifndef _loadMemory_h
#define _loadMemory_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/params.h>

class PhysicalMemory;

void loadMemory( std::string name, PhysicalMemory* realMemory, 
                    const SST::Params& params );
#endif
