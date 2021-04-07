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

//SST includes
#include "sst_config.h"

#include <assert.h>
#include <iostream>
#include <string>
#include <ctime>
#include <cmath>

//local includes
#include "c_TxnGen.hpp"
#include "c_TxnReqEvent.hpp"
#include "c_TxnResEvent.hpp"



using namespace SST;
using namespace SST::CramSim;

c_TxnGenBase::c_TxnGenBase(ComponentId_t x_id, Params& x_params) :
        Component(x_id) {


    int verbosity = x_params.find<int>("verbose", 0);
    debug = new SST::Output("CramSim.TxnGen[@f:@l:@p] ",
            verbosity, 0, SST::Output::STDOUT);
    output = new SST::Output("", 1, 0, SST::Output::STDOUT);

    m_simCycle=0;
    /*---- LOAD PARAMS ----*/

    //used for reading params
    bool l_found = false;

    // internal params
    m_seqNum = 0;
    m_resReadCount = 0;
    m_resWriteCount = 0;
    m_numOutstandingReqs = 0;
    m_numTxns = 0;


    k_numTxnPerCycle =  x_params.find<std::uint32_t>("numTxnPerCycle", 1, l_found);
    if (!l_found) {
        output->fatal(CALL_INFO, -1, "TxnGen:: numTxnPerCycle is missing... exiting\n");
    }

    k_maxOutstandingReqs =  x_params.find<std::uint32_t>("maxOutstandingReqs", 0, l_found);
    if (!l_found) {
        output->output("TxnGen:: maxOutstandingReqs is missing... No limit to the maximum number of outstanding transactions\n");
    }

    k_maxTxns =  x_params.find<std::uint32_t>("maxTxns", 0, l_found);
    if (!l_found) {
        output->output("TxnGen:: maxTxns is missing... No limit to the maximum number of transactions\n");
        //exit(-1);
    }

    uint32_t k_numBytesPerTransaction = x_params.find<std::uint32_t>("numBytesPerTransaction", 32, l_found);
    if (!l_found) {
        output->fatal(CALL_INFO, -1, "TxnGen:: numBytesPerTransaction is missing...  exiting\n");
    }
    m_sizeOffset = (uint)log2(k_numBytesPerTransaction);

    /*---- CONFIGURE LINKS ----*/

    // request-related links
    //// send to txn unit
    //// send token chg to txn unit
    m_memLink = configureLink(
            "memLink",
            new Event::Handler<c_TxnGenBase>(this, &c_TxnGenBase::handleResEvent));

    // get configured clock frequency
    std::string l_controllerClockFreqStr = (std::string)x_params.find<std::string>("strControllerClockFrequency", "1GHz", l_found);

    //set our clock
    registerClock(l_controllerClockFreqStr,
                  new Clock::Handler<c_TxnGenBase>(this, &c_TxnGenBase::clockTic));

    // Statistics
    s_readTxnsCompleted = registerStatistic<uint64_t>("readTxnsCompleted");
    s_writeTxnsCompleted = registerStatistic<uint64_t>("writeTxnsCompleted");
    s_readTxnSent= registerStatistic<uint64_t>("readTxnsSent");
    s_writeTxnSent= registerStatistic<uint64_t>("readTxnsSent");

    s_txnsPerCycle= registerStatistic<double>("txnsPerCycle");
    s_readTxnsLatency= registerStatistic<uint64_t>("readTxnsLatency");
    s_writeTxnsLatency= registerStatistic<uint64_t>("writeTxnsLatency");
    s_txnsLatency= registerStatistic<uint64_t>("txnsLatency");

}

c_TxnGenBase::~c_TxnGenBase() {
}

c_TxnGenBase::c_TxnGenBase() :
        Component(-1) {
    // for serialization only
}

void c_TxnGenBase::finish()
{
    output->output("\n======= CramSim Simulation Report [Transaction Generator] ============================\n");
    output->output("Total Read-Txns Requests sent: %llu\n", m_resReadCount);
    output->output("Total Write-Txns Requests sent: %llu\n", m_resWriteCount);
    output->output("Total Txns Sents: %llu\n", m_resReadCount + m_resWriteCount);

    output->output("Total Read-Txns Responses received: %llu\n", m_resReadCount);
    output->output("Total Write-Txns Responses received: %llu\n", m_resWriteCount);
    output->output("Total Txns Received: %llu\n", m_resReadCount + m_resWriteCount);
    output->output("Cycles Per Transaction (CPT) = %.2f\n",
        static_cast<double>(m_simCycle) / static_cast<double>(m_resReadCount + m_resWriteCount));
    output->output("Component Finished.\n");
    output->output("========================================================================================\n\n");

    double l_txnsPerCycle=  static_cast<double>(m_resReadCount + m_resWriteCount) /static_cast<double>(m_simCycle);

    s_txnsPerCycle->addData(l_txnsPerCycle);
}

bool c_TxnGenBase::clockTic(Cycle_t) {

    m_simCycle++;

    createTxn();

    for(int i=0;i<k_numTxnPerCycle;i++) {
        if (!readResponse())
            break;

        m_numTxns++;

        if(k_maxTxns>0 && m_numTxns>=k_maxTxns) {
            primaryComponentOKToEndSim();
            return true;
        }
    }

    for(int i=0;i<k_numTxnPerCycle;i++) {
        if(k_maxOutstandingReqs==0 || m_numOutstandingReqs<k_maxOutstandingReqs) {

            if(sendRequest()==false)
                break;

            m_numOutstandingReqs++;
        } else
            break;

    }
    return false;
}



void c_TxnGenBase::handleResEvent(SST::Event* ev) {

    c_TxnResEvent* l_txnResEventPtr = dynamic_cast<c_TxnResEvent*> (ev);

    if (l_txnResEventPtr)
    {
        c_Transaction *l_txn=l_txnResEventPtr->m_payload;
        if (l_txn->getTransactionMnemonic()
            == e_TransactionType::READ) {
            s_readTxnsCompleted->addData(1);
            m_resReadCount++;
        } else {
            s_writeTxnsCompleted->addData(1);
            m_resWriteCount++;
        }

        m_numOutstandingReqs--;
        assert(m_numOutstandingReqs>=0);


	m_txnResQ.push_back(l_txn);

        delete l_txnResEventPtr;
        uint64_t l_currentCycle = m_simCycle;
        uint64_t l_seqnum=l_txn->getSeqNum();


        assert(m_outstandingReqs.find(l_seqnum)!=m_outstandingReqs.end());
        SimTime_t l_latency=l_currentCycle-m_outstandingReqs[l_seqnum];

        if(l_txn->isRead())
            s_readTxnsLatency->addData(l_latency);
        else
            s_writeTxnsLatency->addData(l_latency);

        s_txnsLatency->addData(l_latency);

#ifdef __SST_DEBUG_OUTPUT__
        debug->verbose(CALL_INFO,1,0,"[cycle:%lld] addr: 0x%lx isRead:%d seqNum:%lld birthTime:%lld latency:%lld \n",l_currentCycle,l_txn->getAddress(),l_txn->isRead(), l_seqnum,m_outstandingReqs[l_seqnum],l_latency);
#endif


        m_outstandingReqs.erase(l_seqnum);

    } else {
        std::stringstream str;
        str << std::endl << std::endl << "TxnGen:: "
            << __PRETTY_FUNCTION__ << " ERROR:: Bad Event Type!"
            << std::endl;
        output->output("%s", str.str().c_str());
    }
}



bool c_TxnGenBase::sendRequest()
{
    assert(k_maxOutstandingReqs==0 || m_numOutstandingReqs<=k_maxOutstandingReqs);
    if(!m_txnReqQ.empty())
    {
        uint64_t l_cycle=m_simCycle;

        // confirm that interval timer has run out before contiuing
        if (m_txnReqQ.front().second > l_cycle) {
            // std::stringstream str
            // str << __PRETTY_FUNCTION__ << ": Interval timer for front txn is not done"
            // << std::endl;
            // output->output("%s", str.str().c_str());
            // m_txnReqQ.front().first->print();
            // output->output(" - Interval: %d\n", m_txnReqQ.front().second);
            return false;
        }

        if (m_txnReqQ.front().first->getTransactionMnemonic()
            == e_TransactionType::READ)
        {
            s_readTxnSent->addData(1);
            m_reqReadCount++;
        }
        else
        {
            s_writeTxnSent->addData(1);
            m_reqWriteCount++;
        }

        c_TxnReqEvent* l_txnReqEvPtr = new c_TxnReqEvent();
        l_txnReqEvPtr->m_payload = m_txnReqQ.front().first;
        m_txnReqQ.pop_front();

        assert(m_memLink!=NULL);
        m_memLink->send(l_txnReqEvPtr);


        c_Transaction *l_txn=l_txnReqEvPtr->m_payload;
    #ifdef __SST_DEBUG_OUTPUT__
        debug->verbose(CALL_INFO,1,0,"[cycle:%lld] addr: 0x%x isRead:%d seqNum:%lld\n",l_cycle,l_txn->getAddress(),l_txn->isRead(),l_txn->getSeqNum());
    #endif

        m_outstandingReqs.insert(std::pair<uint64_t, uint64_t>(l_txn->getSeqNum(),l_cycle));
        return true;
    }
    else
        return false;
}


bool c_TxnGenBase::readResponse() {
	if (m_txnResQ.size() > 0) {
		c_Transaction* l_txn = m_txnResQ.front();
		delete l_txn;

		m_txnResQ.pop_front();
		// output->output("TxnGen::readResponse() Transaction printed: Addr-%" PRIu64 "\n",
		// 		l_txn->getAddress());
                return true;
	} else {
		// output->output("TxnGen::readResponse(): No transactions in res q to read\n");
                return false;
	}
}




c_TxnGen::c_TxnGen(ComponentId_t x_id, Params& x_params) :
        c_TxnGenBase(x_id,x_params) {

    /*---- LOAD PARAMS ----*/

    //used for reading params
    bool l_found = false;

    // internal params
    m_prevAddress = 0;

    //ratio of read txn's : write txn's to generate
    k_readWriteTxnRatio
            = x_params.find<float>("readWriteRatio", 0.5, l_found);
    if (!l_found) {
        output->output("TxnGen:: readWriteRatio value is missing... it will be 0.5 (default)\n");
        //exit(-1);
    }

    //set mode (random or sequential)
    std::string l_mode = x_params.find<std::string>("mode", "rand", l_found);
    if (!l_found) {
        output->fatal(CALL_INFO, -1, "TxnGen:: mode is missing... exiting");
    } else {
        if (l_mode == "rand")
            m_mode = e_TxnMode::RAND;
        else if (l_mode == "seq")
            m_mode = e_TxnMode::SEQ;
        else {
            output->fatal(CALL_INFO, -1, "TxnGen:: mode (%s).. It should be \"rand\" or \"seq\" .. exiting\n", l_mode.c_str());
        }
    }

    // initialize the random seed
    std::string l_randSeedStr = x_params.find<std::string>("randomSeed","0", l_found);
    l_randSeedStr.pop_back(); // remove trailing newline (??)
    if(l_randSeedStr.compare("-") == 0) { // use a random seed
        k_randSeed = (SimTime_t)time(nullptr);
    } else {
        k_randSeed = (SimTime_t)std::strtoul(l_randSeedStr.c_str(),NULL,0);
    }
    std::srand(k_randSeed);
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}




uint64_t c_TxnGen::getNextAddress() {
    if(m_mode==e_TxnMode::RAND)
        return (rand()<<m_sizeOffset);
    else
        return (m_prevAddress+(1<<m_sizeOffset));
}


void c_TxnGen::createTxn() {

    uint64_t l_cycle = m_simCycle;

    while(m_txnReqQ.size()<k_numTxnPerCycle)
    {
        uint64_t addr = getNextAddress();
        m_seqNum++;

        double l_read2write = ((double) rand() / (RAND_MAX));

        //TODO: Default value for dataWidth?
        c_Transaction* mTxn;
        if (l_read2write < k_readWriteTxnRatio)
            mTxn = new c_Transaction(m_seqNum, e_TransactionType::READ, addr, 1);
        else
            mTxn = new c_Transaction(m_seqNum, e_TransactionType::WRITE, addr, 1);

        m_prevAddress = addr;

        std::pair<c_Transaction*, uint64_t > l_entry = std::make_pair(mTxn,l_cycle);
        m_txnReqQ.push_back(l_entry);
    }
}


// Element Libarary / Serialization stuff
