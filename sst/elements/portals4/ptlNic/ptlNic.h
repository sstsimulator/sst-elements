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

#ifndef _ptlNic_h
#define _ptlNic_h

#include "ptlNicEvent.h"
#include "portals4.h"
#include "dmaEngine.h"
#include "context.h"
#include "callback.h"

#include "sst/elements/SS_router/SS_router/RtrIF.h"

using namespace SST::SS_router;

namespace SST {
namespace Portals4 {

#define PtlNic_DBG( fmt, args... ) {\
    char _tmp[16]; \
    sprintf(_tmp,"%d:",m_nid); \
    _PRINT_AT( PtlNic, _tmp, fmt, ##args ); \
}\

struct PtlHdr;

class PtlNic : public RtrIF 
{
    class SendEntry {
      public:
        SendEntry( ptl_nid_t nid, PtlHdr* hdr, Addr vaddr, ptl_size_t length,
                                            CallbackBase* callback ) :
            m_destNid( nid ),
            m_hdr( hdr ),
            m_vaddr( vaddr ),
            m_length( length ),
            m_callback( callback ), 
            m_offset( 0 ),
            m_dmaed( 0 ),
            m_hdrDone( false )
        {}

        bool hdrDone() { return m_hdrDone; } 
        void setHdrDone() { m_hdrDone = true; } 
        ptl_nid_t destNid() { return m_destNid; }
        ptl_size_t bytesLeft( ) { return m_length - m_offset; }
        bool dmaDone() { return (m_length == m_dmaed ); }
        Addr vaddr() { return m_vaddr + m_offset; }
        void incOffset( size_t nbytes ) { m_offset += nbytes; }
        void incDmaed( size_t nbytes ) { m_dmaed += nbytes; }
        CallbackBase* callback() { return m_callback; }
        PtlHdr* ptlHdr() { return m_hdr; }

      private:
        ptl_nid_t       m_destNid;
        PtlHdr*         m_hdr;
        Addr            m_vaddr;
        ptl_size_t      m_length; 
        CallbackBase*   m_callback;
        ptl_size_t      m_offset; 
        ptl_size_t      m_dmaed; 
        bool            m_hdrDone;
    };

    class VCInfo {

        struct XXX {
            ptl_nid_t   destNid;            
            size_t      offset;
            bool        head;
            bool        tail;
            std::vector<unsigned char>  buf; 
            Callback< VCInfo, XXX >*    callback;
        }; 

        typedef Callback<VCInfo, XXX> XXXCallback;

      public:
        VCInfo( PtlNic& );
        void addMsg( SendEntry* );
        void process();
        void setVC(int);

      private:    
        void processRtrQ();
        void processSendQ();
        bool dmaCallback( XXX* );

        std::deque< SendEntry* > m_sendQ;
        std::deque< XXX* >       m_rtrQ;
        PtlNic&                  m_nic;
        RtrEvent*                m_rtrEvent;
        int                      m_vc;
    };

  public: 
    static const int MsgVC = 0;

    struct RtrXfer {
        int             dest;
        void*           buf;
        size_t          nbytes;
    };

    typedef int ctx_id_t; 

  public:
    PtlNic( SST::ComponentId_t id, Params_t& params );
    ptl_pid_t allocPid( ptl_pid_t );
    DmaEngine& dmaEngine() { return m_dmaEngine; }
    int& nid() { return m_nid; }
    bool sendMsg( ptl_nid_t, PtlHdr*, Addr, ptl_size_t, CallbackBase* );

  private:
    void allocContext( ctx_id_t, cmdContextInit_t& );
    void freeContext( ctx_id_t, cmdContextFini_t& );
    Context* getContext( ctx_id_t );
    Context* findContext( ptl_pid_t pid );
    void processVCs();
    void processFromRtr();
//    int Setup();  // Renamed per Issue 70 - ALevine
    void setup();
    bool clock( SST::Cycle_t cycle );
    void rtrHandler( SST::Event* );
    void mmifHandler( SST::Event* );
    void ptlCmd( PtlNicEvent& );

    typedef std::map<ptl_nid_t,RecvEntry*>  nidRecvEntryM_t;
         
    int                     m_nid;
    SST::Link*              m_mmifLink;
    typedef std::map<ctx_id_t,Context*> ctxMap_t;
    ctxMap_t                m_ctxM;
    DmaEngine               m_dmaEngine;
    std::vector< VCInfo >   m_vcInfoV;
    nidRecvEntryM_t         m_nidRecvEntryM;
    static const char*  m_cmdNames[];
};

}
}

#endif
