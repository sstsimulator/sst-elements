#ifndef _system_h
#define _system_h

#include <sim/system.hh>

static inline System* create_System( std::string name, 
                    PhysicalMemory* physmem,
                    Enums::MemoryMode mem_mode) 
{
    
    System::Params& params = *new System::Params;
    
    params.name = name;
    params.physmem = physmem;
    params.mem_mode = mem_mode;

    return new System( &params );
}

#endif
