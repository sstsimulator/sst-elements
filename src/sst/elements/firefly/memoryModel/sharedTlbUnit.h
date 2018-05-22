// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
        Entry( MemReq* req, Callback callback  ) : req(req), callback(callback) {} 
        MemReq* req;
        Callback callback;
    };
    
  public:
    SharedTlbUnit( SimpleMemoryModel& model, Output& dbg, int id, std::string name, SharedTlb* tlb, Unit* load, Unit* store, int maxStores, int maxLoads ) :
        Unit( model, dbg ),
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
    }

    void resume( UnitBase* unit ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"\n");

        if ( unit == m_store ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"store unblocked\n");
            assert( m_storeBlocked );
            m_storeBlocked = false;
            while( ! m_storeBlocked && ! m_readyStores.empty() ) {
                m_loadBlocked = passUpLoad( m_readyLoads.front() );
                m_readyStores.pop_front();
            }
        }

        if ( unit == m_load ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"load unblocked\n");
            assert( m_loadBlocked );
            m_loadBlocked= false;
            while( ! m_loadBlocked && ! m_readyLoads.empty() ) {
                m_loadBlocked = passUpLoad( m_readyLoads.front() );
                m_readyLoads.pop_front();
            }
        }
    }

    bool storeCB( UnitBase* src, MemReq* req, Callback callback ) {

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
            m_blockedStoreSrc = src;
            return true;
        } else {
            return false; 
        }
    }
    
    bool load( UnitBase* src, MemReq* req, Callback callback ) {

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

        if ( m_readyStores.size() == m_maxPendingLoads ) {
            assert( m_blockedLoadSrc );
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

        if ( m_readyLoads.size() == m_maxPendingStores ) {
            assert( m_blockedStoreSrc );
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

    void storeAddrResolved( Callback callback, MemReq* req, uint64_t addr ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 " pendingLookups=%d\n",
                                         req->addr, addr, m_pendingLookups);
        if ( m_blockedStoreSrc ) {
            if ( m_readyStores.size() < m_maxPendingStores && blockedTlb() ) { 
                m_model.schedResume( 0, m_blockedStoreSrc );
                m_blockedStoreSrc = NULL;
            }
        } 

        assert( m_pendingLookups > 0 );
        --m_pendingLookups;
        //Note that we are called from the TLB and may already be blocked on store
        storeAddrResolved2( callback, req, addr );
    }
    void storeAddrResolved2( Callback callback, MemReq* req, uint64_t addr ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);
        req->addr = addr;
        if ( m_storeBlocked ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);
            m_readyStores.push_back( new Entry( req, callback ) );
        } else {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"call storeCB %#" PRIx64 "\n", req->addr );
            m_storeBlocked = m_store->storeCB( this, req, callback );
        }
    }

    void loadAddrResolved( Callback callback, MemReq* req, uint64_t addr ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 " pendingLookups=%d\n",
                                         req->addr, addr, m_pendingLookups);
        if ( m_blockedLoadSrc ) {
            if ( m_readyLoads.size() < m_maxPendingLoads && blockedTlb() ) { 
                m_model.schedResume( 0, m_blockedLoadSrc );
                m_blockedLoadSrc = NULL;
            }
        } 

        assert( m_pendingLookups > 0 );
        --m_pendingLookups;
        //Note that we are called from the TLB and may already be blocked store
        loadAddrResolved2( callback, req, addr );
    }

    void loadAddrResolved2( Callback callback, MemReq* req, uint64_t addr ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64 " -> %#" PRIx64 "\n", req->addr, addr);

        req->addr = addr;
        if ( m_loadBlocked ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,TLB_MASK,"addr=%#" PRIx64"\n", req->addr);
            m_readyLoads.push_back( new Entry( req, callback ) );
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

    std::deque<Entry*> m_readyLoads;
    std::deque<Entry*> m_readyStores;
};
