#ifndef _DRAMSimWrap2Plus_h
#define _DRAMSimWrap2Plus_h

#include <DRAMSimWrap2.h>

namespace SST {
namespace M5 {

class M5;
class MemLink;

struct DRAMSimWrap2PlusParams : public DRAMSimWrap2Params
{
    std::string linkName;
};

class MemLink;

class DRAMSimWrap2Plus : public DRAMSimWrap2
{
  public:
    DRAMSimWrap2Plus( const DRAMSimWrap2PlusParams* p );

  private:
    MemLink* m_link;
};

}
}
#endif

