#include "ptlNic.h"

class EQAllocCmd : public PtlNic::Cmd 
{
    enum { ArgCount, ArgEventPtr };
  public:
    EQAllocCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"eventPtr=%#lx count=%d\n",
                    m_event->args[ArgEventPtr],
                    m_event->args[ArgCount] );
    }

    bool work( ) {
        m_retval = m_ctx.allocEQ( m_event->args[ArgEventPtr],
                                   m_event->args[ArgCount]  );
        return true;
    }
};

class EQFreeCmd : public PtlNic::Cmd 
{
    enum { ArgHandle };
  public:
    EQFreeCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"ct_handle=%#x\n", m_event->args[ArgHandle] ); 
    }

    bool work() {
        m_retval = m_ctx.freeEQ( m_event->args[ ArgHandle ] ); 
        return true;
    }
};
