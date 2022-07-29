// -*- mode: c++ -*-
// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef COMPONENTS_MEMHIERARCHY_STANDARD_CACHEINTERFACE
#define COMPONENTS_MEMHIERARCHY_STANDARD_CACHEINTERFACE

#include <string>
#include <utility>
#include <map>
#include <queue>

#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST {

class Component;
class Event;


namespace MemHierarchy {

class MemEventBase;
class MemEvent;

/** Class is used to interface a compute mode (CPU, GPU) to MemHierarchy */
/*
 * Notes on ports and connectivity:
 *  This subcomponent translates a standard event type (StandardMem::Request) into an internal MemHierarchy event type. 
 *  It can be loaded by components to provide an interface into the memory system.
 *  
 *  - If this subcomponent is loaded anonymously, the component loading it must connect a port and pass that port name via the "port" parameter. 
 *    The loading component must share that port with the subcomponent (via SHARE_PORTS flag). Anonymous loading does not support a SimpleNetwork connection.
 *  - If this subcomponent is not loaded anonymously, the subcomponent can connect either directly to a memHierarchy component (default) or to a SimpleNetwork interface such as Merlin or Kingsley
 *  - Ways to connect
 *      - Options to connect directly to another memHierarchy component (pick ONE):
 *          a. Load this subcomponent anonymously with the SHARE_PORTS flag and set the "port" parameter to a connected port owned by the parent component
 *          b. Load this subcomponent explicitly in the input file and connect the 'port' port to either a memHierarchy component's port, 
 *             or to a MemLink subcomponent belonging to a memHierarchy component. Do not use the "port" parameter or fill the "memlink" subcomponent slot.
 *          c. Load this subcomponent explicitly in the input file and fill the memlink subcomponent slot with "memHierarchy.MemLink". 
 *             Connect the memlink subcomponent's port. Do not connect this subcomponent's port or use the "port" parameter.
 *      - To connect over a network
 *          - Load a MemNIC (e.g., MemNIC, MemNICFour) into the "memlink" subcomponent and parameterize appropriately. Do not connect the "port" port or use the "port" parameter.
 * 
 * Notes on using this interface
 *  - The parent component MUST call init(), setup(), and finish() on this subcomponent during each of SST's respective phases. In particular, failing to call init() will lead to errors.
 *
 *
 */
class StandardInterface : public Interfaces::StandardMem {

public:
    friend class MemEventConverter;

/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(StandardInterface, "memHierarchy", "standardInterface", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Interface to memory hierarchy between endpoint and cache. Converts StandardMem requests into MemEventBases.", SST::Interfaces::StandardMem)
    
    SST_ELI_DOCUMENT_PARAMS( 
        {"verbose",     "(uint) Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]", "1"},
        {"debug",       "(uint) Where to send debug output. Options: 0[none], 1[stdout], 2[stderr], 3[file]", "0"},
        {"debug_level", "(uint) Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},
        {"port",        "(string) port name to use for interfacing to the memory system. This must be provided if this subcomponent is being loaded anonymously. Otherwise this should not be specified and either the 'port' port should be connected or the 'memlink' subcomponent slot should be filled"}
    )

    SST_ELI_DOCUMENT_PORTS( {"port", "Port to memory hierarchy (caches/memory/etc.). Required if subcomponent slot not filled or if 'port' parameter not provided.", {}} )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"memlink", "Link manager, optional - if not filled this subcompoonent's port should be connected", "SST::MemHierarchy::MemLinkBase"})

/* Begin class definition */
    StandardInterface(SST::ComponentId_t id, Params &params, TimeConverter* time, HandlerBase* handler = NULL);

    /* Begin API to Parent */

    virtual Addr getLineSize() override { return lineSize_; }
    virtual void setMemoryMappedAddressRegion(Addr start, Addr size) override;
    
    // Send/Recv during init()/complete() phase
    virtual void sendUntimedData(Request *req) override;
    virtual StandardMem::Request* recvUntimedData() override;

    // Send/recv during simulation
    virtual void send(Request* req) override;
    virtual Request* poll() override;

    // SST simulation life cycle hooks - parent must call these
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;

    /* End API to Parent */
    
protected:

    Output      output;
    Output      debug;
    int         dlevel;

    Addr        baseAddrMask_;
    Addr        lineSize_;
    std::string rqstr_;
    std::map<MemEventBase::id_type, std::pair<StandardMem::Request*,Command>> requests_;   /* Map requests sent by the endpoint */
    std::map<StandardMem::Request::id_t, MemEventBase*> responses_;     /* Map requests received by the endpoint */
    SST::MemHierarchy::MemLinkBase*  link_;
    bool cacheDst_; // Whether we've got a cache below us to handle certain conversions or we need to 

    bool initDone_;
    std::queue<MemEventInit*> initSendQueue_;

    MemRegion region;   // For MMIO
    Endpoint epType;    // Endpoint type -> CPU or MMIO 
    
    class MemEventConverter : public Interfaces::StandardMem::RequestConverter {
    public:
        MemEventConverter(StandardInterface* iface) : iface(iface) {}
        virtual ~MemEventConverter() {}
        virtual SST::Event* convert(StandardMem::Read* req) override;
        virtual SST::Event* convert(StandardMem::ReadResp* req) override;
        virtual SST::Event* convert(StandardMem::Write* req) override;
        virtual SST::Event* convert(StandardMem::WriteResp* req) override;
        virtual SST::Event* convert(StandardMem::FlushAddr* req) override;
        virtual SST::Event* convert(StandardMem::FlushResp* req) override;
        virtual SST::Event* convert(StandardMem::ReadLock* req) override;
        virtual SST::Event* convert(StandardMem::WriteUnlock* req) override;
        virtual SST::Event* convert(StandardMem::LoadLink* req) override;
        virtual SST::Event* convert(StandardMem::StoreConditional* req) override;
        virtual SST::Event* convert(StandardMem::MoveData* req) override;
        virtual SST::Event* convert(StandardMem::CustomReq* req) override;
        virtual SST::Event* convert(StandardMem::CustomResp* req) override;
        virtual SST::Event* convert(StandardMem::InvNotify* req) override;

        void debugChecks(MemEvent* ev);

        SST::Output output;
        StandardInterface* iface;
    };

private:

    /** Callback handler for our link
     * Parses event and calls the parent's handler */
    void receive(SST::Event *ev);

    /** Convert MemEvents into updated Requests*/
    Interfaces::StandardMem::Request* processIncoming(MemEventBase *ev);


    /** Conversion functions to convert internal memHierarchy events to StandardMem::Requests
     * Called by receive() on incoming requests
     */
    StandardMem::Request* convertResponseGetSResp(StandardMem::Request* req, MemEventBase* meb);
    StandardMem::Request* convertResponseWriteResp(StandardMem::Request* req, MemEventBase* meb);
    StandardMem::Request* convertResponseFlushResp(StandardMem::Request* req, MemEventBase* meb);
    StandardMem::Request* convertResponseAckMove(StandardMem::Request* req, MemEventBase* meb);
    StandardMem::Request* convertResponseCustomResp(StandardMem::Request* req, MemEventBase* meb);
    StandardMem::Request* convertRequestInv(MemEventBase* req);
    StandardMem::Request* convertRequestGetS(MemEventBase* req);
    StandardMem::Request* convertRequestWrite(MemEventBase* req);
    StandardMem::Request* convertRequestCustom(MemEventBase* req);
    StandardMem::Request* convertRequestLL(MemEventBase* req);
    StandardMem::Request* convertRequestSC(MemEventBase* req);
    StandardMem::Request* convertRequestLock(MemEventBase* req);
    StandardMem::Request* convertRequestUnlock(MemEventBase* req);
    
    /* Handling NACKs
     * These do not occur if the interface is linked to a cache. 
     * However, if the endpoint issues coherent accesses (not flagged F_NONCACHEABLE), is not linked to a cache,
     * and there are caches in the system, NACKs are possible.
     * 
     * NOTE that if a NACK is received, the interface will resend the request.
     * This **can** lead to request reordering if the endpoint has issued multiple
     * requests for the same address in parallel. If the endpoint requires both ordering AND will
     * issue multiple requests for the same address in parallel, the endpoint should use a cache.
     */
    void handleNACK(MemEventBase* meb);

    /* Record noncacheable regions (e.g., MMIO device addresses) */
    std::multimap<Addr, MemRegion> noncacheableRegions;
   
    /** Perform some sanity checks to assist with debugging
     * These are only called if SST Core is configured with --enable-debug
     */
    MemEventBase*   response;
    HandlerBase*    recvHandler_;
    MemEventConverter* converter_;
};

}
}

#endif
