#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "dmaEvent.h"
#include "dmaEngine.h"

#include "recvEntry.h"
#include "ptlNic.h"
#include "ptlNicEvent.h"

#include "trace.h"

const char * PtlNic::m_cmdNames[] = CMD_NAMES;


PtlNic::PtlNic( SST::ComponentId_t id, Params_t& params ) :
    RtrIF( id, params ),
    m_dmaEngine( *this, params.find_integer( "nid") ),
    m_nid( params.find_integer("nid") ),
    m_vcInfoV( 2, *this )
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
    processVCs();
    processFromRtr();
    return false;
}

void PtlNic::processFromRtr()
{
    if ( ! toNicQ_empty( MsgVC ) ) {
        RtrEvent* event = toNicQ_front( MsgVC );
        toNicQ_pop( MsgVC );
        CtrlFlit* cFlit = (CtrlFlit*) event->packet.payload; 

        if ( cFlit->s.head ) {
            PRINT_AT(PtlNic,"got head packet from nid %d\n",cFlit->s.nid);
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

            if( m_nidRecvEntryM.find( cFlit->s.nid ) != 
                                        m_nidRecvEntryM.end() ) {
            
                PRINT_AT(PtlNic,"push packet\n");
                if ( m_nidRecvEntryM[ cFlit->s.nid ]->pushPkt(
                                (unsigned char*) ( cFlit + 1 ), 64 ) ) {
            
                    PRINT_AT(PtlNic,"erase send entry for nid %d\n", cFlit->s.nid );
                    m_nidRecvEntryM.erase( cFlit->s.nid );
                }
            } else {
                PRINT_AT(PtlNic,"drop packet\n");
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

    ctxMap_t::iterator iter = m_ctxM.begin();

    for (; iter != m_ctxM.end(); ++iter ) {
        if ( (*iter).second->pid() == pid ) {
                return (*iter).second;
        }
    }
    return NULL;
}

void PtlNic::mmifHandler( SST::Event* e )
{
    PtlNicEvent& event = *static_cast< PtlNicEvent* >( e );
    PRINT_AT(PtlNic,"cmd=%s\n",m_cmdNames[event.cmd().type]);

    switch( event.cmd().type ) {

      case ContextInit:
        allocContext( event.cmd().ctx_id, event.cmd().u.ctxInit );
        break;

      case ContextFini:
        freeContext( event.cmd().ctx_id, event.cmd().u.ctxFini );
        break;

      default:
        ptlCmd( event );
    }
    delete e;
}

void PtlNic::ptlCmd( PtlNicEvent& event )
{
    Context* ctx = getContext( event.cmd().ctx_id );
    assert( ctx );

    switch( event.cmd().type ) {

        case PtlNIInit:
            ctx->NIInit( event.cmd().u.niInit );
            break;

        case PtlNIFini:
            ctx->NIFini( event.cmd().u.niFini );
            break;

        case PtlPTAlloc:
            ctx->PTAlloc( event.cmd().u.ptAlloc );
            break;

        case PtlPTFree:
            ctx->PTFree( event.cmd().u.ptFree );
            break;

        case PtlMDBind:
            ctx->MDBind( event.cmd().u.mdBind );
            break;

        case PtlMDRelease:
            ctx->MDRelease( event.cmd().u.mdRelease );
            break;

        case PtlMEAppend:
            ctx->MEAppend( event.cmd().u.meAppend );
            break;

        case PtlMEUnlink:
            ctx->MEUnlink( event.cmd().u.meUnlink );
            break;

        case PtlCTAlloc:
            ctx->CTAlloc( event.cmd().u.ctAlloc );
            break;

        case PtlCTFree:
            ctx->CTFree( event.cmd().u.ctFree );
            break;

        case PtlEQAlloc:
            ctx->EQAlloc( event.cmd().u.eqAlloc );
            break;

        case PtlEQFree:
            ctx->EQFree( event.cmd().u.eqFree );
            break;

        case PtlPut:
            ctx->Put( event.cmd().u.ptlPut );
            break;

        case PtlGet:
            ctx->Get( event.cmd().u.ptlGet );
            break;

        default:
            assert(0);
    }
}

ptl_pid_t PtlNic::allocPid( ptl_pid_t req_pid )
{
    if ( req_pid == PTL_PID_ANY ) {
        req_pid = 1;
    }
    return req_pid;
}

void PtlNic::allocContext( ctx_id_t ctx, cmdContextInit_t& cmd )
{
    PRINT_AT( PtlNic, "ctx=%d uid=%d jid=%d ptlPtr=%p\n",
                    ctx, cmd.uid, cmd.jid, cmd.nidPtr );
    assert( m_ctxM.find( ctx ) == m_ctxM.end() );
    m_ctxM[ctx] = new Context( this, cmd );
}

void PtlNic::freeContext( ctx_id_t ctx,  cmdContextFini_t& cmd )
{
    PRINT_AT( PtlNic, "ctx=%d\n",ctx);
    assert( m_ctxM.find( ctx ) != m_ctxM.end() );
    delete m_ctxM[ctx];
    m_ctxM.erase(ctx);
}

Context* PtlNic::getContext( ctx_id_t ctx )
{
    PRINT_AT( PtlNic, "ctx=%d\n",ctx);
    assert( m_ctxM.find( ctx ) != m_ctxM.end() );
    return m_ctxM[ctx];
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
