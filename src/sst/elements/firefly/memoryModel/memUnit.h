    class MemUnit : public Unit {
        enum Op { Read, Write };
      public:
        MemUnit( SimpleMemoryModel& model, Output& dbg, int id, int readLat_ns, int writeLat_ns, int numSlots ) :
            Unit( model, dbg ), m_pending(0), m_readLat_ns(readLat_ns), m_writeLat_ns(writeLat_ns), m_numSlots(numSlots)
        {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::MemUnit::@p():@l ";
        }

        bool store( UnitBase* src, MemReq* req ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);
            return work( m_writeLat_ns, Write, req, src );
        }

        bool load( UnitBase* src, MemReq* req, Callback callback ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
            return work( m_readLat_ns, Read, req, src, callback );
        }

      private:

        struct XXX {

            XXX( SimTime_t delay, Op op, MemReq* memReq, UnitBase* src, Callback callback ) :
                delay(delay), op(op), memReq(memReq), src(src), callback( callback )
            {}
            SimTime_t delay;
            Op op;
			MemReq* memReq;
            UnitBase* src;
			Callback callback;
        };

        bool work( SimTime_t delay, Op op, MemReq* req,  UnitBase* src, Callback callback = NULL ) {

            if ( m_pending == m_numSlots ) {

				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"blocking src\n");
                m_blocked.push_back( XXX( delay, op, req, src, callback ) );
                return true;
            }

            ++m_pending;
            SimTime_t issueTime = m_model.getCurrentSimTimeNano();

            m_model.schedCallback( delay,
                [=]()
                {
                    --m_pending;

                    SimTime_t latency = m_model.getCurrentSimTimeNano() - issueTime;

                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"%s complete latency=%" PRIu64 " addr=%#" PRIx64 " length=%lu\n",
                                                        op == Read ? "Read":"Write" ,latency, req->addr, req->length);

                    if ( callback ) {
                        m_model.schedCallback( 1, callback);
                    }

					delete req;

                    if ( ! m_blocked.empty() ) {
		
                        XXX& xxx = m_blocked.front( );
                        work( xxx.delay, xxx.op, xxx.memReq, xxx.src, xxx.callback );
                		m_model.schedResume( 1, xxx.src );
                        m_blocked.pop_front();
                    }
                }
            );
			return false;
        }

        std::deque< XXX > m_blocked;
        int m_pending;
        int m_numSlots;
        int m_readLat_ns;
        int m_writeLat_ns;
    };
