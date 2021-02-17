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

	class MuxUnit : public Unit {
		struct Entry {
			enum Op { Load, Store } op;
			Entry( Op op, UnitBase* src, MemReq* req, uint64_t start, Callback* callback=NULL ) :
                op(op), src(src), req(req), start(start), callback(callback) {}
			UnitBase* src;
			MemReq* req;
			Callback* callback;
			uint64_t start;
		};

		std::string m_name;

	  public:

		MuxUnit( SimpleMemoryModel& model, Output& dbg, int id, Unit* unit, std::string name ) : Unit( model, dbg), m_unit(unit), m_blockedSrc(NULL), m_scheduled(false)  {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::" + name + "MuxUnit::@p():@l ";

			m_blocked_ns = model.registerStatistic<uint64_t>(name + "_mux_blocked_ns");
		}

		std::string& name() { return m_name; }

        bool store( UnitBase* src, MemReq* req ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"%s addr=%#" PRIx64 " length=%lu\n",src->name().c_str(), req->addr,req->length);
			if ( ! m_blockedSrc && ! m_scheduled ) {
				if ( m_unit->store( this, req ) ) {

                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"blocking\n");
					m_blockedSrc = src;
					m_blockedTime_ns = m_model.getCurrentSimTimeNano();
					return true;
				} else {
					return false;
				}
			} else {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"blocking\n");
				m_blockedQ.push( Entry( Entry::Store, src, req, m_model.getCurrentSimTimeNano() ) );
				return true;
			}
		}


        bool load( UnitBase* src, MemReq* req, Callback* callback ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"%s addr=%#" PRIx64 " length=%lu\n",src->name().c_str(), req->addr,req->length);

			uint64_t now = m_model.getCurrentSimTimeNano();
			if ( ! m_blockedSrc && ! m_scheduled ) {


				Callback* cb = new Callback;
				*cb = [=]() {
								m_dbg.verbosePrefix( prefix(), CALL_INFO_LAMBDA, "load",1,MUX_MASK, "load done latency=%" PRIu64 "\n",
												m_model.getCurrentSimTimeNano() - now );
								(*callback)();
								delete callback;
							};
				if ( m_unit->load( this, req, cb ) ) {
                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"blocking\n");
					m_blockedSrc = src;
					m_blockedTime_ns = m_model.getCurrentSimTimeNano();
					return true;
				} else {
					return false;
				}
			} else {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"blocking\n");
				m_blockedQ.push( Entry( Entry::Load, src, req, m_model.getCurrentSimTimeNano(), callback ) );
				return true;
			}
		}

		void processQ( ) {
            m_scheduled = false;
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,MUX_MASK,"\n");
			assert( ! m_blockedQ.empty() );
			Entry& entry = m_blockedQ.front();

			Callback* callback =  entry.callback;
			bool blocked = false;
			uint64_t now = entry.start;
			if ( Entry::Load == entry.op ) {
				Callback* cb = new Callback;
				*cb = [=](){
								m_dbg.verbosePrefix( prefix(), CALL_INFO_LAMBDA, "processQ",1,MUX_MASK, "load done latency=%" PRIu64 "\n",
										m_model.getCurrentSimTimeNano() - now );
								(*callback)();
								delete callback;
						   };
				blocked = m_unit->load( this, entry.req, cb );
			} else {
				blocked = m_unit->store( this, entry.req );
			}

			if ( ! blocked ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"unblocking\n");
				m_model.schedResume( 0, entry.src  );
				if ( m_blockedQ.size() > 1 ) {
                    m_scheduled = true;
					Callback* cb = new Callback;
					*cb = std::bind( &MuxUnit::processQ, this );
					m_model.schedCallback( 0, cb );
				}
			} else {
				m_blockedSrc = entry.src;
			}
			m_blockedQ.pop();
		}

		void resume( UnitBase* src = NULL ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,2,MUX_MASK,"\n");
			if ( m_blockedSrc ) {
                m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MUX_MASK,"unblocking\n");
				m_model.schedResume( 0, m_blockedSrc );
				m_blockedSrc = NULL;
				SimTime_t latency = m_model.getCurrentSimTimeNano() - m_blockedTime_ns;
				if ( latency ) {
					m_blocked_ns->addData( latency );
				}
			}
                m_dbg.verbosePrefix(prefix(),CALL_INFO,2,MUX_MASK,"scheduled=%d numBlocked=%zu\n",m_scheduled, m_blockedQ.size());

			if ( !m_scheduled && ! m_blockedQ.empty() ) {
				processQ();
			}
		}

	  private:
		UnitBase* m_blockedSrc;
		Unit* m_unit;
		std::queue<Entry> m_blockedQ;
        bool m_scheduled;
		Statistic<uint64_t>* m_blocked_ns;
		SimTime_t m_blockedTime_ns;
	};
