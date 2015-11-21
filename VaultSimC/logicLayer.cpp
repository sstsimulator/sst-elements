// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "logicLayer.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

#ifdef USE_VAULTSIM_HMC
//Transcation GLOBAL FIXME
unordered_map<unsigned, unordered_map<unsigned, unordered_set<uint64_t> > > vaultBankTrans;
unordered_map<uint64_t, bool> vaultTransActive;
queue<uint64_t> vaultConflictedTransDone;
queue<uint64_t> vaultDoneTrans;
#endif

logicLayer::logicLayer(ComponentId_t id, Params& params) : IntrospectedComponent( id )
{
    // Debug and Output Initializatio
    out.init("", 0, 0, Output::STDOUT);

    int debugLevel = params.find_integer("debug_level", 0);
    dbg.init("@R:LogicLayer::@p():@l " + getName() + ": ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    if(debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    // logicLayer Params Initialization
    int ident = params.find_integer("llID", -1);
    if (-1 == ident)
        dbg.fatal(CALL_INFO, -1, "no llID defined\n");
    llID = ident;

    reqLimit = params.find_integer("req_LimitPerCycle", -1);
    if (-1 == reqLimit)  
        dbg.fatal(CALL_INFO, -1, " no req_LimitPerCycle param defined for logiclayer\n");

    int mask = params.find_integer("LL_MASK", -1);
    if (-1 == mask) 
        dbg.fatal(CALL_INFO, -1, " no LL_MASK param defined for logiclayer\n");
    LL_MASK = mask;

    bool terminal = params.find_integer("terminal", 0);

    // VaultSims Initializations (Links)
    numVaults = params.find_integer("vaults", -1);
    if (-1 == numVaults) 
        dbg.fatal(CALL_INFO, -1, " no vaults param defined for LogicLayer\n");
    // connect up our vaults
    for (int i = 0; i < numVaults; ++i) {
        char bus_name[50];
        snprintf(bus_name, 50, "bus_%d", i);
        memChan_t *chan = configureLink(bus_name);      //link delay is configurable by python scripts
        if (chan) {
            memChans.push_back(chan);
            dbg.debug(_INFO_, "\tConnected %s\n", bus_name);
        }
        else
            dbg.fatal(CALL_INFO, -1, " could not find %s\n", bus_name);
    }
    out.output("*LogicLayer%d: Connected %d Vaults\n", ident, numVaults);
    #ifdef USE_VAULTSIM_HMC
    out.output("*LogicLayer%d: Flag USE_VAULTSIM_HMC set\n", ident);
    #endif

    // Connect Chain
    toCPU = configureLink("toCPU");
    if (!terminal) 
        toMem = configureLink("toMem");
    else 
        toMem = NULL;
    dbg.debug(_INFO_, "Made LogicLayer %d toMem:%p toCPU:%p\n", llID, toMem, toCPU);

    // Transaction Support
    #ifdef USE_VAULTSIM_HMC
    tIdQueue.reserve(TRANS_FOOTPRINT_MAP_OPTIMUM_SIZE);
    activeTransactions.reserve(ACTIVE_TRANS_OPTIMUM_SIZE);
    vaultBankTrans.reserve(ACTIVE_TRANS_OPTIMUM_SIZE);

    for (int i = 0; i < numVaults; ++i)
        vaultTransActive[i] = false;
    #endif

    // etc
    std::string frequency;
    frequency = params.find_string("clock", "2.2 Ghz");
    registerClock(frequency, new Clock::Handler<logicLayer>(this, &logicLayer::clock));
    dbg.debug(_INFO_, "Making LogicLayer with id=%d & clock=%s\n", llID, frequency.c_str());

    CacheLineSize = params.find_integer("cacheLineSize", 64);
    CacheLineSizeLog2 = log(CacheLineSize) / log(2);

    // Stats Initialization
    statsFormat = params.find_integer("statistics_format", 0);

    memOpsProcessed = registerStatistic<uint64_t>("Total_memory_ops_processed", "0");
    HMCOpsProcessed = registerStatistic<uint64_t>("HMC_ops_processed", "0");
    HMCCandidateProcessed = registerStatistic<uint64_t>("Total_HMC_candidate_processed", "0");
    HMCTransOpsProcessed = registerStatistic<uint64_t>("Total_HMC_transactions_processed", "0");
    reqUsedToCpu[0] = registerStatistic<uint64_t>("Req_recv_from_CPU", "0");  
    reqUsedToCpu[1] = registerStatistic<uint64_t>("Req_send_to_CPU", "0");
    reqUsedToMem[0] = registerStatistic<uint64_t>("Req_recv_from_Mem", "0");
    reqUsedToMem[1] = registerStatistic<uint64_t>("Req_send_to_Mem", "0");
}

void logicLayer::finish() 
{
    dbg.debug(_INFO_, "Logic Layer %d completed %lu ops\n", llID, memOpsProcessed->getCollectionCount());
    //Print Statistics
    if (statsFormat == 1)
        printStatsForMacSim();
}

bool logicLayer::clock(Cycle_t current) 
{
    SST::Event* ev = NULL;

    // Bandwidth Stats
    int toMemory[2] = {0,0};    // {recv, send}
    int toCpu[2] = {0,0};       // {recv, send}

    // 1-a)
    /* Retire Done Transactions
     *     delete tIdQueue entry, activeTransactions entry, edit vaultTransActive, edit vaultBankTrans
     **/
     #ifdef USE_VAULTSIM_HMC
     while (!vaultDoneTrans.empty()) {
        uint64_t doneTransId = vaultDoneTrans.front();
        vaultDoneTrans.pop();

        activeTransactions.erase(doneTransId);
        tIdQueue.erase(doneTransId);

        for (auto itA = vaultBankTrans.begin(); itA != vaultBankTrans.end(); ++itA)
            for (auto itB = itA->second.begin(); itB != itA->second.end(); ++itB)
                for (auto itC = itB->second.begin(); itC != itB->second.end(); ++itC) {
                    if (*itC == doneTransId) {
                        itB->second.erase(doneTransId);
                        if (itB->second.empty()) {
                            itA->second.erase(itB);
                            vaultTransActive[itA->first] = false;
                        }
                    }
                }
        dbg.debug(_L3_, "LogicLayer%d Transaction Done %lu\n", llID, doneTransId);
     }
     #endif

    // 1-b)
    /* Check Transactions conflicts
     *     and erase entry of 
     *     
     **/
     #ifdef USE_VAULTSIM_HMC
     while ( !vaultConflictedTransDone.empty() ) {
            uint64_t conflictTransId = vaultConflictedTransDone.front();
            vaultConflictedTransDone.pop();
            transReadyQueue.push(conflictTransId);
            dbg.debug(_L3_, "LogicLayer%d conflicted transaction restarted%lu\n", llID, conflictTransId);
     }
     #endif

    // 1-c)
    /* Check For Events From CPU
     *     Check ownership, if owned send to internal vaults, if not send to another LogicLayer
     **/
    while ((toCpu[0] < reqLimit) && (ev = toCPU->recv())) {
        MemEvent *event  = dynamic_cast<MemEvent*>(ev);
        dbg.debug(_L4_, "LogicLayer%d got req for %p (%" PRIu64 " %d)\n", llID, (void*)event->getAddr(), event->getID().first, event->getID().second);
        if (NULL == event)
            dbg.fatal(CALL_INFO, -1, "LogicLayer%d got bad event\n", llID);

        // HMC Type verifications and stats
        #ifdef USE_VAULTSIM_HMC
        uint8_t HMCTypeEvent = event->getHMCInstType();
        if (HMCTypeEvent >= NUM_HMC_TYPES)
            dbg.fatal(CALL_INFO, -1, "LogicLayer%d got bad HMC type %d for address %p\n", llID, event->getHMCInstType(), (void*)event->getAddr());
        if (HMCTypeEvent == HMC_CANDIDATE)
            HMCCandidateProcessed->addData(1);
        else if (HMCTypeEvent == HMC_TRANS_BEG || HMCTypeEvent == HMC_TRANS_MID || HMCTypeEvent == HMC_TRANS_END)
            HMCTransOpsProcessed->addData(1);
        else if (HMCTypeEvent == HMC_hook || HMCTypeEvent == HMC_hook) 
             dbg.fatal(CALL_INFO, -1, "LogicLayer%d got HMC type hook/unhook %d for address %p\n", llID, event->getHMCInstType(), (void*)event->getAddr());
        else if (HMCTypeEvent != HMC_NONE && HMCTypeEvent != HMC_CANDIDATE)
            HMCOpsProcessed->addData(1);
        #endif

        toCpu[0]++;
        reqUsedToCpu[0]->addData(1);

        // (Multi LogicLayer) Check if it is for this LogicLayer
        if (isOurs(event->getAddr())) {
            bool eventIsNotTransaction = false;

            // Transaction support: Process HMC Transactions
            #ifdef USE_VAULTSIM_HMC
            if (HMCTypeEvent == HMC_TRANS_BEG) {
                eventIsNotTransaction = true;
                uint64_t IdEvent = event->getHMCTransId();
                // Add this event to Queue
                //tIdQueue.insert(pair <uint64_t, queue<MemHierarchy::MemEvent> > (IdEvent, queue<MemHierarchy::MemEvent>() ));
                tIdQueue[IdEvent].push_back(*event);
                dbg.debug(_L3_, "LogicLayer%d got transaction BEG for addr %p with id %lu\n", llID, (void*)event->getAddr(), IdEvent);
            }
            else if (HMCTypeEvent == HMC_TRANS_MID) {
                eventIsNotTransaction = true;
                uint64_t IdEvent = event->getHMCTransId();
                // Add this event to Queue
                tIdQueue[IdEvent].push_back(*event);
                dbg.debug(_L3_, "LogicLayer%d got transaction MID for addr %p with id %lu\n", llID, (void*)event->getAddr(), IdEvent);
            }
            else if (HMCTypeEvent == HMC_TRANS_END) {
                eventIsNotTransaction = true;
                uint64_t IdEvent = event->getHMCTransId();
                // Add this event to Queue
                tIdQueue[IdEvent].push_back(*event);
                //This the end of this ID. Issue
                transReadyQueue.push(IdEvent);
                dbg.debug(_L3_, "LogicLayer%d got transaction END for addr %p with id %lu\n", llID, (void*)event->getAddr(), IdEvent);
            }
            #endif

            if (!eventIsNotTransaction) {
                unsigned int vaultID = (event->getAddr() >> CacheLineSizeLog2) % memChans.size();
                memChans[vaultID]->send(event);
                dbg.debug(_L4_, "LogicLayer%d sends %p to vault%u @ %" PRIu64 "\n", llID, (void*)event->getAddr(), vaultID, current);
            }
        } 
        // This event is not for this LogicLayer
        else {
            if (NULL == toMem) 
                dbg.fatal(CALL_INFO, -1, "LogicLayer%d not sure what to do with %p...\n", llID, event);
            toMem->send(event);
            toMemory[1]++;
            reqUsedToMem[1]->addData(1);
            dbg.debug(_L4_, "LogicLayer%d sends %p to next\n", llID, event);
        }
    }

    // 1-d)
    /* Issue Transactions if they are ready
     *     and save their footprint, let vaults know to check conflicts
     **/
    #ifdef USE_VAULTSIM_HMC
    while (!transReadyQueue.empty()) { //FIXME: no limit on number of transaction issue
        unsigned currentTransId = transReadyQueue.front();
        transReadyQueue.pop();

        activeTransactions.insert(currentTransId);
        dbg.debug(_L3_, "LogicLayer%d issuing ready transaction %u with size %lu\n", llID, currentTransId, tIdQueue[currentTransId].size());

        for (vector<MemHierarchy::MemEvent>::iterator it = tIdQueue[currentTransId].begin() ; it != tIdQueue[currentTransId].end(); ++it) {
            MemEvent event = *it;
            unsigned int vaultID = (event.getAddr() >> CacheLineSizeLog2) % memChans.size();

            // Save this event footprint
            unsigned newChan, newRank, newBank, newRow, newColumn;
            DRAMSim::addressMapping(event.getAddr(), newChan, newRank, newBank, newRow, newColumn);
            vaultBankTrans[vaultID][newBank].insert(currentTransId);
            vaultTransActive[vaultID] = true;
            memChans[vaultID]->send(&event);
        }


    }
    #endif



    // 2)
    /* Check For Events From Memory Chain
     *     and send them to CPU
     **/
    if (NULL != toMem) {
        while ((toMemory[0] < reqLimit) && (ev = toMem->recv())) {
            MemEvent *event  = dynamic_cast<MemEvent*>(ev);
            if (NULL == event)
                dbg.fatal(CALL_INFO, -1, "LogicLayer%d got bad event from another LogicLayer\n", llID); 

            toMemory[0]++;
            reqUsedToMem[0]->addData(1);
    
            toCPU->send(event);
            toCpu[1]++;
            reqUsedToCpu[1]->addData(1);
            dbg.debug(_L4_, "LogicLayer%d sends %p towards cpu (%" PRIu64 " %d)\n", llID, event, event->getID().first, event->getID().second);
        }
    }

    // 3)
    /* Check For Events From Vaults 
     *     and send them to CPU
     **/
    for (memChans_t::iterator i = memChans.begin(); i != memChans.end(); ++i) {
        memChan_t *m_memChan = *i;
        while ((ev = m_memChan->recv())) {
            MemEvent *event  = dynamic_cast<MemEvent*>(ev);
            if (event == NULL)
                dbg.fatal(CALL_INFO, -1, "LogicLayer%d got bad event from vaults\n", llID);

            memOpsProcessed->addData(1);
            toCPU->send(event);
            toCpu[1]++;
            reqUsedToCpu[1]->addData(1);
            dbg.debug(_L4_, "LogicLayer%d got an event %p from vault @ %" PRIu64 ", sends towards cpu\n", llID, event, current);
        }    
    }

    if (toMemory[0] > reqLimit || toMemory[1] > reqLimit || toCpu[0] > reqLimit || toCpu[1] > reqLimit) {
        dbg.output(CALL_INFO, "logicLayer%d Bandwdith: %d %d %d %d\n", llID, toMemory[0], toMemory[1], toCpu[0], toCpu[1]);
    }

    return false;
}


extern "C" Component* create_logicLayer( SST::ComponentId_t id,  SST::Params& params ) 
{
    return new logicLayer( id, params );
}


// Determine if we 'own' a given address
bool logicLayer::isOurs(unsigned int addr) 
{
        return ((((addr >> LL_SHIFT) & LL_MASK) == llID) || (LL_MASK == 0));
}


/*
    Other Functions
*/

/*
 *  Print Macsim style output in a file
 **/

void logicLayer::printStatsForMacSim() {
    string name_ = "LogicLayer" + to_string(llID);
    stringstream ss;
    ss << name_.c_str() << ".stat.out";
    string filename = ss.str();

    ofstream ofs;
    ofs.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filename.c_str(), std::ios_base::out);

    writeTo(ofs, name_, string("total_memory_ops_processed"), memOpsProcessed->getCollectionCount());
    writeTo(ofs, name_, string("total_hmc_ops_processed"), HMCOpsProcessed->getCollectionCount());
    ofs << "\n";
    writeTo(ofs, name_, string("req_recv_from_CPU"), reqUsedToCpu[0]->getCollectionCount());
    writeTo(ofs, name_, string("req_send_to_CPU"),   reqUsedToCpu[1]->getCollectionCount());
    writeTo(ofs, name_, string("req_recv_from_Mem"), reqUsedToMem[0]->getCollectionCount());
    writeTo(ofs, name_, string("req_send_to_Mem"),   reqUsedToMem[1]->getCollectionCount());
    ofs << "\n";
    writeTo(ofs, name_, string("total_HMC_candidate_ops"),   HMCCandidateProcessed->getCollectionCount());
}


// Helper function for printing statistics in MacSim format
template<typename T>
void logicLayer::writeTo(ofstream &ofs, string prefix, string name, T count)
{
    #define FILED1_LENGTH 45
    #define FILED2_LENGTH 20
    #define FILED3_LENGTH 30

    ofs.setf(ios::left, ios::adjustfield);
    string capitalized_prefixed_name = boost::to_upper_copy(prefix + "_" + name);
    ofs << setw(FILED1_LENGTH) << capitalized_prefixed_name;

    ofs.setf(ios::right, ios::adjustfield);
    ofs << setw(FILED2_LENGTH) << count << setw(FILED3_LENGTH) << count << endl << endl;
}

