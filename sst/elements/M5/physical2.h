
#ifndef _physical2_h
#define _physical2_h

#include <mem/physical.hh>

namespace SST {
namespace M5 {

class PhysicalMemory2 : public PhysicalMemory 
{
  public:
    PhysicalMemory2( const Params* p ) : PhysicalMemory( p ) {} 
    // PhyscialMemory panics in init() if it is not connected
    void init() {}
};

}
}

#endif
