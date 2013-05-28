// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization/element.h>

#include "dmaEvent.h"
#include "dmaEngine.h"

#include "recvEntry.h"
#include "ptlNic.h"
#include "ptlNicEvent.h"
#include "./debug.h"

using namespace SST::Portals4;

const char * PtlNic::m_cmdNames[] = CMD_NAMES;

PtlNic::PtlNic( SST::ComponentId_t id, Params_t& params ) :
    RtrIF( id, params ),
    m_nid( params.find_integer("nid") ),
    m_dmaEngine( *this, params ),
    m_vcInfoV( 2, *this )
{
    TRACE_ADD( PtlNic );
    TRACE_ADD( PtlCmd );
    TRACE_ADD( RecvEntry);
    TRACE_ADD( DmaBuf );
    TRACE_ADD( DmaEngine );
    TRACE_INIT();

    PtlNic_DBG("\n");

    for ( unsigned int i=0; i < m_vcInfoV.size(); i++ ) {
        m_vcInfoV[ i ].setVC( i );
    }

    std::string freq = params.find_string( "clock" );

    printf("%s: freq = %s\n",__func__,freq.c_str());
    assert ( ! freq.empty() );

    registerClock( freq,
            new SST::Clock::Handler< PtlNic >( this, &PtlNic::clock ) );

    m_mmifLink = configureLink( "mmif",
           new SST::Event::Handler< PtlNic >( this, &PtlNic::mmifHandler ) );

    assert( m_mmifLink );
}

void PtlNic::setup()
{
}

bool PtlNic::clock( SST::Cycle_t cycle )
{
    processVCs();
    processFromRtr();
#if USE_DMA_LIMIT_BW
    m_dmaEngine.clock();
#endif
    return false;
}

void PtlNic::processFromRtr()
{
    if ( ! toNicQ_empty( MsgVC ) ) {
        RtrEvent* event = toNicQ_front( MsgVC );
        toNicQ_pop( MsgVC );
        CtrlFlit* cFlit = (CtrlFlit*) event->packet.payload; 

        if ( cFlit->s.head ) {
            PtlHdr* hdr = (PtlHdr*) (cFlit + 1);
            PtlNic_DBG("got head packet from nid %d dest_pid=%d\n",
                            cFlit->s.nid,hdr->dest_pid );
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
            
                PtlNic_DBG("push packet\n");
                if ( m_nidRecvEntryM[ cFlit->s.nid ]->pushPkt(
                                (unsigned char*) ( cFlit + 1 ), 64 ) ) {
            
                    PtlNic_DBG("erase send entry for nid %d\n", cFlit->s.nid );
                    m_nidRecvEntryM.erase( cFlit->s.nid );
                }
            } else {
                PtlNic_DBG("drop packet\n");
            }
        }

        if ( cFlit->s.tail ) {
            PtlNic_DBG("got tailPacket for nid %d\n",cFlit->s.nid);
            assert( m_nidRecvEntryM.find( cFlit->s.nid ) == 
                                        m_nidRecvEntryM.end() );
        }

        delete event;
    }
}

Context* PtlNic::findContext( ptl_pid_t pid )
{
    PtlNic_DBG("targetPid=%d\n", pid );

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
    PtlNic_DBG("cmd=%s\n",m_cmdNames[event.cmd().type]);

    switch( event.cmd().type ) {

      case ContextInitCmd:
        allocContext( event.cmd().ctx_id, event.cmd().u.ctxInit );
        break;

      case ContextFiniCmd:
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

        case PtlNIInitCmd:
            ctx->NIInit( event.cmd().u.niInit );
            break;

        case PtlNIFiniCmd:
            ctx->NIFini( event.cmd().u.niFini );
            break;

        case PtlPTAllocCmd:
            ctx->PTAlloc( event.cmd().u.ptAlloc );
            break;

        case PtlPTFreeCmd:
            ctx->PTFree( event.cmd().u.ptFree );
            break;

        case PtlMDBindCmd:
            ctx->MDBind( event.cmd().u.mdBind );
            break;

        case PtlMDReleaseCmd:
            ctx->MDRelease( event.cmd().u.mdRelease );
            break;

        case PtlMEAppendCmd:
            ctx->MEAppend( event.cmd().u.meAppend );
            break;

        case PtlMEUnlinkCmd:
            ctx->MEUnlink( event.cmd().u.meUnlink );
            break;

        case PtlCTAllocCmd:
            ctx->CTAlloc( event.cmd().u.ctAlloc );
            break;

        case PtlCTFreeCmd:
            ctx->CTFree( event.cmd().u.ctFree );
            break;

        case PtlEQAllocCmd:
            ctx->EQAlloc( event.cmd().u.eqAlloc );
            break;

        case PtlEQFreeCmd:
            ctx->EQFree( event.cmd().u.eqFree );
            break;

        case PtlPutCmd:
            ctx->Put( event.cmd().u.ptlPut );
            break;

        case PtlGetCmd:
            ctx->Get( event.cmd().u.ptlGet );
            break;

        case PtlTrigGetCmd:
            ctx->TrigGet( event.cmd().u.ptlGet );
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
    PtlNic_DBG(  "ctx=%d uid=%d ptlPtr=%p\n", ctx, cmd.uid, cmd.nidPtr );
    assert( m_ctxM.find( ctx ) == m_ctxM.end() );
    m_ctxM[ctx] = new Context( this, cmd );
}

void PtlNic::freeContext( ctx_id_t ctx,  cmdContextFini_t& cmd )
{
    PtlNic_DBG(  "ctx=%d\n",ctx);
    assert( m_ctxM.find( ctx ) != m_ctxM.end() );
    delete m_ctxM[ctx];
    m_ctxM.erase(ctx);
}

Context* PtlNic::getContext( ctx_id_t ctx )
{
    PtlNic_DBG(  "ctx=%d\n",ctx);
    assert( m_ctxM.find( ctx ) != m_ctxM.end() );
    return m_ctxM[ctx];
}

void PtlNic::processVCs()
{
    for ( unsigned int i = 0; i < m_vcInfoV.size(); i++ ) {
        m_vcInfoV[i].process();
    } 
}

bool PtlNic::sendMsg( ptl_nid_t nid, PtlHdr* hdr, Addr vaddr, ptl_size_t nbytes,
                                            CallbackBase* callback )
{
    PtlNic_DBG("destNid=%d vaddr=%#lx nbytes=%lu\n", nid, vaddr, nbytes );
    SendEntry* entry = new SendEntry( nid, hdr, vaddr, nbytes, callback );

    assert( entry );

    m_vcInfoV[MsgVC].addMsg( entry );
    return false;
}

