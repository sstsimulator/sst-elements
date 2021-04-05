// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_TIMING_DRAM_BACKEND
#define _H_SST_MEMH_TIMING_DRAM_BACKEND

#include <queue>

#include <sst/core/componentExtension.h>

#include "sst/elements/memHierarchy/membackend/simpleMemBackend.h"
#include "sst/elements/memHierarchy/membackend/timingAddrMapper.h"
#include "sst/elements/memHierarchy/membackend/timingTransaction.h"
#include "sst/elements/memHierarchy/membackend/timingPagePolicy.h"
#include "sst/elements/memHierarchy/util.h"

namespace SST {
namespace MemHierarchy {

using namespace  TimingDRAM_NS;

class TimingDRAM : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(TimingDRAM, "memHierarchy", "timingDRAM", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Moderately-detailed timing model for DRAM", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"id", "ID number for this TimingDRAM instance", NULL},
            {"dbg_level", "Output verbosity for debug", "1"},
            {"dbg_mask", "Mask on dbg_level", "-1"},
            {"printconfig", "Print configuration at start", "true"},
            {"addrMapper", "Address map subcomponent", "memHierarchy.simpleAddrMapper"},
            {"channels", "Number of channels", "1"},
            {"channel.numRanks", "Number of ranks per channel", "1"},
            {"channel.transaction_Q_size", "Size of transaction queue", "32"},
            {"channel.rank.numBanks", "Number of banks per rank", "8"},
            {"channel.rank.bank.CL", "Column access latency in cycles", "11"},
            {"channel.rank.bank.CL_WR", "Column write latency", "11"},
            {"channel.rank.bank.RCD", "Row access latency in cycles", "11"},
            {"channel.rank.bank.TRP", "Precharge delay in cycles", "11"},
            {"channel.rank.bank.dataCycles", "", "4"},
            {"channel.rank.bank.transactionQ", "Transaction queue model (subcomponent)", "memHierarchy.fifoTransactionQ"},
            {"channel.rank.bank.pagePolicy", "Policy subcomponent for managing row buffer", "memHierarchy.simplePagePolicy"})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"transactionQ", "Transaction queue model", "SST::MemHierarchy::TimingDRAM_NS::TransactionQ"},
            {"pagePolicy", "Policy subcomponent for managing row buffer", "SST::MemHierarchy::TimingDRAM_NS::PagePolicy"} )

/* Begin class definition */
private:
    const uint64_t DBG_MASK = 0x1;

    class Cmd;

    class Bank : public ComponentExtension {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 3);
        Bank( ComponentId_t, Params&, unsigned mc, unsigned chan, unsigned rank, unsigned bank, Output* );

        void pushTrans( Transaction* trans ) {
            m_transQ->push(trans);
        }

        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );

        void setLastCmd( Cmd* cmd ) {
            m_lastCmd = cmd;
        }

        void clearLastCmd() {
            m_lastCmd = NULL;
        }

        Cmd* getLastCmd() {
            return m_lastCmd;
        }

        void verbose( int line, const char* func, const char * format, ...) {

            char buf[500];
            va_list arg;
            va_start( arg, format );
            vsnprintf( buf, 500, format, arg );
            va_end(arg);
            m_output->verbosePrefix( prefix(), line,"",func, 2, DBG_MASK, "%s", buf );
        }

        bool isIdle() {
            return (m_row == -1 || !m_pagePolicy->canClose()) && m_cmdQ.empty() && m_transQ->empty();
        }

        unsigned getRank() { return m_rank; }
        unsigned getBank() { return m_bank; }

      private:
        void update( SimTime_t );
        const char* prefix() { return m_pre.c_str(); }

        Output*             m_output;
        std::string         m_pre;

        unsigned            m_col_rd_lat;
        unsigned            m_col_wr_lat;
        unsigned            m_rcd_lat;
        unsigned            m_trp_lat;
        unsigned            m_data_lat;
        Cmd*                m_lastCmd;
//        bool                m_busy;
        unsigned            m_rank;
        unsigned            m_bank;
        unsigned            m_row;
        std::deque<Cmd*>    m_cmdQ;
        TransactionQ*       m_transQ;
        PagePolicy*         m_pagePolicy;
    };

    class Cmd {
      public:
        enum Op { PRE, ACT, COL } m_op;
        Cmd( Bank* bank, Op op, unsigned cycles, unsigned row = -1, unsigned dataCycles = 0, Transaction* trans  = NULL  ) :
            m_bank(bank), m_op(op), m_cycles(cycles), m_row(row), m_dataCycles(dataCycles), m_trans(trans)
        {
            switch( m_op ) {
              case PRE:
                m_name = "PRE";
                break;
              case ACT:
                m_name = "ACT";
                break;
              case COL:
                m_name = "COL";
                break;
            }
            if (is_debug)
                m_bank->verbose(__LINE__,__FUNCTION__,"new %s for rank=%d bank=%d row=%d\n",
                        getName().c_str(), getRank(), getBank(), getRow());
        }

        ~Cmd() {
            m_bank->clearLastCmd();
        }

        SimTime_t issue() {

            m_bank->setLastCmd(this);

            return m_dataBusAvailCycle;
        }

        bool canIssue( SimTime_t currentCycle, SimTime_t dataBusAvailCycle ) {
            bool ret=false;

            Cmd* lastCmd = m_bank->getLastCmd();

            if ( lastCmd ) {
#if 0
                if ( lastCmd->m_trans && lastCmd->m_trans->req->isWrite_ && m_op == PRE ) {
                    printf( "WR to PRE\n" );
                }
#endif
                if ( m_op == COL && lastCmd->m_op == COL ) {
                    if ( lastCmd->m_trans->isWrite && ! m_trans->isWrite ) {
#if 0
                        printf("WR to RD\n");
                        assert(0);
#endif
                    }
                    if ( lastCmd->m_issueTime + m_dataCycles > currentCycle ) {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            m_issueTime = currentCycle;
            m_finiTime = currentCycle + m_cycles;
            m_dataBusAvailCycle = dataBusAvailCycle;

            if ( m_finiTime >= dataBusAvailCycle ) {
                m_finiTime += m_dataCycles;
                m_dataBusAvailCycle = m_finiTime;
                ret = true;
            } else if (is_debug) {
                m_bank->verbose(__LINE__,__FUNCTION__,"bus not ready\n");
            }

            return ret;
        }

        bool isDone( SimTime_t now ) {

            if (is_debug)
                m_bank->verbose(__LINE__,__FUNCTION__,"%lu %lu\n",now,m_finiTime);
            return ( now >= m_finiTime );
        }

        // these are used for debugging
        std::string& getName()  { return m_name; }
        unsigned getRank()      { return m_bank->getRank(); }
        unsigned getBank()      { return m_bank->getBank(); }
        unsigned getRow()       { return m_row; }
        Transaction* getTrans() { return m_trans; }
      private:

        Bank*           m_bank;
        std::string     m_name;
        unsigned        m_cycles;
        unsigned        m_row;
        unsigned        m_dataCycles;
        Transaction*    m_trans;

        SimTime_t       m_issueTime;
        SimTime_t       m_finiTime;
        SimTime_t       m_dataBusAvailCycle;
    };

    class Rank : public ComponentExtension {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 2);

        Rank( ComponentId_t, Params&, unsigned mc, unsigned chan, unsigned rank, Output*, AddrMapper* );

        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );

        void pushTrans( Transaction* trans ) {
            unsigned bank = m_mapper->getBank( trans->addr);

            if (is_debug)
                m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK,"bank=%d addr=%#" PRIx64 "\n",
                    bank,trans->addr);

            m_banks[bank]->pushTrans( trans );

            m_banksActive.insert(bank);
        }

        bool hasActiveBanks() {
            return !m_banksActive.empty();
        }

      private:

        const char* prefix() { return m_pre.c_str(); }
        Output*         m_output;
        AddrMapper*     m_mapper;
        std::string     m_pre;

        unsigned            m_nextBankUp;
        std::vector<Bank*>  m_banks;
        std::set<unsigned>  m_banksActive;
    };

    class Channel : public ComponentExtension {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 1);

        Channel( ComponentId_t, std::function<void(ReqId)>, Params&, unsigned mc, unsigned chan, Output*, AddrMapper* );

        bool issue( SimTime_t createTime, ReqId id, Addr addr, bool isWrite, unsigned numBytes ) {

            if ( m_maxPendingTrans == m_pendingCount ) {
                return false;
            }

            unsigned rank = m_mapper->getRank( addr);

            if (is_debug)
                m_output->verbosePrefix(prefix(),CALL_INFO, 3, DBG_MASK,"reqId=%" PRIu64 " rank=%d addr=%#" PRIx64 ", createTime=%" PRIu64 "\n", id, rank, addr, createTime );

            Transaction* trans = new Transaction( createTime, id, addr, isWrite, numBytes, m_mapper->getBank(addr),
                                                m_mapper->getRow(addr) );
            m_pendingCount++;
            m_ranks[ rank ]->pushTrans( trans );
            return true;
        }

        void clock(SimTime_t );

      private:
        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );
        const char* prefix() { return m_pre.c_str(); }
        Output*             m_output;
        AddrMapper*         m_mapper;
        std::string         m_pre;

        unsigned            m_nextRankUp;
        std::vector<Rank*>  m_ranks;

        unsigned            m_dataBusAvailCycle;
        unsigned            m_maxPendingTrans;
        unsigned            m_pendingCount;

        std::list<Cmd*>     m_issuedCmds;
        std::queue<Transaction*> m_retiredTrans;

        std::function<void(ReqId)> m_responseHandler;
    };

    static bool m_printConfig;

public:
    TimingDRAM();
    TimingDRAM(ComponentId_t, Params& );
    virtual bool issueRequest( ReqId, Addr, bool, unsigned );
    void handleResponse(ReqId  id ) {
        output->verbose(CALL_INFO, 2, DBG_MASK, "req=%" PRIu64 "\n", id );
        handleMemResponse( id );
    }
    virtual bool clock(Cycle_t cycle);
    virtual void finish() {}

private:
    std::vector<Channel*> m_channels;
    AddrMapper* m_mapper;
    SimTime_t   m_cycle;

};

}
}

#endif
