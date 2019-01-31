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

class Work {

  public:
    Work( int pid, std::vector< MemOp >* ops, Callback callback, SimTime_t start, int alignment = 64 ) :
        m_pid(pid), m_ops(ops), m_callback(callback), m_start(start), m_pos(0)
    {
        if( 0 == ops->size() ) {
            ops->push_back( MemOp( MemOp::Op::NoOp ) );
        }
#if 0
        std::vector< MemOp >::iterator iter = (*ops).begin();
        size_t x = (*ops).size();
        while ( iter != (*ops).end() ) {
            if ( iter->addr & (alignment -1) ) {
                //printf("%s() not aligned addr=%#" PRIx64 " length=%" PRIu64 "\n",__func__, iter->addr, iter->length);
                if ( iter->length < alignment ) {
                    //printf("%s() fix addr=%#" PRIx64 " length=%" PRIu64 "\n",__func__, iter->addr, iter->length);
                    iter->addr = iter->addr & ~(alignment-1);
                    iter->length = alignment;
                    ++iter;
                } else {
                    MemOp op = MemOp( iter->addr & ~(alignment-1), alignment, iter->getOp() );
                    //printf("%s() insert addr=%#" PRIx64 " length=%" PRIu64 "\n",__func__, op.addr, op.length);

                    iter->length = iter->length - (iter->addr & (alignment-1));  
                    iter->length = iter->length + alignment & ~(alignment-1);  
                    iter->addr = op.addr + alignment; 
                    //printf("%s() fix addr=%#" PRIx64 " length=%" PRIu64 "\n",__func__, iter->addr, iter->length);
                    iter = (*ops).insert( iter, op );
                }
            } else {
                ++iter;
            }
        }
#endif
#if 0
        if ( x !=(*ops).size() ) {
            printf("%s() grew %lu\n",__func__, (*ops).size() - x );
        }
#endif
    }

    ~Work() {
        m_callback();
        delete m_ops;
    }

    int getPid()        { return m_pid; } 
    SimTime_t start()   { return m_start; }
	size_t getNumOps()  { return m_ops->size(); }
    bool isDone()       { return m_pos ==  m_ops->size(); }

	MemOp* popOp() {
		if ( m_pos <  m_ops->size() )  {
			return &(*m_ops)[m_pos++];
		} else {
			return NULL;
		}
	}

    void print( Output& dbg, const char* prefix ) {
        for ( unsigned i = 0; i < m_ops->size(); i++ ) {
           dbg.verbosePrefix(prefix,CALL_INFO,2,THREAD_MASK,"%s %#" PRIx64 " %lu\n",(*m_ops)[i].getName(), (*m_ops)[i].addr, (*m_ops)[i].length );
        }
    }
    int m_workNum;
    std::queue<Callback>    m_pendingCallbacks;
  private:
    SimTime_t               m_start;
    int 					m_pos;
    Callback 				m_callback;
    std::vector< MemOp >*   m_ops;
    int                     m_pid;
};


class Thread : public UnitBase {

    std::string m_name;

  public:	  
     Thread( SimpleMemoryModel& model, std::string name, Output& output, int id, int thread_id , int accessSize, Unit* load, Unit* store ) : 
			m_model(model), m_name(name), m_dbg(output), m_id(id), m_loadUnit(load), m_storeUnit(store), 
			m_maxAccessSize( accessSize ), m_nextOp(NULL), m_waitingOnOp(NULL), m_blocked(false), m_curWorkNum(0),m_lastDelete(0)
	{
		m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name +"::@p():@l ";
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,THREAD_MASK,"this=%p\n",this );
		m_workQdepth = model.registerStatistic<uint64_t>(name + "_thread_work_Q_depth",std::to_string(thread_id));
	}


    ~Thread() {
        delete m_loadUnit;
        if ( m_loadUnit != m_storeUnit ) {
            delete m_storeUnit;
        }
    }

    void printStatus( Output& out, int id ) {
        out.output( "NIC %d: %s cur=%d last=%d blocked=%d %p %p\n",id, m_name.c_str(), m_curWorkNum, m_lastDelete, m_blocked, m_nextOp, m_waitingOnOp );
        if ( m_workQ.size() ) {
            out.output( "NIC %d: %s work.size=%zu\n", id, m_name.c_str(), m_workQ.size() );
            std::deque<Work*>::iterator iter = m_workQ.begin();

            for ( ; iter != m_workQ.end(); ++iter) {
                (*iter)->print(out,"");
            } 
        }
        if ( m_OOOwork.size() ) {
            out.output( "NIC %d: %s OOOwork.size: %zu \n", id, m_name.c_str(), m_OOOwork.size() );
        }
        m_loadUnit->printStatus( out, id );
        m_storeUnit->printStatus( out, id );
    }

    bool isIdle() {
        return !m_workQ.size();
    }

	void addWork( Work* work ) { 
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"work=%p numOps=%lu workSize=%lu blocked=%d\n",
														work, work->getNumOps(), m_workQ.size(), (int) m_blocked );
        work->m_workNum = m_curWorkNum++;
		m_workQ.push_back( work ); 

        work->print(m_dbg,prefix());
		m_workQdepth->addData( m_workQ.size() );

		if ( ! m_blocked && ! m_nextOp ) {
			m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"prime pump\n");		
			process( );
        }
	}

	void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"work=%lu\n", m_workQ.size() );
		
		m_blocked = false;
        if ( m_waitingOnOp ) { return; } 

        if ( m_nextOp ) {
            process( m_nextOp ); 
        } else if ( ! m_workQ.empty() ) {
			process();
		}
	}

  private:

    void process( MemOp* op = NULL ) {

        assert( ! m_workQ.empty() );
		Work* work = m_workQ.front();
        if ( ! op ) {
		    op = work->popOp();
        }

        Hermes::Vaddr addr = op->getCurrentAddr();
        size_t length = op->getCurrentLength( m_maxAccessSize );
		op->incOffset( length );

        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"op=%s op.length=%lu offset=%lu addr=%#" PRIx64 " length=%lu\n",
                            op->getName(), op->length, op->offset, addr, length );

        int pid = work->getPid();
		MemOp::Op type = op->getOp();

        bool deleteWork = false;
        if ( work->isDone() ) {
            if ( op->isDone() ) {
                // if the work is done and the Op is done pop the work Q so 
                m_workQ.pop_front();
            }
            deleteWork = true;
        }
        
        // note that "work" will be a valid  ptr for all of the issues of the last Op 
        // because we don't know which one will complete last
    	Callback* callback = new Callback;
		*callback = std::bind(&Thread::opCallback,this, work, op, deleteWork );

        switch( op->getOp() ) {
          case MemOp::NoOp:
	        m_model.schedCallback( 0, callback );
            break;

		  case MemOp::HostBusWrite:
            m_blocked = m_model.busUnitWrite( this, new MemReq( addr, length ), callback );
		    break;

          case MemOp::LocalLoad:
			m_blocked = m_model.nicUnit().load( this, new MemReq( 0, 0), callback );
            break;

          case MemOp::LocalStore:
			m_blocked = m_model.nicUnit().storeCB( this, new MemReq( 0, 0), callback );
            break;

          case MemOp::HostStore:
          case MemOp::BusStore:
          case MemOp::BusDmaToHost:
            addr |= (uint64_t) pid << 56;
			m_blocked = m_storeUnit->storeCB( this, new MemReq( addr, length, pid ), callback );
            break;

          case MemOp::HostLoad:
          case MemOp::BusLoad:
          case MemOp::BusDmaFromHost:
            addr |= (uint64_t) pid << 56;
			m_blocked = m_loadUnit->load( this, new MemReq( addr, length, pid ), callback );
            break;

          default:
			printf("%d\n",op->getOp() );
            assert(0);
        }

        // if the Op is done it means we are issing the last chunk of this Op
        if ( op->isDone() ) {
            if ( ! m_workQ.empty() )  {
                // the just issued Op is complete, get the next Op
                m_nextOp = m_workQ.front()->popOp();
            } else {
                m_nextOp = NULL;
            }
        } else {
            m_nextOp = op;
        }

        m_waitingOnOp = NULL;
        if ( m_nextOp && op->getOp() != m_nextOp->getOp() ) { 
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"stalled on Op %p\n",op);
            m_waitingOnOp = op;
        }

		if ( m_blocked ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"blocked\n");
            // resume() will be called
            // the OP callback will also be called
		} else if ( m_nextOp && ! m_waitingOnOp ) { 
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"schedule process()\n");
			Callback* cb = new Callback;
			*cb = std::bind(&Thread::process, this, m_nextOp ); 
		    m_model.schedCallback( 0, cb );
        }
    }

    void opCallback( Work* work, MemOp* op, bool deleteWork ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"opPtr=%p %s waitingOnOp=%p\n",op,op->getName(),m_waitingOnOp);
        op->decPending();

        if ( op->canBeRetired() ) {
            if ( work->m_workNum  == m_lastDelete ) {
                while ( ! work->m_pendingCallbacks.empty() ) {
                    work->m_pendingCallbacks.front()();
                    work->m_pendingCallbacks.pop();
                }
                if ( op->callback ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"do op callback\n");
                    op->callback();
                }

                if ( deleteWork ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"delete work=%p %d\n",work, work->m_workNum);
                    delete work;
                    ++m_lastDelete;
                    while ( ! m_OOOwork.empty() ) {
                        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"check OOO, looking for %d\n",m_lastDelete);
                        if ( m_OOOwork.find( m_lastDelete ) != m_OOOwork.end() ) {
                            work = m_OOOwork[ m_lastDelete ];
                            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"delete OOO work %p\n",m_OOOwork[m_lastDelete]);
                            while ( ! work->m_pendingCallbacks.empty() ) {
                                work->m_pendingCallbacks.front()();
                                work->m_pendingCallbacks.pop();
                            }
                            delete work;
                            m_OOOwork.erase( m_lastDelete++ );
                        } else {
                            break;
                        }
                    }
                }
            } else {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"OOO work %p %d\n",work,work->m_workNum);
                if ( deleteWork ) {
                    m_OOOwork[work->m_workNum] = work;
                }
                if ( op->callback ) {
                    work->m_pendingCallbacks.push(op->callback);
                }
            }
        }

        if ( op == m_waitingOnOp ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"issue the stalled Op\n");
            m_waitingOnOp = NULL;
            if ( ! m_blocked ) {
                assert( m_nextOp );
                process( m_nextOp );
            }
        }
    }

    const char* prefix() { return m_prefix.c_str(); }
	
	MemOp*  m_nextOp;
	MemOp*  m_waitingOnOp;
	bool    m_blocked;
	
	std::string         m_prefix;
	SimpleMemoryModel&  m_model;
    std::deque<Work*>   m_workQ;
    Unit*               m_loadUnit;
    Unit*               m_storeUnit;
    Output& 		    m_dbg;
    int 		        m_maxAccessSize;
    int                 m_curWorkNum;
    int                 m_lastDelete;
    int                 m_id;
    std::map<int,Work*> m_OOOwork;
	Statistic<uint64_t>* m_workQdepth;
};

