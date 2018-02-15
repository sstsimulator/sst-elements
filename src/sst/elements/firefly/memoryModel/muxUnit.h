    

	class MuxUnit : public Unit {
		struct Entry {
			enum Op { Load, Store } op;
			Entry( Op op, UnitBase* src, MemReq* req, Callback callback=NULL ) : 
				op(op), src(src), req(req), callback(callback) {}
			UnitBase* src;
			MemReq* req;
			Callback callback;
		};

	  public:
		MuxUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* unit, std::string name ) : Unit( model, dbg), m_unit(unit), m_blockedSrc(NULL), m_scheduled(false)  {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "MuxUnit::@p():@l ";
		}

        bool store( UnitBase* src, MemReq* req ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"%s addr=%#" PRIx64 " length=%lu\n",src->name().c_str(), req->addr,req->length);
			if ( ! m_blockedSrc ) {
				if ( m_unit->store( this, req ) ) {
					m_blockedSrc = src;
					return true;
				} else { 
					return false; 
				}
			} else {
				m_blockedQ.push_back( Entry( Entry::Store, src, req ) );	
				return true;
			}
		}


        bool load( UnitBase* src, MemReq* req, Callback callback ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"%s addr=%#" PRIx64 " length=%lu\n",src->name().c_str(), req->addr,req->length);

			if ( ! m_blockedSrc ) {
				if ( m_unit->load( this, req, callback ) ) {
					m_blockedSrc = src;
					return true;
				} else { 
					return false; 
				}
			} else {
				m_blockedQ.push_back( Entry( Entry::Load, src, req, callback ) );	
				return true;
			}
		}

		void processQ( ) {
            m_scheduled = false;
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,MUX_MASK,"\n");
			assert( ! m_blockedQ.empty() );
			Entry& entry = m_blockedQ.front();

			bool blocked = false;
			if ( Entry::Load == entry.op ) {
				blocked = m_unit->load( this, entry.req, entry.callback );
			} else {
				blocked = m_unit->store( this, entry.req );
			}	

			if ( ! blocked ) {
				m_model.schedResume( 1, entry.src  );
				if ( m_blockedQ.size() > 1 ) {
                    m_scheduled = true;
					m_model.schedCallback( 1, std::bind( &MuxUnit::processQ, this ) );
				}
			} else {
				m_blockedSrc = entry.src;
			}
			m_blockedQ.pop_front();
		}

		void resume( UnitBase* src = NULL ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,MUX_MASK,"\n");
			if ( m_blockedSrc ) {
				m_model.schedResume( 1, m_blockedSrc );
				m_blockedSrc = NULL;
			}

			if ( !m_scheduled && ! m_blockedQ.empty() ) {
				processQ();
			}
		}

	  private:
		UnitBase* m_blockedSrc;
		Unit* m_unit;
		std::deque<Entry> m_blockedQ;
        bool m_scheduled;
	};
