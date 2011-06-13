#include "ptlNic.h"
class NIInitCmd : public PtlNic::Cmd 
{
    enum { Options, Pid, Desired, Actual, MapSize, DM, AM }; 
    enum { AllocPid,
            ReadDesiredLimits,
            WaitDesiredLimits,
            WriteActualLimits,
            WaitActualLimits  } m_state;
  public:
    NIInitCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e),
        m_state( AllocPid )
    {
        PRINT_AT(PtlCmd,"options=%#lx pid=%d desired=%#lx actual=%#lx "
                            "map_size=%#lx dm=%#lx am=%#lx\n",
            m_event->args[Options],
            m_event->args[Pid],
            m_event->args[Desired],
            m_event->args[Actual],
            m_event->args[MapSize],
            m_event->args[DM],
            m_event->args[AM]);
    
            if ( m_event->args[Desired] )
                m_fff.push_back( Desired );
            if ( m_event->args[Actual] )
                m_fff.push_back( Actual );
            if ( m_event->args[DM] )
                m_fff.push_back( Actual );
            if ( m_event->args[AM] )
                m_fff.push_back( Actual );

        m_ctx.initOptions( m_event->args[Options] );
    }

    bool work() {
        switch ( m_state ) {
          case AllocPid:
            if ( allocPid( ) != Finished ) break;
            m_state = WriteActualLimits;
#if 0
          case ReadDesiredLimits:         
            if ( readLimits() != Finished ) break;
            m_state = WaitDesiredLimits;

          case WaitDesiredLimits:
            if ( waitLimits() != Finished ) break;
            m_state = WriteActualLimits;
#endif

          case WriteActualLimits:         
            if ( writeLimits() != Finished ) break;
            m_state = WaitActualLimits;

          case WaitActualLimits:
            if ( waitLimits() != Finished ) break;

        }
        return m_done;
    }

  private:

    State allocPid( ) {
        int pid = m_nic.allocPid( m_event->args[Pid] );
        if ( pid < 0 ) { 
            m_done = true;
            m_retval = -pid;
        } else {
            m_ctx.initPid(pid);
        }
        return Finished; 
    }
    
    State readLimits() {
        m_dmaCompletion = new Callback< NIInitCmd >
                        ( this, &NIInitCmd::dmaDone );

        m_nic.dmaEngine().read( m_event->args[Desired], 
                                (uint8_t*)&m_limits, 
                                sizeof( m_limits ),
                                m_dmaCompletion);
        return Finished;
    }

    State writeLimits() {
        m_dmaCompletion = new Callback< NIInitCmd >
                        ( this, &NIInitCmd::dmaDone );

        ptl_ni_limits_t* limits = m_ctx.limits();
        m_nic.dmaEngine().write( m_event->args[Actual], 
                                (uint8_t*) limits,
                                sizeof( *limits ),
                                m_dmaCompletion);
        return Finished;
    }

    State waitLimits() {
        if ( ! m_dmaCompletion->done ) return NotFinished;
        PRINT_AT(PtlCmd,"\n");
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

    std::list< int > m_fff;
    ptl_ni_limits_t  m_limits;
    Callback<NIInitCmd>*        m_dmaCompletion;
};

class NIFiniCmd : public PtlNic::Cmd 
{
  public:
    NIFiniCmd( PtlNic& nic, Context& ctx, PtlNicEvent* e ) :
        PtlNic::Cmd(nic,ctx,e)
    {
    }
    bool work() {
        m_retval = 0;
        return true;
    }
};
