  

class Work {

  public:
    Work( std::vector< MemOp >* ops, Callback callback, SimTime_t start ) : m_ops(ops),
                        m_callback(callback), m_start(start), m_pos(0) {}

    ~Work() {
        m_callback();
        delete m_ops;
    }
    SimTime_t start() { return m_start; }

	size_t getNumOps() { return m_ops->size(); }

	MemOp* popOp() {
		if ( m_pos <  m_ops->size() )  {
			return &(*m_ops)[m_pos++];
		} else {
			return NULL;
		}
	}

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
			m_maxAccessSize( accessSize ), m_currentOp(NULL), m_blocked(false)
	{
		m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name +"::@p():@l ";
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,THREAD_MASK,"this=%p\n",this );
	}

	void addWork( Work* work ) { 
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"work=%p numOps=%lu workSize=%lu blocked=%d\n",
														work, work->getNumOps(), m_workQ.size(), (int) m_blocked );
		m_workQ.push_back( work ); 

		if ( ! m_blocked && 1 == m_workQ.size() ) {

			m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"prime pump\n");		
			m_currentOp = m_workQ.front()->popOp();
			process();
        }
	}

	void resume( UnitBase* src = NULL ) {
        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"work=%lu\n", m_workQ.size() );
		
		m_blocked = false;
		if ( ! m_workQ.empty() ) {
			process();
		}
	}

  private:

    const char* prefix() { return m_prefix.c_str(); }
	
    void process() {

        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"enter\n");
		assert( ! m_workQ.empty() ); 

        MemOp* op;
		if ( m_currentOp ) {
			op = m_currentOp;
		} else {
			op = &m_noop;
		}

        Hermes::Vaddr addr = op->getCurrentAddr();
        size_t length = op->getCurrentLength( m_maxAccessSize );
		op->incOffset( length );

        m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"op=%s op.length=%lu offset=%lu addr=%#" PRIx64 " length=%lu\n",
                            op->getName(), op->length, op->offset, addr, length );

    	Callback callback = NULL;

		MemOp::Op type = op->getOp();

		if ( op->isDone() ) { 
		    bool isLoad = op->isLoad();
			m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"op %s is done\n",op->getName());

			Work* work = m_workQ.front();

            if ( op->callback ) {
                assert( !isLoad );
			    m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"callback\n");
                if ( op->isWrite() ) {
                    callback = op->callback;
                } else {
                    op->callback();
                }
            }

			m_currentOp = work->popOp();

			// if this work entry is done
			if ( ! m_currentOp ) {
				if ( isLoad ) {
					m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"last load of op and work\n");
            		callback = std::bind( &Thread::workDone, this, work);
				} else {
					m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"work %p done, workQ=%lu\n",work,m_workQ.size());
					workDone(work);
				}	
				m_workQ.pop_front();

				if ( ! m_workQ.empty() ) {
					m_currentOp = m_workQ.front()->popOp();
				}
			} else {

				if ( isLoad ) {
					m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"last load of op\n");
            		callback = std::bind( &Thread::process, this);
				}
			}
		}

        switch( type ) {
		  case MemOp::HostBusWrite:
            m_blocked = m_model.busUnit().write( this, new MemReq( addr, length), callback );
		    break;

		  case MemOp::NotInit:
		    break;

          case MemOp::LocalLoad:
            m_blocked = m_model.nicUnit().load( this, new MemReq( 0, 0), callback );
            break;

          case MemOp::LocalStore:
            m_blocked = m_model.nicUnit().store( this, new MemReq( 0, 0) );
            break;

          case MemOp::HostStore:
          case MemOp::BusStore:
          case MemOp::BusDmaToHost:
            m_blocked = m_storeUnit->store( this, new MemReq( addr, length ) );
            break;

          case MemOp::HostLoad:
          case MemOp::BusLoad:
          case MemOp::BusDmaFromHost:
            m_blocked = m_loadUnit->load( this, new MemReq( addr, length ), callback );
            break;

          default:
			printf("%d\n",type);
            assert(0);
        }

		if ( m_blocked ) {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"blocked\n");
		} else if ( ! callback && ! m_workQ.empty() )  {
        	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"schedule process()\n");
			m_model.schedCallback( 1, std::bind(&Thread::process, this ) ); 
		}
       	m_dbg.verbosePrefix(prefix(),CALL_INFO,2,THREAD_MASK,"blocked=%d\n",(int)m_blocked);
    }
	
	void workDone( Work* work ) {
		m_dbg.verbosePrefix(prefix(),CALL_INFO,1,THREAD_MASK,"work %p done, latency=%" PRIu64 "\n",work,
                    m_model.getCurrentSimTimeNano() - work->start());
		delete work;
	}

	MemOp* m_currentOp;
	bool m_blocked;
	
	MemOp			  m_noop;
	std::string m_prefix;
	SimpleMemoryModel& m_model;
    std::deque<Work*> m_workQ;
    Unit*             m_loadUnit;
    Unit*             m_storeUnit;
    Output& 		  m_dbg;
    int 		      m_maxAccessSize;
};
