#ifndef _busPlus_h
#define _busPlus_h

#include <sst_config.h>
#include <sst/core/component.h>
#include <mem/bus.hh>

class M5;
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
    struct linkInfo {
        MemLink* memLink;
        Port*    busPort;
    };

    linkInfo* addMemLink( M5*, const SST::Params& );

    std::deque<linkInfo*> m_links;
};

#endif
