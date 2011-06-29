#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "dmaEvent.h"
#include "dmaEngine.h"

#include "recvEntry.h"
#include "ptlNic.h"
#include "ptlNicEvent.h"
#include "ptlNI.h"
#include "ptlPT.h"
#include "ptlMD.h"
#include "ptlME.h"
#include "ptlCT.h"
#include "ptlEQ.h"
#include "ptlProcess.h"
#include "ptlPut.h"
#include "ptlGet.h"

#include "trace.h"

const char * PtlNic::Cmd::m_cmdNames[] = CMD_NAMES;

PtlNic::PtlNic( SST::ComponentId_t id, Params_t& params ) :
    RtrIF( id, params ),
    m_dmaEngine( *this, params.find_integer( "nid") ),
    m_nid( params.find_integer("nid") ),
    m_vcInfoV( 2, *this ),
    m_contextV( 1, NULL )
{
    TRACE_ADD( PtlNic );
    TRACE_ADD( PtlCmd );
    TRACE_ADD( RecvEntry);
    TRACE_ADD( DmaBuf );

    PRINT_AT(PtlNic,"\n");

    for ( int i=0; i < m_vcInfoV.size(); i++ ) {
        m_vcInfoV[ i ].setVC( i );
    }

    registerTimeBase( "1ns" );
    registerClock( "500Mhz",
            new SST::Clock::Handler< PtlNic >( this, &PtlNic::clock ) );

    m_mmifLink = configureLink( "mmif",
           new SST::Event::Handler< PtlNic >( this, &PtlNic::mmifHandler ) );

    assert( m_mmifLink );
}

int PtlNic::Setup()
{
    return 0;
}

bool PtlNic::clock( SST::Cycle_t cycle )
{
    processPtlCmdQ();
    processVCs();
    processFromRtr();
    return false;
}

void PtlNic::processFromRtr()
{
    if ( ! toNicQ_empty( MsgVC ) ) {
        PRINT_AT(PtlNic,"\n");
        RtrEvent* event = toNicQ_front( MsgVC );
        toNicQ_pop( MsgVC );
        CtrlFlit* cFlit = (CtrlFlit*) event->packet.payload; 

        if ( cFlit->s.head ) {
            PRINT_AT(PtlNic,"got head Packet from nid %d\n",cFlit->s.nid);
            PtlHdr* hdr = (PtlHdr*) (cFlit + 1);
            assert( m_nidRecvEntryM.find( cFlit->s.nid ) == 
                                        m_nidRecvEntryM.end() );
            Context* ctx = findContext( hdr->dest_pid );
            assert(ctx);
            RecvEntry* entry = ctx->processHdrPkt( event->packet.payload );
            if ( entry ) {
                m_nidRecvEntryM[ cFlit->s.nid ] = entry;
            }
        } else {

            assert( m_nidRecvEntryM[ cFlit->s.nid ] );

            if ( m_nidRecvEntryM[ cFlit->s.nid ]->pushPkt(
                                (unsigned char*) ( cFlit + 1 ), 64 ) ) {
            
                PRINT_AT(PtlNic,"erase send entry for nid %d\n", cFlit->s.nid );
                m_nidRecvEntryM.erase( cFlit->s.nid );
            }
        }

        if ( cFlit->s.tail ) {
            PRINT_AT(PtlNic,"got tailPacket for nid %d\n",cFlit->s.nid);
            assert( m_nidRecvEntryM.find( cFlit->s.nid ) == 
                                        m_nidRecvEntryM.end() );
        }

        delete event;
    }
}

Context* PtlNic::findContext( ptl_pid_t pid )
{
    PRINT_AT(PtlNic,"targetPid=%d\n", pid );

    for ( int i = 0; i < m_contextV.size(); i++ ) {
        if ( m_contextV[i] ) {
            if ( m_contextV[i]->pid() == pid ) {
                return m_contextV[i];
            }
        }
    }
}

void PtlNic::mmifHandler( SST::Event* e )
{
    PtlNicEvent* event = static_cast< PtlNicEvent* >( e );
    switch( event->cmd ) {
      case ContextInit:
        contextInit( event );
        break;

      case ContextFini:
        contextFini( event );
        break;

      default:
        ptlCmd( event );
    }
}

void PtlNic::ptlCmd( PtlNicEvent* event )
{
    Context* ctx = getContext( event->context );
    assert( ctx );
    m_ptlCmdQ.push_back( Cmd::create( *this, *ctx, event ) );
}

void PtlNic::contextInit( PtlNicEvent* event )
{
    PRINT_AT(PtlNic,"uid=%d jid=%d\n",event->args[0], event->args[1]);
    int retval = allocContext( event->args[0], event->args[1] );
    m_mmifLink->Send( new PtlNicRespEvent( retval ) );
    delete event;
}

void PtlNic::contextFini( PtlNicEvent* event )
{
    int retval = freeContext( event->context );
    m_mmifLink->Send( new PtlNicRespEvent( retval ) );
    delete event;
}

ptl_pid_t PtlNic::allocPid( ptl_pid_t req_pid )
{
    if ( req_pid == PTL_PID_ANY ) {
        req_pid = 1;
    }
    return req_pid;
}

void PtlNic::processPtlCmdQ( )
{
    if ( !m_ptlCmdQ.empty() ) {
        Cmd* cmd = m_ptlCmdQ.front();

        if ( cmd->work() ) {
            PRINT_AT(PtlNic,"%s done\n", cmd->name().c_str());
            m_mmifLink->Send( new PtlNicRespEvent( cmd->retval() ) );
            delete cmd;
            m_ptlCmdQ.pop_front();
        }
    }
}

int PtlNic::allocContext(ptl_uid_t uid, ptl_jid_t jid )
{
    for ( int i = 0; i < m_contextV.size(); i++ ) {
        if ( ! m_contextV[i] ) {
            m_contextV[i] = new Context( this, uid, jid );
            return i;
        }
    }
    return -1;
}

int PtlNic::freeContext( int i )
{
    assert( i < m_contextV.size() );
    delete m_contextV[i];
    m_contextV[i] = 0;
    return 0;
}

Context* PtlNic::getContext( int i )
{
    assert( i < m_contextV.size() );
    return m_contextV[i];
}

PtlNic::Cmd* PtlNic::Cmd::create( PtlNic& nic, 
                    Context& ctx, PtlNicEvent* e ) 
{
    switch( e->cmd ) {
        case PtlNIInit:
            return new NIInitCmd(nic,ctx,e);
        case PtlNIFini:
            return new NIFiniCmd(nic,ctx,e);
        case PtlPTAlloc:
            return new PTAllocCmd(nic,ctx,e);
        case PtlPTFree:
            return new PTFreeCmd(nic,ctx,e);
        case PtlMDBind:
            return new MDBindCmd(nic,ctx,e);
        case PtlMDRelease:
            return new MDReleaseCmd(nic,ctx,e);
        case PtlMEAppend:
            return new MEAppendCmd(nic,ctx,e);
        case PtlMEUnlink:
            return new MEUnlinkCmd(nic,ctx,e);
        case PtlGetId:
            return new GetIdCmd(nic,ctx,e);
        case PtlCTAlloc:
            return new CTAllocCmd(nic,ctx,e);
        case PtlCTFree:
            return new CTFreeCmd(nic,ctx,e);
        case PtlEQAlloc:
            return new EQAllocCmd(nic,ctx,e);
        case PtlEQFree:
            return new EQFreeCmd(nic,ctx,e);
        case PtlPut:
            return new PutCmd(nic,ctx,e);
        case PtlGet:
            return new GetCmd(nic,ctx,e);
        default:
            abort();
    }
}

void PtlNic::processVCs()
{
    for ( int i = 0; i < m_vcInfoV.size(); i++ ) {
        m_vcInfoV[i].process();
    } 
}

bool PtlNic::sendMsg( ptl_nid_t nid, PtlHdr* hdr, Addr vaddr, ptl_size_t nbytes,
                                            CallbackBase* callback )
{
    PRINT_AT(PtlNic,"destNid=%d vaddr=%#lx nbytes=%lu\n", nid, vaddr, nbytes );
    SendEntry* entry = new SendEntry( nid, hdr, vaddr, nbytes, callback );

    assert( entry );

    m_vcInfoV[MsgVC].addMsg( entry );
    return false;
}

BOOST_CLASS_EXPORT( RtrEvent );
