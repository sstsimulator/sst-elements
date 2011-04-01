#ifndef _DRAMSimWrapPlus_h
#define _DRAMSimWrapPlus_h

#include <DRAMSimWrap.h>

class M5;
class MemLink;

struct DRAMSimWrapPlusParams : public DRAMSimWrapParams
{
    std::string linkName;
};

class MemLink;

class DRAMSimWrapPlus : public DRAMSimWrap 
{
  public:
    DRAMSimWrapPlus( const DRAMSimWrapPlusParams* p );

  private:
    MemLink* m_link;
};

#endif

