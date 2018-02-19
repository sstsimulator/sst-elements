
class Work {

  public:
    Work( std::vector< MemOp >* ops, Callback callback, SimTime_t start ) : m_ops(ops),
                        m_callback(callback), m_start(start), m_pos(0) {
        if( 0 == ops->size() ) {
            ops->push_back( MemOp( MemOp::Op::NoOp ) );
        }
    }

    ~Work() {
        m_callback();
        delete m_ops;
    }

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
           dbg.verbosePrefix(prefix,CALL_INFO,1,THREAD_MASK,"%s %#" PRIx64 " %lu\n",(*m_ops)[i].getName(), (*m_ops)[i].addr, (*m_ops)[i].length );
        }
    }
    int m_workNum;
    std::deque<Callback>    m_pendingCallbacks;
  private:
    SimTime_t               m_start;
    int 					m_pos;
    Callback 				m_callback;
    std::vector< MemOp >*   m_ops;
};


class Thread : public UnitBase {

  public:	  
     Thread( SimpleMemoryModel& model, std::string name, Output& output, int id, int accessSize, Unit* load, Unit* store ) : 
			m_model(model), m_dbg(output), m_loadUnit(load), m_storeUnit(store),
			m_maxAccessSize( accessSize ), m_currentOp(NULL), m_waitingOnOp(NULL), m_blocked(false), m_curWorkNum(0),m_lastDelete(0)
	{
		m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name +"::@p():@l ";
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,THREAD_MASK,"this=%p\n",this );
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

		if ( ! m_blocked && ! m_currentOp ) {
			m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"prime pump\n");		
			process( );
        }
	}

	void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"work=%lu\n", m_workQ.size() );
		
		m_blocked = false;
        if ( m_currentOp ) {
            process( m_currentOp ); 
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
    	Callback callback = std::bind(&Thread::opCallback,this, work, op, deleteWork );

        switch( op->getOp() ) {
          case MemOp::NoOp:
	        m_model.schedCallback( 1, callback );
            break;

		  case MemOp::HostBusWrite:
            m_blocked = m_model.busUnit().write( this, new MemReq( addr, length), callback );
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
            m_blocked = m_storeUnit->storeCB( this, new MemReq( addr, length ), callback );
            break;

          case MemOp::HostLoad:
          case MemOp::BusLoad:
          case MemOp::BusDmaFromHost:
            m_blocked = m_loadUnit->load( this, new MemReq( addr, length ), callback );
            break;

          default:
			printf("%d\n",op->getOp() );
            assert(0);
        }

        // if the Op is done it means we are issing the last chunk of this Op
        if ( op->isDone() ) {
            m_currentOp = NULL;
        } else {
            m_currentOp = op;
        }

		if ( m_blocked ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"blocked\n");
            // resume() will be called
            // the OP callback will also be called
		} else if ( m_currentOp ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"op is not done\n");
			m_model.schedCallback( 1, std::bind(&Thread::process, this, m_currentOp ) ); 
        } else {

            if ( ! m_workQ.empty() )  {
                
                // the just issued Op is complete, get the next Op
		        m_currentOp = m_workQ.front()->popOp();

       	        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"currentOp=%s nextOp=%s\n",op->getName(), m_currentOp->getName());

                // if the just issued Op and the next Op are the same we don't have to stall the pipeline
                if ( op->getOp() == m_currentOp->getOp() ) { 
        	        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"schedule process()\n");
			        m_model.schedCallback( 1, std::bind(&Thread::process, this, m_currentOp ) ); 
                } else {
        	        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"stalled on Op %p\n",op);
                    m_waitingOnOp = op;
                }
            }
		}
    }

    void opCallback( Work* work, MemOp* op, bool deleteWork ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"opPtr=%p %s waitingOnOp=%p\n",op,op->getName(),m_waitingOnOp);

        op->decPending();

        if ( op->canBeRetired() ) {
            if ( work->m_workNum  == m_lastDelete ) {
                if ( op->callback ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"do op callback\n");
                    op->callback();
                }

                if ( deleteWork ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"delete work %p %d\n",work, work->m_workNum);
                    delete work;
                    ++m_lastDelete;
                    while ( ! m_OOOwork.empty() ) {
                        m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"check OOO, looking for %d\n",m_lastDelete);
                        if ( m_OOOwork.find( m_lastDelete ) != m_OOOwork.end() ) {
                            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"retire OOO work %p\n",m_OOOwork[m_lastDelete]);
                            while ( ! work->m_pendingCallbacks.empty() ) {
                                work->m_pendingCallbacks.front()();
                                work->m_pendingCallbacks.pop_front();
                            }
                            delete m_OOOwork[m_lastDelete];
                            m_OOOwork.erase( m_lastDelete++ );
                        } else {
                            break;
                        }
                    }
                }
            } else {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"OOO work %p %d\n",work,work->m_workNum);
                m_OOOwork[work->m_workNum] = work;
                if ( op->callback ) {
                    work->m_pendingCallbacks.push_back(op->callback);
                }
            }
        }

        if ( op == m_waitingOnOp ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"issue the stalled Op\n");
            m_waitingOnOp = NULL;
            assert( m_currentOp );
            process( m_currentOp );
        }
    }

    const char* prefix() { return m_prefix.c_str(); }
	
	MemOp*  m_currentOp;
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
    std::map<int,Work*> m_OOOwork;
};

