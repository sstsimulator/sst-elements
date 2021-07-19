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

class StoreUnit : public Unit {
	std::string m_name;
  public:
    StoreUnit( SimpleMemoryModel& model, Output& dbg, int id, int thread_id, Unit* cache, int numSlots, std::string name ) :
        Unit( model, dbg ), m_qSize(numSlots), m_cache(cache), m_blocked(false), m_blockedSrc(NULL), m_scheduled(false), m_name(name) {
        m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::"+ name + "StoreUnit::@p():@l ";

        std::stringstream tmp;
        tmp << "_" << name << "_" << id << "_"<< thread_id;
        m_name = tmp.str();

        m_pendingQdepth = model.registerStatistic<uint64_t>(name + "_store_pending_Q_depth",std::to_string(thread_id));
    }

	std::string& name() { return m_name; }

    bool storeCB( UnitBase* src, MemReq* req, Callback* callback = NULL ) {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"addr=%#" PRIx64 " length=%lu pending=%lu\n",req->addr,req->length,m_pendingQ.size());
		assert( NULL == m_blockedSrc );
		m_pendingQ.push( std::make_pair(req, callback) );
		m_pendingQdepth->addData( m_pendingQ.size() );

		if ( m_pendingQ.size() < m_qSize + 1) {
			if ( ! m_blocked && ! m_scheduled ) {
				Callback* cb = new Callback;
				*cb = std::bind( &StoreUnit::process, this );
				m_model.schedCallback( 0, cb );
				m_scheduled = true;
			}
		}

        if ( m_pendingQ.size() == m_qSize  ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"blocking src\n");
            m_blockedSrc = src;
            return true;
        } else {
            return false;
        }
	}

  private:

	void process( ) {
		std::pair<MemReq*,Callback*>& front = m_pendingQ.front();
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"addr=%#" PRIx64 " length=%lu\n",front.first->addr,front.first->length);

		assert( m_blocked == false );
		m_scheduled = false;

        m_blocked = m_cache->store( this, front.first );
        if ( front.second ) {
            m_model.schedCallback( 0, front.second );
        }
		m_pendingQ.pop();

		if ( m_blockedSrc ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"unblock src\n");
			m_model.schedResume( 0, m_blockedSrc, this );
			m_blockedSrc = NULL;
		}

		if ( ! m_blocked && ! m_pendingQ.empty() ) {
			Callback* cb = new Callback;
			*cb = std::bind( &StoreUnit::process, this );
			m_model.schedCallback( 0, cb );
			m_scheduled = true;
		}
	}

    void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,STORE_MASK,"pending=%lu\n",m_pendingQ.size());

		assert( m_blocked == true );
		m_blocked = false;
		if ( ! m_scheduled && ! m_pendingQ.empty() ) {
			process();
		}
    }


	UnitBase* m_blockedSrc;
	bool 	m_scheduled;
	bool    m_blocked;
    Unit*   m_cache;
    int     m_qSize;

    std::queue< std::pair<MemReq*,Callback*> >       m_pendingQ;
	Statistic<uint64_t>* m_pendingQdepth;
};
