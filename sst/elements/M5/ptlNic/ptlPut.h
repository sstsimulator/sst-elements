#include "ptlNic.h"

class PutCmd : public PtlNic::Cmd 
{
  public:

    PutCmd( PtlNic& nic, Context& ctx,  PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"\n");
    }

    bool work() {
        m_retval = m_ctx.put( (ptl_handle_md_t) m_event->args[0],
                                (ptl_size_t) m_event->args[1],
                                (ptl_size_t) m_event->args[2],
                                (ptl_ack_req_t) m_event->args[3],
                                *((ptl_process_t*) &m_event->args[4]),
                                (ptl_pt_index_t) m_event->args[5],
                                (ptl_match_bits_t) m_event->args[6],
                                (ptl_size_t) m_event->args[7],
                                (void*) m_event->args[8],
                                (ptl_hdr_data_t) m_event->args[9] );
        return true;
    }
};
