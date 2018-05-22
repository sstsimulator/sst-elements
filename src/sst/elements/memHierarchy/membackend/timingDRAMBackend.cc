// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/timeLord.h>
#include "membackend/timingDRAMBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

bool TimingDRAM::m_printConfig = true;
bool TimingDRAM::Channel::m_printConfig = true;
bool TimingDRAM::Rank::m_printConfig = true;
bool TimingDRAM::Bank::m_printConfig = true;

TimingDRAM::TimingDRAM(Component *comp, Params &params) : SimpleMemBackend(comp, params), m_cycle(0) {

    int id = params.find<int>("id", -1);
    assert( id != -1 );

    std::ostringstream tmp;
    tmp << "@t:TimingDRAM::@p():@l:mc=" << id << ": "; 
    
    int dbg_level = params.find<uint32_t>("dbg_level", 1);
    int dbg_mask = params.find<uint32_t>("dbg_mask", -1);

    output = new Output(tmp.str().c_str(), dbg_level, dbg_mask, Output::STDOUT);

    std::string addrMapper = params.find<std::string>("addrMapper","memHierarchy.simpleAddrMapper");

    Params tmpParams = params.find_prefix_params("addrMapper." );
    m_mapper = dynamic_cast<AddrMapper*>(loadModule( addrMapper, tmpParams ) );

    if ( ! m_mapper ) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): addrMapper,  '%s'.\n", 
            comp->getName().c_str(), addrMapper.c_str());
    }

    int numChannels = params.find<int>("channels", 1);

    if ( m_printConfig ) {
        output->verbose(CALL_INFO, 1, DBG_MASK, "number of channels: %d\n",numChannels);
        output->verbose(CALL_INFO, 1, DBG_MASK, "address mapper:     %s\n",addrMapper.c_str());
        m_printConfig = false;
    }

    m_mapper->setNumChannels( numChannels );

    tmpParams = params.find_prefix_params("channel." );
    for ( unsigned i=0; i < numChannels; i++ ) {
        m_channels.push_back( Channel( comp, this, tmpParams, id, i, output, m_mapper ) );
    }
}

bool TimingDRAM::issueRequest( ReqId id, Addr addr, bool isWrite, unsigned numBytes )
{
    unsigned chan = m_mapper->getChannel(addr);

    bool ret = m_channels[chan].issue(m_cycle, id, addr, isWrite, numBytes );

    if ( ret ) { 
        output->verbose(CALL_INFO, 2, DBG_MASK, "chan=%d reqId=%" PRIu64 " addr=%#" PRIx64 "\n",chan,id,addr);
    } else {
        output->verbose(CALL_INFO, 5, DBG_MASK, "chan=%d reqId=%" PRIu64 " addr=%#" PRIx64 " failed\n",chan,id,addr);
    }
    return ret;
}

bool TimingDRAM::clock(Cycle_t cycle)
{
    output->verbose(CALL_INFO, 5, DBG_MASK, "cycle %" PRIu64 "\n",m_cycle);
    for ( unsigned i = 0; i < m_channels.size(); i++ ) {
        m_channels[i].clock(m_cycle);
    }
    ++m_cycle;
    return false;
}

//==================================================================================
// Channel 
//==================================================================================

TimingDRAM::Channel::Channel( Component* comp, TimingDRAM* mem, Params& params, unsigned mc, unsigned myNum, Output* output, AddrMapper* mapper ) :
    m_mem(mem), m_output( output ), m_mapper( mapper ), m_nextRankUp(0), m_dataBusAvailCycle(0)
{
    std::ostringstream tmp;
    tmp << "@t:TimingDRAM:Channel:@p():@l:mc=" << mc << ":chan=" << myNum << ": "; 
    m_pre = tmp.str();

    unsigned numRanks = params.find<unsigned>("numRanks", 1);
    m_maxPendingTrans = params.find<unsigned>("transaction_Q_size", 32);

    m_pendingCount = 0;
    
    m_mapper->setNumRanks( numRanks );

    if ( m_printConfig ) {
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "max pending trans: %d\n",m_maxPendingTrans);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "number of ranks:   %d\n",numRanks);
        m_printConfig = false;
    }

    Params tmpParams = params.find_prefix_params("rank." );
    for ( unsigned i=0; i<numRanks; i++ ) {
        m_ranks.push_back( Rank( comp, tmpParams, mc, myNum, i, output, mapper ) );
    } 
}

void TimingDRAM::Channel::clock( SimTime_t cycle )
{
    m_output->verbosePrefix(prefix(),CALL_INFO, 5, DBG_MASK, "cycle %" PRIu64 "\n",cycle);

    std::list<Cmd*>::iterator iter = m_issuedCmds.begin();

    while ( iter != m_issuedCmds.end() ) {
        if ( (*iter)->isDone(cycle) ) {
            Cmd* cmd = (*iter);

            m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK, "cycle=%" PRIu64 " retire %s for rank=%d bank=%d row=%d\n",
                    cycle, cmd->getName().c_str(), cmd->getRank(), cmd->getBank(), cmd->getRow());

            if (cmd->getTrans() != nullptr) {
                m_retiredTrans.push(cmd->getTrans());
            }

            delete (*iter);
            iter = m_issuedCmds.erase(iter); 
        } else {
            ++iter;
        }
    }
 
    if ( ! m_retiredTrans.empty() ) {
        m_output->verbosePrefix(prefix(),CALL_INFO, 3, DBG_MASK, "send response: reqId=%" PRIu64 " bank=%d addr=%#" PRIx64 ", createTime=%" PRIu64 "\n", m_retiredTrans.front()->id, m_retiredTrans.front()->bank, m_retiredTrans.front()->addr, m_retiredTrans.front()->createTime);

        m_mem->handleResponse( m_retiredTrans.front()->id );
        delete m_retiredTrans.front();

        m_retiredTrans.pop();
        m_pendingCount--;
    }

    Cmd* cmd = popCmd( cycle, m_dataBusAvailCycle );
    if ( cmd ) {
        m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK, "cycle=%" PRIu64 " issue %s for rank=%d bank=%d row=%d\n",
                    cycle, cmd->getName().c_str(), cmd->getRank(), cmd->getBank(), cmd->getRow());

        m_dataBusAvailCycle = cmd->issue();  

        m_issuedCmds.push_back(cmd);
    }
}

TimingDRAM::Cmd* TimingDRAM::Channel::popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle )
{
    Cmd* cmd = NULL;
    unsigned current = m_nextRankUp;
    for ( unsigned i = 0; i < m_ranks.size(); i++ ) {

        cmd = m_ranks[current].popCmd( cycle, m_dataBusAvailCycle );

        if ( cmd ) {

            if ( current == m_nextRankUp ) {
                ++m_nextRankUp;
                m_nextRankUp %= m_ranks.size();
                m_output->verbosePrefix(prefix(),CALL_INFO, 3, DBG_MASK, "rank %d next up\n",m_nextRankUp);
            }
            
            break;
        }

        ++current;
        current %= m_ranks.size();
    }
    return cmd;
}

//==================================================================================
// Rank 
//==================================================================================

TimingDRAM::Rank::Rank( Component* comp, Params& params, unsigned mc, unsigned chan, unsigned myNum, Output* output, AddrMapper* mapper ) : 
    m_output( output ), m_mapper( mapper ), m_nextBankUp(0)
{
    std::ostringstream tmp;
    tmp << "@t:TimingDRAM:Rank:@p():@l:mc=" << mc << ":chan=" << chan << ":rank=" << myNum <<": "; 
    m_pre = tmp.str();

    int banks = params.find<int>("numBanks", 8);

    m_mapper->setNumBanks( banks );

    if ( m_printConfig ) {
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "number of banks: %d\n",banks);
        m_printConfig = false;
    }

    Params tmpParams = params.find_prefix_params("bank." );
    for ( unsigned i=0; i<banks; i++ ) {
        m_banks.push_back( Bank( comp, tmpParams, mc, chan, myNum, i, output ) );
    } 
}

TimingDRAM::Cmd* TimingDRAM::Rank::popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle )
{
    m_output->verbosePrefix(prefix(),CALL_INFO, 5, DBG_MASK, "\n" );

    unsigned current = m_nextBankUp;
    for ( unsigned i = 0; i < m_banks.size(); i++ ) {
        Cmd* cmd = m_banks[current].popCmd( cycle, dataBusAvailCycle );

        if ( cmd ) {
            if ( current == m_nextBankUp ) {
                ++m_nextBankUp;
                m_nextBankUp %= m_banks.size();
                m_output->verbosePrefix(prefix(),CALL_INFO, 3, DBG_MASK, "rank %d next up\n",m_nextBankUp);
            }
            return cmd;
        }

        ++current;
        current %= m_banks.size();
    }
    return NULL;
}

//==================================================================================
// Bank
//==================================================================================

TimingDRAM::Bank::Bank( Component* comp, Params& params, unsigned mc, unsigned chan, unsigned rank, unsigned myNum, Output* output ) : 
    m_output( output ), m_lastCmd(NULL), m_bank(myNum), m_rank(rank), m_row( -1 ) 
{
    std::ostringstream tmp;
    tmp << "@t:TimingDRAM:Bank:@p():@l:mc=" << mc << ":chan=" << chan << ":rank=" << rank << ":bank=" << myNum <<": "; 
    m_pre = tmp.str();

    m_col_rd_lat = params.find<unsigned int>("CL", 11);
    m_col_wr_lat = params.find<unsigned int>("CL_WR", m_col_rd_lat);
    m_rcd_lat = params.find<unsigned int>("RCD", 11);
    m_trp_lat = params.find<unsigned int>("TRP", 11);
    m_data_lat = params.find<unsigned int>("dataCycles", 4);

    std::string name = params.find<std::string>("transactionQ", "memHierarchy.fifoTransactionQ");
    Params tmpParams = params.find_prefix_params("transactionQ." );
    m_transQ = dynamic_cast<TransactionQ*>(comp->loadSubComponent( name, comp, tmpParams ) );

    std::string ppName = params.find<std::string>("pagePolicy", "memHierarchy.simplePagePolicy");
    tmpParams = params.find_prefix_params("pagePolicy." );
    m_pagePolicy = dynamic_cast<PagePolicy*>(comp->loadSubComponent( ppName, comp, tmpParams ) );

    if ( m_printConfig ) {
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "CL:           %d\n",m_col_rd_lat);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "CL_WR:        %d\n",m_col_wr_lat);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "RCD:          %d\n",m_rcd_lat);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "TRP:          %d\n",m_trp_lat);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "dataCycles:   %d\n",m_data_lat);
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "transactionQ: %s\n",name.c_str());
        m_output->verbosePrefix(prefix(),CALL_INFO, 1, DBG_MASK, "pagePolicy:   %s\n",  ppName.c_str());
        m_printConfig = false;
    }
}


TimingDRAM::Cmd* TimingDRAM::Bank::popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle )
{
    m_output->verbosePrefix(prefix(),CALL_INFO, 4, DBG_MASK, "numCmds=%zu\n", m_cmdQ.size() );
    update( cycle );

    Cmd* cmd = NULL;
    if ( ! m_cmdQ.empty() && m_cmdQ.front()->canIssue( cycle, dataBusAvailCycle ) ) {
        cmd = m_cmdQ.front();
        m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK, "%s row=%d\n",cmd->getName().c_str(), cmd->getRow() );
        m_cmdQ.pop_front();
    }  
    return cmd;
}

void TimingDRAM::Bank::update( SimTime_t current )
{
    if ( NULL == m_lastCmd && m_row != -1 && m_pagePolicy->shouldClose( current ) ) {
        Cmd* cmd = new Cmd( this, Cmd::PRE, m_trp_lat ); 
        m_cmdQ.push_back(cmd);
        m_row = -1;
        return;
    }

    Transaction* trans = m_transQ->pop(m_row);

    if ( ! trans ) {
        return;
    }

    m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK, "addr=%#" PRIx64 " current row=%d trans row=%d, time=%" PRIu64 "\n",
            trans->addr, m_row, trans->row, trans->createTime );

    Cmd* cmd;

    if ( trans->row != m_row ) {
        if ( m_row != -1 ) {
            cmd = new Cmd( this, Cmd::PRE, m_trp_lat ); 
            m_cmdQ.push_back(cmd);
        }

        cmd = new Cmd( this, Cmd::ACT, m_rcd_lat, trans->row ); 
        m_cmdQ.push_back(cmd);
        m_row = trans->row;
    }  

    unsigned val = trans->isWrite ? m_col_wr_lat :  m_col_rd_lat; 
    cmd = new Cmd( this, Cmd::COL, val, trans->row, m_data_lat, trans );
    m_cmdQ.push_back(cmd);
}
