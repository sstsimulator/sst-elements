#ifndef _ptlNicMMIF_h
#define _ptlNicMMIF_h

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/component.h>

#include "palacios.h"
#include "portals4/ptlNic/cmdQueue.h"
#include "M5/barrier.h"

class PtlNicMMIF : public SST::Component
{
  public:
    PtlNicMMIF( SST::ComponentId_t, Params_t& );
    virtual ~PtlNicMMIF();

  private:

    void ptlCmdRespHandler( SST::Event* );
    void dmaHandler( SST::Event* );

    void writeFunc( unsigned long );
    void barrierLeave();

    SST::Link*                  m_cmdLink;
    SST::Link*                  m_dmaLink;
    PalaciosIF*                 m_palaciosIF;
    
    unsigned long               m_barrierOffset;
    
    std::vector<uint8_t>        m_devMemory;
    cmdQueue_t*                 m_cmdQueue;
    WriteFunctor<PtlNicMMIF>    m_writeFunc;

    static const char          *m_cmdNames[];
    BarrierAction               m_barrier;

    BarrierAction::Handler<PtlNicMMIF> m_barrierCallback;
};

#endif
