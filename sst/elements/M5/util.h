#ifndef _util_h
#define _util_h

#include <cstdio>
#include <dll/gem5dll.hh>

namespace SST {
namespace M5 {

class M5;

typedef std::map< std::string, Gem5Object_t* > objectMap_t;

extern objectMap_t buildConfig( SST::M5::M5*, std::string, std::string, SST::Params& );

extern unsigned freq_to_ticks( std::string val );
extern unsigned latency_to_ticks( std::string val );

}
}

#endif
