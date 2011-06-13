#include "ptlNic.h"

class MEAppendCmd : public PtlNic::Cmd 
{
    enum { ArgPT,
            ArgMd,
            ArgList,
            ArgUserPtr };
    enum { ValidateArgs,
            AllocME, 
            ReadME, 
            ReadMEWait,
            link } m_state;
  public:
    MEAppendCmd( PtlNic& nic, Context& ctx,  PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e),
        m_state( ValidateArgs )
    {
        PRINT_AT(PtlCmd,"portal=%d &md=%#lx list=%d userPtr %#lx\n", 
                    m_event->args[ArgPT],
                    m_event->args[ArgMd],
                    m_event->args[ArgList],
                    m_event->args[ArgUserPtr] );
    }

    bool work() {
        switch (m_state) {
          case ValidateArgs:
            m_state = AllocME; 
          case AllocME:
            if ( allocME() != Finished ) break;
            m_state = ReadME;

          case ReadME:
            if ( readME() != Finished ) break;
            m_state = ReadMEWait;
          
          case ReadMEWait:
            if ( waitME() != Finished ) break;
        }
        
        return m_done;
    }

  private:

    State readME() {
        m_dmaCompletion = new Callback< MEAppendCmd >
                                        ( this, &MEAppendCmd::dmaDone );

        ptl_me_t *me = m_ctx.findME( m_handle );
        m_nic.dmaEngine().read( m_event->args[ArgMd],
                                (uint8_t*) me,
                                sizeof( *me ),
                                m_dmaCompletion );
        return Finished;
    }

    State allocME() {
        m_handle = m_ctx.allocME( (ptl_pt_index_t) m_event->args[ArgPT],
                        (ptl_list_t) m_event->args[ArgList],
                        (void*) m_event->args[ArgUserPtr]);
        if ( m_handle < 0 ) {
            m_done = true;
            return NotFinished;
        }
        return Finished;
    }

    State waitME(){
        if ( ! m_dmaCompletion->done ) return NotFinished;
        m_retval = PTL_OK;
        m_done = true;
        delete m_dmaCompletion;
        return Finished;
    }

    bool dmaDone( void* ) {
        PRINT_AT(PtlCmd,"\n");
        m_dmaCompletion->done = true;
        return false;
    }

    Callback< MEAppendCmd >*  m_dmaCompletion;
    int m_handle;
};

class MEUnlinkCmd : public PtlNic::Cmd 
{
    enum { ArgMdHandle };
  public:
    MEUnlinkCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"md_handle=%#x\n", m_event->args[ArgMdHandle] ); 
    }

    bool work() {
        m_retval = m_ctx.freeME( m_event->args[ArgMdHandle] );
        m_done = true;
        return m_done;
    }
};
