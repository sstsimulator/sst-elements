#include "ptlNic.h"

class MDBindCmd : public PtlNic::Cmd 
{
    enum { ArgMd };
    enum { AllocMD, 
            ReadMD, 
            ReadMDWait } m_state;
  public:
    MDBindCmd( PtlNic& nic, Context& ctx,  PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e),
        m_state( AllocMD )
    {
        PRINT_AT(PtlCmd,"&md=%#lx\n", m_event->args[ArgMd] );
    }

    bool work() {
        switch (m_state) {
          case AllocMD:
            if ( allocMD() != Finished ) break;
            m_state = ReadMD;

          case ReadMD:
            if ( readMD() != Finished ) break;
            m_state = ReadMDWait;
          
          case ReadMDWait:
            if ( waitMD() != Finished ) break;
        }
        
        return m_done;
    }

  private:

    State readMD() {
        m_dmaCompletion = new Callback< MDBindCmd >
                                        ( this, &MDBindCmd::dmaDone );

        ptl_md_t *md = m_ctx.findMD( m_handle );
        m_nic.dmaEngine().read( m_event->args[ArgMd],
                                (uint8_t*) md,
                                sizeof( *md ),
                                m_dmaCompletion );
        return Finished;
    }

    State allocMD() {
        m_handle = m_ctx.allocMD();
        return Finished;
    }

    State waitMD(){
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

    Callback< MDBindCmd >*  m_dmaCompletion;
    int m_handle;
};

class MDReleaseCmd : public PtlNic::Cmd 
{
    enum { ArgMdHandle };
  public:
    MDReleaseCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
        PRINT_AT(PtlCmd,"md_handle=%#x\n", m_event->args[ArgMdHandle] ); 
    }

    bool work() {
        m_retval = m_ctx.freeMD( m_event->args[ArgMdHandle] );
        m_done = true;
        return m_done;
    }
};
