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


class Shmem {


    class Op {
      public:
        typedef std::function<void()> Callback;
        enum Type { Wait } m_type;
        Op( Type type, NicShmemOpCmdEvent* cmd, Callback callback ) : m_type(type), m_cmd(cmd), m_callback(callback) {}
        virtual ~Op() {
			delete m_cmd;
        }
        Callback&  callback() { return m_callback; }
        virtual bool checkOp( Output&, int core ) = 0;
        bool inRange( Hermes::Vaddr addr, size_t length ) {
            //printf("%s() addr=%lu length=%lu\n",__func__,addr, length);
            return ( m_cmd->addr >= addr && m_cmd->addr + m_cmd->value.getLength() <= addr + length );
        }

      protected:
        NicShmemOpCmdEvent* m_cmd;
        Callback            m_callback;
        Hermes::Value       m_value;
    };

    class WaitOp : public Op {
      public:
        WaitOp( NicShmemOpCmdEvent* cmd, void* backing, Callback callback ) :
            Op( Wait, cmd, callback ),
            m_value( cmd->value.getType(), backing )
        {}

        bool checkOp( Output& dbg, int core ) {
            std::stringstream tmp;
            tmp << "op=" << WaitOpName(m_cmd->op) << " testValue=" << m_cmd->value << " memValue=" << m_value;
            dbg.debug( CALL_INFO,1,NIC_DBG_SHMEM,"%s core=%d %s\n",__func__,core,tmp.str().c_str());
            switch ( m_cmd->op ) {
              case Hermes::Shmem::NE:
                return m_value != m_cmd->value;
                break;
              case Hermes::Shmem::GTE:
                return m_value >= m_cmd->value;
                break;
              case Hermes::Shmem::GT:
                return m_value > m_cmd->value;
                break;
              case Hermes::Shmem::EQ:
                return m_value == m_cmd->value;
                break;
              case Hermes::Shmem::LT:
                return m_value < m_cmd->value;
                break;
              case Hermes::Shmem::LTE:
                return m_value <= m_cmd->value;
                break;
              default:
                assert(0);
            }
            return false;
        }
      private:
        Hermes::Value m_value;
    };

	struct RegionEntry {
		RegionEntry( Hermes::MemAddr addr, Hermes::Vaddr realAddr, size_t length ) : addr(addr), realAddr(addr.getSimVAddr(),addr.getBacking() ), length(length) { }
		Hermes::MemAddr addr;
		Hermes::MemAddr realAddr;
		size_t length;
	};

	std::string m_prefix;

	const char* prefix() { return m_prefix.c_str(); }
  public:
    Shmem( Nic& nic, Params& params, int id, int numVnics, Output& output, SimTime_t nic2HostDelay_ns, SimTime_t host2NicDelay_ns ) :
		m_nic( nic ), m_dbg(output), m_one( (long) 1 ),
    	m_nic2HostDelay_ns(nic2HostDelay_ns), m_host2NicDelay_ns(host2NicDelay_ns), m_engineBusy(false),m_hostBusy(false)
    {
        m_prefix = "@t:" + std::to_string(id) + ":Nic::Shmem::@p():@l ";
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"this=%p\n",this );

		m_activePuts.resize( numVnics );
		m_regMem.resize( numVnics );
		m_pendingOps.resize( numVnics );
		m_pendingPuts.resize( numVnics );
		m_pendingGets.resize( numVnics );
		m_nicCmdLatency =    params.find<int>( "nicCmdLatency", 10 );
		m_hostCmdLatency =   params.find<int>( "hostCmdLatency", 10 );
	}
    ~Shmem() {
        m_regMem.clear();
    }
	void handleEvent( NicShmemCmdEvent* event, int id );
	void handleHostEvent( NicShmemCmdEvent* event, int id );
	void handleHostEvent2( NicShmemCmdEvent* event, int id );
	void handleNicEvent( NicShmemCmdEvent* event, int id );
	void handleNicEvent2( NicShmemCmdEvent* event, int id );
	void handleNicEvent3( NicShmemCmdEvent* event, int id );

	long getPendingPuts( int core ) {
		return  m_pendingPuts[core].second.get<long>();
    }
	long getPendingGets( int core ) {
		return  m_pendingGets[core].second.get<long>();
    }

    void incPendingPuts( int core ) {
		long value = m_pendingPuts[core].second.get<long>();
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"core=%d count=%lu\n", core, value );
        m_pendingPuts[core].second += m_one;
    }
	void decPendingPuts( int core ) {

		long value = m_pendingPuts[core].second.get<long>();
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"core=%d count=%lu\n", core, value );
        assert(value>0);
		m_pendingPuts[core].second -= m_one;
		checkWaitOps( core, m_pendingPuts[core].first, m_pendingPuts[core].second.getLength() );
	}

    void incPendingGets( int core ) {
		long value = m_pendingGets[core].second.get<long>();
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"core=%d count=%lu\n", core, value );
        m_pendingGets[core].second += m_one;
    }

	void decPendingGets( int core ) {
		long value = m_pendingGets[core].second.get<long>();
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"core=%d count=%lu\n", core, value );
        assert(value>0);
		m_pendingGets[core].second -= m_one;
		checkWaitOps( core, m_pendingGets[core].first, m_pendingGets[core].second.getLength() );
	}

    std::pair<Hermes::MemAddr, size_t> findRegion( int core, uint64_t addr ) {
        for ( int i = 0; i < m_regMem[core].size(); i++ ) {
            if ( addr >= m_regMem[core][i].addr.getSimVAddr() &&
                addr < m_regMem[core][i].addr.getSimVAddr() + m_regMem[core][i].length ) {
                return std::make_pair( m_regMem[core][i].realAddr, m_regMem[core][i].length );
            }
        }
		m_dbg.fatal(CALL_INFO,0," core %d Unable to find for for addr %" PRIx64 "\n", core, addr);
		// quiet compiler warning
		assert(0);
    }

	void regMem( int id, uint64_t simAddr, size_t length, void* backing ) {
		Hermes::MemAddr addr( simAddr, backing );

	    m_dbg.verbosePrefix( prefix(),CALL_INFO,1,NIC_DBG_SHMEM,"core=%d simVAddr=%" PRIx64 " backing=%p len=%lu\n",
            id, addr.getSimVAddr(), addr.getBacking(), length );

    	m_regMem[id].push_back( RegionEntry( addr, simAddr, length)  );
	}

    void checkWaitOps( int core, Hermes::Vaddr addr, size_t length );

private:
	SimTime_t getNic2HostDelay_ns() { return m_nic2HostDelay_ns; }
	SimTime_t getHost2NicDelay_ns() { return m_host2NicDelay_ns; }
    void init( NicShmemInitCmdEvent*, int id );
    void fence( NicShmemFenceCmdEvent*, int id );
    void regMem( NicShmemRegMemCmdEvent*, int id );

    void hostWait( NicShmemOpCmdEvent*, int id );
    void hostPut( NicShmemPutCmdEvent*, int id );
    void hostPutv( NicShmemPutvCmdEvent*, int id );
    void hostGet( NicShmemGetCmdEvent*, int id );
    void hostGetv( NicShmemGetvCmdEvent*, int id );

    void hostAdd( NicShmemAddCmdEvent*, int id );
    void hostFadd( NicShmemFaddCmdEvent*, int id );
    void sameNodeCswap( NicShmemCswapCmdEvent*, int id );
    void sameNodeSwap( NicShmemSwapCmdEvent*, int id );

    void put( NicShmemPutCmdEvent*, int id );
    void putv( NicShmemPutvCmdEvent*, int id );
    void get( NicShmemGetCmdEvent*, int id );
    void getv( NicShmemGetvCmdEvent*, int id );
    void add( NicShmemAddCmdEvent*, int id );
    void fadd( NicShmemFaddCmdEvent*, int id );
    void cswap( NicShmemCswapCmdEvent*, int id );
    void swap( NicShmemSwapCmdEvent*, int id );

    void* getBacking( int core, Hermes::Vaddr addr, size_t length ) {
        m_dbg.verbosePrefix( prefix(), CALL_INFO,1,NIC_DBG_SHMEM,"core=%d addr=%#" PRIx64 "\n", core, addr );
        return  m_nic.findShmem( core, addr, length ).getBacking();
    }
	void doReduction( Hermes::Shmem::ReduOp op, int destCore, Hermes::Vaddr destAddr,
            int srcCore, Hermes::Vaddr srcAddr, size_t length, Hermes::Value::Type,
			std::vector<MemOp>& vec );

	Hermes::Value m_one;
	std::vector< std::pair< Hermes::Vaddr, Hermes::Value > > m_pendingPuts;
	std::vector< std::pair< Hermes::Vaddr, Hermes::Value > > m_pendingGets;
    Nic& m_nic;
    Output& m_dbg;
    std::vector< std::list<Op*> > m_pendingOps;

    std::vector<std::vector< RegionEntry > > m_regMem;
	SimTime_t m_nic2HostDelay_ns;
	SimTime_t m_host2NicDelay_ns;

	std::queue< std::pair< NicShmemCmdEvent*, int > > m_cmdQ;
	bool m_engineBusy;
	std::queue< std::pair< NicShmemCmdEvent*, int > > m_hostCmdQ;
	bool m_hostBusy;
	SimTime_t m_nicCmdLatency;
	SimTime_t m_hostCmdLatency;

	// number of puts that are not sent
	struct ActivePuts {
		ActivePuts() : count(0), event(NULL) {}
		NicShmemFenceCmdEvent* event;
		int count;
	};

	void incActivePuts( int id ) {
		++m_activePuts[id].count;
	}

	void decActivePuts( int id ) {
		ActivePuts& info = m_activePuts[id];
		assert( info.count );
		--info.count;
		if ( 0 == info.count && info.event ) {
			 m_nic.getVirtNic(id)->notifyShmem( getNic2HostDelay_ns(), info.event->callback );
			 delete info.event;
			 info.event = NULL;
		}
	}
	std::vector< ActivePuts > m_activePuts;
};
