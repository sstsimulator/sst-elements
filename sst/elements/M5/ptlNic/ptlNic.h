#ifndef _ptlNic_h
#define _ptlNic_h

#include <dev/io_device.hh>
#include "cmdQueue.h"

#include <debug.h>

using namespace std;
class M5;

struct PtlNicParams : public DmaDeviceParams
{
    M5*  m5Comp; 
    Addr startAddr;
};

class PtlNic : public DmaDevice 
{
  public:
    typedef PtlNicParams Params;
    PtlNic( const Params* p );
    virtual ~PtlNic();

    virtual Tick write(Packet*);
    virtual Tick read(Packet*);
    virtual void addressRanges(AddrRangeList&); 

  private:

    void process( void );
    void foo( int );

    cmdQueue_t      m_cmdQueue;

    uint64_t        m_startAddr;
    uint64_t        m_endAddr;

    M5*             m_comp;
    static const char* m_cmdNames[];
};

const char * PtlNic::m_cmdNames[] = CMD_NAMES;

#endif
