#include "ptlNic.h"

class PTAllocCmd : public PtlNic::Cmd 
{
    enum { ArgOptions, ArgEqHandle, ArgReqPtl };
  public:
    PTAllocCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"options=%#lx eq_handle=%#lx pt_index_req=%d\n",
            m_event->args[ArgOptions],  
            m_event->args[ArgEqHandle],  
            m_event->args[ArgReqPtl] ); 
    }
    bool work() {
        m_retval = m_ctx.allocPT( m_event->args[ArgOptions],  
            m_event->args[ArgEqHandle],  
            m_event->args[ArgReqPtl] ); 
        return true;
    }
};

class PTFreeCmd : public PtlNic::Cmd 
{
    enum { ArgHandle };
  public:
    PTFreeCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"pt_index=%d\n", m_event->args[ArgHandle] ); 
    }

    bool work() {
        m_retval = m_ctx.freePT( m_event->args[ArgHandle] ); 
        return true;
    }
};
