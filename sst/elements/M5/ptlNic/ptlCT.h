#include "ptlNic.h"

class CTAllocCmd : public PtlNic::Cmd 
{
    enum { ArgEventPtr };
  public:
    CTAllocCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"eventPtr=%#lx\n", m_event->args[ArgEventPtr] );
    }

    bool work( ) {
        m_retval = m_ctx.allocCT( m_event->args[ArgEventPtr] );
        return true;
    }
};

class CTFreeCmd : public PtlNic::Cmd 
{
    enum { ArgHandle };
  public:
    CTFreeCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"ct_handle=%#x\n", m_event->args[ArgHandle] ); 
    }

    bool work() {
        m_retval = m_ctx.freeCT( m_event->args[ ArgHandle ] ); 
        return true;
    }
};
