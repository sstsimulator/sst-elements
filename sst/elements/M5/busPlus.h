#ifndef _busPlus_h
#define _busPlus_h

#include <sst_config.h>
#include <sst/core/component.h>
#include <mem/bus.hh>

namespace SST {
namespace M5 {

class M5;
class MemLink;

struct BusPlusParams : public BusParams
{
    M5*         m5Comp;
    SST::Params params;
};

class MemLink;

class BusPlus : public Bus
{
  public:
    BusPlus( const BusPlusParams* p );

  private:
    std::deque<MemLink*> m_links;
};

}
}

#endif
