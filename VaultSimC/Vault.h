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

#ifndef VAULT_H
#define VAULT_H

#include <cstring>
#include <string>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <set>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include <sst/core/subcomponent.h>
#include <sst/core/output.h>
#include <sst/core/params.h>

#include "globals.h"
#include "transaction.h"
#include "callback_hmc.h"

#include <DRAMSim.h>
#include <AddressMapping.h>

using namespace std;
using namespace SST;

#define ON_FLY_HMC_OP_OPTIMUM_SIZE 10
#define TRANS_Q_OPTIMUM_SIZE 10

//#define MAX_BANK_SIZE                   // FIXME: used for mapping (rank,bank) pair to a single number

class Vault : public SubComponent {
private:
    typedef CallbackBase<void, unsigned, uint64_t, uint64_t> callback_t;
    typedef unordered_map<uint64_t, transaction_c> addr2TransactionMap_t;
    typedef unordered_map<unsigned, bool> bank2BoolMap_t;
    typedef unordered_map<unsigned, uint64_t> bank2CycleMap_t;
    typedef unordered_map<unsigned, uint64_t> bank2AddrMap_t;
    typedef vector<transaction_c> transQ_t;

public:
    /** 
     * Constructor
     */
    Vault();

    /** 
     * Constructor
     * @param id Vault id
     * @param dbg VaultSimC() class wrapper sst debuger
     */
    Vault(Component *comp, Params &params);

    /**
     * finish
     * Callback function that gets to be called when simulation finishes
     */
    void finish();

    /** 
     * update
     * Vaultsim handle to update DRAMSIM, it also increases the cycle 
     */
    void update();

    /** 
     * registerCallback
     */
    void registerCallback(callback_t *rc, callback_t *wc) { readCallback = rc; writeCallback = wc; }

    /** 
     * addTransaction
     * @param transaction
     * Adds a transaction to the transaction queue, with some local initialization for transaction
     */
    bool addTransaction(transaction_c transaction);

    /** 
     * readComplete
     * DRAMSim calls this function when it is done with a read
     */
    void readComplete(unsigned id, uint64_t addr, uint64_t cycle);

    /**
     * writeComplete
     * DRAMSim calls this function when it is done with a write
     */
    void writeComplete(unsigned id, uint64_t addr, uint64_t cycle);

    /** 
     * getId
     */
    unsigned getId() { return id; }

private:
    /** 
     * updateQueue
     * update transaction queue and issue read/write to DRAM
     */
    void updateQueue();

    /**
     * issueAtomicPhases
     */
    void issueAtomicFirstMemoryPhase(addr2TransactionMap_t::iterator mi);
    void issueAtomicSecondMemoryPhase(addr2TransactionMap_t::iterator mi);
    void issueAtomicComputePhase(addr2TransactionMap_t::iterator mi);


    /** 
     * Bank BusyMap Functions
     */
    inline bool getBankState(unsigned bankId) { return bankBusyMap[bankId]; }
    inline void unlockBank(unsigned bankId) { bankBusyMap[bankId] = false; }
    inline void lockBank(unsigned bankId) { bankBusyMap[bankId] = true; }
    inline void unlockAllBanks() {
        for (unsigned i = 0; i < BANK_SIZE_OPTIMUM; i++) {
            bankBusyMap[i] = false;
        }
    }

    /** 
     * Compute Phase Functions 
     */
    inline void setComputeDoneCycle(unsigned bankId, uint64_t cycle) { computeDoneCycleMap[bankId] = cycle; }
    inline uint64_t getComputeDoneCycle(unsigned bankId) { return computeDoneCycleMap[bankId]; }
    inline void eraseComputeDoneCycle(unsigned bankId) { computeDoneCycleMap.erase(bankId); }

    inline void setAddrCompute(unsigned bankId, uint64_t addr) { addrComputeMap[bankId] = addr; }
    inline uint64_t getAddrCompute(unsigned bankId) { return addrComputeMap[bankId]; }
    inline void eraseAddrCompute(unsigned bankId) { addrComputeMap.erase(bankId); }

    /**
     *  Stats
     */
    // Helper function for printing statistics in MacSim format
    template<typename T>
    void writeTo(ofstream &ofs, string prefix, string name, T count);
    void printStatsForMacSim();

public:
    unsigned id;
    uint64_t currentClockCycle;

    callback_t *readCallback;
    callback_t *writeCallback;

    

private:
    DRAMSim::MultiChannelMemorySystem *memorySystem;
    int bankMappingScheme;
    
    //Debugs
    Output dbg;                                  // VaulSimC wrapper dbg, for printing debuging commands
    Output out;                                  // VaulSimC wrapper output, for printing always printed info and stats
    Output dbgOnFlyHmcOps;                       // For debugging long lasting hmc_ops in queue
    int dbgOnFlyHmcOpsIsOn;                      // For debuggung late onFlyHMC ops (bool variable)
    int dbgOnFlyHmcOpsThresh;                    // For debuggung late onFlyHMC ops (threshhold Value)

    //Stat Format
    int statsFormat;                             // Type of Stat output 0:Defualt 1:Macsim (Default Value is set to 0)

    addr2TransactionMap_t onFlyHmcOps;           // Currently issued atomic ops
    bank2BoolMap_t bankBusyMap;                  // Current Busy Banks
    transQ_t transQ;                             // Transaction Queue
    list<unsigned> computePhaseEnabledBanks;     // Current Compute Phase Insturctions (same size as bankBusyMap)
    bank2CycleMap_t computeDoneCycleMap;         // Current Compute Done Cycle ((same size as bankBusyMap)
    bank2AddrMap_t addrComputeMap;

    //Transaction Support
    #ifdef USE_VAULTSIM_HMC
    unordered_multimap<uint64_t, transaction_c> addrTransMap;
    #endif

    // HMC ops Cost in Cycles
    int HMCCostLogicalOps;
    int HMCCostCASOps;
    int HMCCostCompOps;
    int HMCCostAdd8;
    int HMCCostAdd16;
    int HMCCostAddDual;
    int HMCCostFPAdd;
    int HMCCostSwap;
    int HMCCostBitW;

    // stats
    Statistic<uint64_t>* statTotalTransactions;
    Statistic<uint64_t>* statTotalHmcOps;
    Statistic<uint64_t>* statTotalNonHmcOps;
    Statistic<uint64_t>* statTotalHmcCandidate;

    Statistic<uint64_t>* statTotalNonHmcRead;
    Statistic<uint64_t>* statTotalNonHmcWrite;

    Statistic<uint64_t>* statTotalHmcLatency;
    Statistic<uint64_t>* statIssueHmcLatency;
    Statistic<uint64_t>* statReadHmcLatency;
    Statistic<uint64_t>* statWriteHmcLatency;

    Statistic<uint64_t>* statMemTransTotalProcessed;
    Statistic<uint64_t>* statMemTransTotalBegProcessed;
    Statistic<uint64_t>* statMemTransTotalEndProcessed;
    Statistic<uint64_t>* statMemTransTotalMidProcessed;
    Statistic<uint64_t>* statMemTransTotalConflict;
    Statistic<uint64_t>* statMemTransTotalConflictHappened;
    Statistic<uint64_t>* statMemTransTotalRetired;

    vector<uint64_t> statMemTransTotalConflictBanks; //FIXME: convert to sst type stats

    // internal stats
    uint64_t statTotalHmcLatencyInt;   //statapi does not provide any non-collection type addData (ORno documentation)
    uint64_t statIssueHmcLatencyInt;
    uint64_t statReadHmcLatencyInt;
    uint64_t statWriteHmcLatencyInt;


};
#endif
