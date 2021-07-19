// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

class SharedTlbUnit : public Unit {
    std::string m_prefix;
    const char* prefix() { return m_prefix.c_str(); }

    struct Entry {
        Entry( MemReq* req, Callback* callback  ) : req(req), callback(callback) {}
        MemReq* req;
        Callback* callback;
    };

    std::string m_name;
  public:
    SharedTlbUnit( SimpleMemoryModel& model, Output& dbg, int id, std::string name, SharedTlb* tlb, Unit* load, Unit* store, int maxStores, int maxLoads ) :
        Unit( model, dbg ),
        m_name(name),
        m_tlb(tlb),
        m_load(load),
        m_store(store),
        m_storeBlocked( false ),
        m_loadBlocked( false ),
        m_maxPendingStores(maxStores),
        m_maxPendingLoads(maxLoads),
        m_blockedLoadSrc( NULL ),
        m_blockedStoreSrc(NULL),
        m_pendingLookups(0),
        m_maxPendingLookups(4)
    {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::"+name+"SharedTlbUnit::@p():@l ";
    }

    ~SharedTlbUnit() {
		delete m_load;
		if ( m_load != m_store ) {
			delete m_store;
		}
    }

    void printStatus( Output& out, int id ) {
        out.output("NIC %d: %s pending=%d %p %p\n",id, m_name.c_str(), m_pendingLookups, m_blockedStoreSrc, m_blockedLoadSrc );
    }

    void resume( UnitBase* unit ) {

        if ( unit == m_store ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"store unblocked\n");
            assert( m_storeBlocked );
            m_storeBlocked = false;
            while( ! m_storeBlocked && ! m_readyStores.empty() ) {
                m_storeBlocked = passUpStore( m_readyStores.front() );
                m_readyStores.pop();
            }
        }

        if ( unit == m_load ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"load unblocked\n");
            assert( m_loadBlocked );
            m_loadBlocked= false;
            while( ! m_loadBlocked && ! m_readyLoads.empty() ) {
                m_loadBlocked = passUpLoad( m_readyLoads.front() );
                m_readyLoads.pop();
            }
        }
    }

    bool storeCB( UnitBase* src, MemReq* req, Callback* callback ) {

        uint64_t addr = m_tlb->lookup( req,
                    std::bind(&SharedTlbUnit::storeAddrResolved, this, callback, std::placeholders::_1, std::placeholders::_2 ) );
        if ( -1 == addr ) {
            ++m_pendingLookups;
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Miss, pid %d, req Addr %#" PRIx64 " pendingLookups=%d\n",
                                                req->pid, req->addr, m_pendingLookups );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Hit, pid %d, req Addr %#" PRIx64 "\n", req->pid, req->addr );
            storeAddrResolved2( callback, req, addr );
        }

        if ( blockedStore() ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Blocking source, pid %d, req Addr %#" PRIx64 "\n", req->pid, req->addr );
            assert( ! m_blockedStoreSrc );
            m_blockedStoreSrc = src;
            return true;
        } else {
            return false;
        }
    }

    bool load( UnitBase* src, MemReq* req, Callback* callback ) {

        uint64_t addr = m_tlb->lookup( req,
                    std::bind(&SharedTlbUnit::loadAddrResolved, this, callback, std::placeholders::_1, std::placeholders::_2 ) );
        if ( -1 == addr ) {
            ++m_pendingLookups;
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Miss, pid %d, req Addr %#" PRIx64 " pendingLookups=%d\n",
                                                req->pid, req->addr, m_pendingLookups );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Hit, pid %d, req Addr %#" PRIx64 "\n", req->pid, req->addr );
            loadAddrResolved2( callback, req, addr );
        }
        if ( blockedLoad() ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"Blocking source, pid %d, req Addr %#" PRIx64 "\n", req->pid, req->addr );
            assert( ! m_blockedLoadSrc );
            m_blockedLoadSrc = src;
            return true;
        } else {
            return false;
        }
    }

  private:
   bool passUpLoad( Entry* entry ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK," req addr=%#" PRIx64 "\n",entry->req->addr);
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"call load %#" PRIx64 "\n", entry->req->addr );
        bool retval = m_load->load( this, entry->req, entry->callback );
        delete entry;

        if ( m_blockedLoadSrc && m_readyLoads.size() == m_maxPendingLoads ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"schedule resume\n");
            m_model.schedResume( 0, m_blockedLoadSrc );
            m_blockedLoadSrc = NULL;
        }
        return retval;
    }

    bool passUpStore( Entry* entry ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"req addr=%#" PRIx64 "\n",entry->req->addr);
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"call storeCB %#" PRIx64 "\n", entry->req->addr );
        bool retval = m_store->storeCB( this, entry->req, entry->callback );
        delete entry;

        if ( m_blockedStoreSrc && m_readyStores.size() == m_maxPendingStores ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"schedule resume\n");
            m_model.schedResume( 0, m_blockedStoreSrc );
            m_blockedStoreSrc = NULL;
        }

        return retval;
    }

    bool blockedTlb() {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,TLB_MASK,"pendingLookups=%d\n", m_pendingLookups );
        return m_pendingLookups == m_maxPendingLookups;
    }
    bool blockedStore() {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,TLB_MASK,"pendingLookups=%d\n", m_pendingLookups );
        return blockedTlb() || m_readyStores.size() >= m_maxPendingStores;
    }
    bool blockedLoad() {
        return blockedTlb() || m_readyLoads.size() >= m_maxPendingLoads;
    }

    void checkBlockedSrcs() {
        bool flag = true;
        if ( m_blockedLoadSrc ) {
            if ( m_readyLoads.size() < m_maxPendingLoads ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"schedule resume\n");
                m_model.schedResume( 0, m_blockedLoadSrc );
                m_blockedLoadSrc = NULL;
                flag = false;
            }
        }

        if ( flag && m_blockedStoreSrc ) {
            if ( m_readyStores.size() < m_maxPendingStores  ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"schedule resume\n");
                m_model.schedResume( 0, m_blockedStoreSrc );
                m_blockedStoreSrc = NULL;
            }
        }
    }

    void storeAddrResolved( Callback* callback, MemReq* req, uint64_t addr ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 " pendingLookups=%d\n",
                                         req->addr, addr, m_pendingLookups);

        checkBlockedSrcs();

        assert( m_pendingLookups > 0 );
        --m_pendingLookups;
        //Note that we are called from the TLB and may already be blocked on store
        storeAddrResolved2( callback, req, addr );
    }

    void storeAddrResolved2( Callback* callback, MemReq* req, uint64_t addr ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);
        req->addr = addr;
        if ( m_storeBlocked ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);
            m_readyStores.push( new Entry( req, callback ) );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"call storeCB %#" PRIx64 "\n", req->addr );
            m_storeBlocked = m_store->storeCB( this, req, callback );
        }
    }

    void loadAddrResolved( Callback* callback, MemReq* req, uint64_t addr ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 " pendingLookups=%d\n",
                                         req->addr, addr, m_pendingLookups);
        checkBlockedSrcs();

        assert( m_pendingLookups > 0 );
        --m_pendingLookups;
        //Note that we are called from the TLB and may already be blocked store
        loadAddrResolved2( callback, req, addr );
    }

    void loadAddrResolved2( Callback* callback, MemReq* req, uint64_t addr ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);

        req->addr = addr;
        if ( m_loadBlocked ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64"\n", req->addr);
            m_readyLoads.push( new Entry( req, callback ) );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"call load %#" PRIx64 "\n", req->addr );
            m_loadBlocked = m_load->load( this, req, callback );
        }
    }

    Unit* m_load;
    Unit* m_store;
    SharedTlb* m_tlb;

    int m_pendingLookups;
    int m_maxPendingLookups;

    int m_maxPendingStores;
    int m_maxPendingLoads;

    bool m_storeBlocked;
    bool m_loadBlocked;

    UnitBase* m_blockedStoreSrc;
    UnitBase* m_blockedLoadSrc;

    std::queue<Entry*> m_readyLoads;
    std::queue<Entry*> m_readyStores;
};
