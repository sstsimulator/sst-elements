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

#include <string.h>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <sst/core/output.h>
#include <sst/core/params.h>

#include "globals.h"
#include "callback_hmc.h"
#include "transaction.h"

using namespace std;
using namespace SST;

#define ON_FLY_HMC_OP_OPTIMUM_SIZE 2
#define BANK_BUSY_MAP_OPTIMUM_SIZE 10
#define TRANS_Q_OPTIMUM_SIZE 10

namespace DRAMSim {
    class MultiChannelMemorySystem;
};

class Vault {
private:
    typedef CallbackBase<void, unsigned, uint64_t, uint64_t> callback_t;
    typedef unordered_map<uint64_t, transaction_c> addr2TransactionMap_t;
    typedef unordered_map<unsigned, bool> bank2busyMap_t;
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
    Vault(unsigned id, Output* dbg, bool statistics, string frequency);

    /**
     * finish
     * Callback function that gets to be called when simulation finishes
     */
    void finish();

    /** 
     * update
     * Vaultsim handle to update DramSim, it also increases the cycle 
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
     * issueAtomicFirstMemoryPhase
     */
    void issueAtomicFirstMemoryPhase(addr2TransactionMap_t::iterator mi);

    /**
     * issueAtomicSecondMemoryPhase
     */
    void issueAtomicSecondMemoryPhase(addr2TransactionMap_t::iterator mi);

    /**
     * issueAtomicComputePhase
     */
    //void issueAtomicComputePhase(addr2TransactionMap_t::iterator mi);

    /**
     * printStats
     */
    void printStats();

    /**
     * initStats
     */
    void initStats();

    /** 
     * bankBusyMap Functions
     */
    inline bool getBankState(unsigned bankid) { return bankBusyMap[bankid]; }
    inline void unlockBank(unsigned bankid) { bankBusyMap[bankid] = false; }
    inline void lockBank(unsigned bankid) { bankBusyMap[bankid] = true; }
    inline void unlockAllBanks() {
        for (int i = 0; i < BANK_BUSY_MAP_OPTIMUM_SIZE; i++) {
            bankBusyMap[i] = false;
        }
    }

    /** 
     * statistics functions
     */
    inline bool isStatSet() { return statistics; } 

    /** 
     * computePhase Functions 
     */
    //inline void setComputePhase() { computePhase = true; }
    //inline void resetComputePhase() { computePhase = false; }
    //inline bool getComputePhase() { return computePhase; }

    /** 
     * compupteDoneCycle Functions    
     */
    //inline void setComputeDoneCycle(uint64_t cycle) { compupteDoneCycle = cycle; }
    //inline uint64_t getComputeDoneCycle() { return compupteDoneCycle; }

public:
    unsigned id;
    uint64_t currentClockCycle;

    callback_t *readCallback;
    callback_t *writeCallback;

    DRAMSim::MultiChannelMemorySystem* dramsim;

private:
    Output* dbg;                                 // VaulSimC wrapper dbg, for printing debuging commands
    bool statistics;                             // Print statistics knob
    string frequency;                            // Vault Frequency

    addr2TransactionMap_t onFlyHmcOps;           // Currently issued atomic ops
    bank2busyMap_t bankBusyMap;                  // Current Busy Banks
    transQ_t transQ;                             // Transaction Queue

    struct statistics_t {
        uint64_t totalTransactions;
        uint64_t totalHmcOps;
        uint64_t totalNonHmcOps;

        uint64_t totalHmcLatency;
        uint64_t issueHmcLatency;
        uint64_t readHmcLatency;
        uint64_t writeHmcLatency;
    } stats;

    //bool computePhase;
    //uint64_t compupteDoneCycle;
    //unsigned int bankNoCompute;
    //uint64_t addrCompute;
};
#endif
