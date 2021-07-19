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

    class MemUnit : public Unit {
        enum Op { Read, Write };
      public:
        MemUnit( SimpleMemoryModel& model, Output& dbg, int id, int readLat_ns, int writeLat_ns, int numSlots ) :
            Unit( model, dbg ), m_pending(0), m_readLat_ns(readLat_ns), m_writeLat_ns(writeLat_ns), m_numSlots(numSlots)
        {
            m_prefix = "@t:" + std::to_string(id) + ":SimpleMemoryModel::MemUnit::@p():@l ";
			m_latency = model.registerStatistic<uint64_t>("mem_blocked_time");
			m_loads = model.registerStatistic<uint64_t>("mem_num_loads");
			m_stores = model.registerStatistic<uint64_t>("mem_num_stores");
			m_addrs = model.registerStatistic<uint64_t>("mem_addrs");
        }

        bool store( UnitBase* src, MemReq* req ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr, req->length);
			m_stores->addData( 1 );
            return work( m_writeLat_ns, Write, req, src, m_model.getCurrentSimTimeNano() );
        }

        bool load( UnitBase* src, MemReq* req, Callback* callback ) {
            m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"addr=%#" PRIx64 " length=%lu\n",req->addr,req->length);
			m_loads->addData( 1 );
            return work( m_readLat_ns, Read, req, src, m_model.getCurrentSimTimeNano(), callback );
        }

      private:

        struct Entry {

			Entry( SimTime_t delay, Op op, MemReq* memReq, UnitBase* src, Callback* callback, SimTime_t qTime ) :
                delay(delay), op(op), memReq(memReq), src(src), callback( callback ), qTime(qTime)
            { }
            SimTime_t delay;
            Op op;
			MemReq* memReq;
            UnitBase* src;
			Callback* callback;
            SimTime_t qTime;
        };

        bool work( SimTime_t delay, Op op, MemReq* req,  UnitBase* src, SimTime_t qTime, Callback* callback = NULL ) {

			m_addrs->addData( req->addr  );
            if ( m_pending == m_numSlots ) {

				m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"blocking src\n");
				m_blocked.push( Entry( delay, op, req, src, callback, m_model.getCurrentSimTimeNano() ) );
				m_blockedTime = m_model.getCurrentSimTimeNano();
                return true;
            }

            ++m_pending;

			Callback* cb = new Callback;

            SimTime_t issueTime  = m_model.getCurrentSimTimeNano();

            *cb = [=]()
                {
                    --m_pending;

                    SimTime_t latency = m_model.getCurrentSimTimeNano() - issueTime;

                    m_dbg.verbosePrefix(prefix(),CALL_INFO,1,MEM_MASK,"%s complete latency=%" PRIu64 " qLatency=%" PRIu64 " addr=%#" PRIx64 " length=%lu\n",
                                                        op == Read ? "Read":"Write" ,latency, issueTime-qTime, req->addr, req->length);

                    if ( callback ) {
                        m_model.schedCallback( 0, callback);
                    }

					delete req;

                    if ( ! m_blocked.empty() ) {

						SimTime_t latency = m_model.getCurrentSimTimeNano() - m_blockedTime;
						if ( latency ) {
							m_latency->addData( latency );
						}
						Entry& entry = m_blocked.front( );

                        work( entry.delay, entry.op, entry.memReq, entry.src, entry.qTime, entry.callback );
                		m_model.schedResume( 0, entry.src );
						m_blocked.pop();
                    }
                };

            m_model.schedCallback( delay, cb );

			return false;
        }

		SimTime_t m_blockedTime;
        Statistic<uint64_t>* m_latency;
        Statistic<uint64_t>* m_loads;
        Statistic<uint64_t>* m_stores;
        Statistic<uint64_t>* m_addrs;
        std::queue< Entry > m_blocked;
        int m_pending;
        int m_numSlots;
        int m_readLat_ns;
        int m_writeLat_ns;
    };
