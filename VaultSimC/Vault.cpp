// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015 Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <string>
#include "Vault.h"

#ifdef USE_VAULTSIM_HMC
#include "logicLayer.h"
#endif

using namespace std;

#define NO_STRING_DEFINED "N/A"

Vault::Vault(Component *comp, Params &params) : SubComponent(comp) 
{
    // Debug and Output Initialization
    out.init("", 0, 0, Output::STDOUT);

    int debugLevel = params.find_integer("debug_level", 0);
    if (debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg.init("@R:Vault::@p():@l: ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));

    dbgOnFlyHmcOpsIsOn = params.find_integer("debug_OnFlyHmcOps", 0);
    dbgOnFlyHmcOps.init("onFlyHmcOps: ", 0, 0, (Output::output_location_t)dbgOnFlyHmcOpsIsOn);
    if (1 == dbgOnFlyHmcOpsIsOn) {
        dbgOnFlyHmcOpsThresh = params.find_integer("debug_OnFlyHmcOpsThresh", -1);
        if (-1 == dbgOnFlyHmcOpsThresh)
            dbg.fatal(CALL_INFO, -1, "vault.debug_OnFlyHmcOpsThresh is set to 1, definition of vault.debug_OnFlyHmcOpsThresh is required as well");
    }

    statsFormat = params.find_integer("statistics_format", 0);

    // HMC Cost Initialization
    HMCCostLogicalOps = params.find_integer("HMCCost_LogicalOps", 0);
    HMCCostCASOps = params.find_integer("HMCCost_CASOps", 0);
    HMCCostCompOps = params.find_integer("HMCCost_CompOps", 0);
    HMCCostAdd8 = params.find_integer("HMCCost_Add8", 0);
    HMCCostAdd16 = params.find_integer("HMCCost_Add16", 0);
    HMCCostAddDual = params.find_integer("HMCCost_AddDual", 0);
    HMCCostFPAdd = params.find_integer("HMCCost_FPAdd", 0);
    HMCCostSwap = params.find_integer("HMCCost_Swap", 0);
    HMCCostBitW = params.find_integer("HMCCost_BitW", 0);

    // DRAMSim2 Initialization
    string deviceIniFilename = params.find_string("device_ini", NO_STRING_DEFINED);
    if (NO_STRING_DEFINED == deviceIniFilename)
        dbg.fatal(CALL_INFO, -1, "Define a 'device_ini' file parameter\n");

    string systemIniFilename = params.find_string("system_ini", NO_STRING_DEFINED);
    if (NO_STRING_DEFINED == systemIniFilename)
        dbg.fatal(CALL_INFO, -1, "Define a 'system_ini' file parameter\n");

    string pwd = params.find_string("pwd", ".");
    string logFilename = params.find_string("logfile", "log");

    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
    if (0 == ramSize)
        dbg.fatal(CALL_INFO, -1, "DRAMSim mem_size parameter set to zero. Not allowed, must be power of two in megs.\n");

    id = params.find_integer("id", -1);
    string idStr = std::to_string(id);
    string traceFilename = "VAULT_" + idStr + "_EPOCHS";

    dbg.output(CALL_INFO, "deviceIniFilename = %s, systemIniFilename = %s, pwd = %s, traceFilename = %s\n", 
            deviceIniFilename.c_str(), systemIniFilename.c_str(), pwd.c_str(), traceFilename.c_str());

    memorySystem = DRAMSim::getMemorySystemInstance(deviceIniFilename, systemIniFilename, pwd, traceFilename, ramSize); 

    DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t> *readDataCB = 
        new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::readComplete);
    DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t> *writeDataCB = 
        new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::writeComplete);

    memorySystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);

    //Transaction Support
    #ifdef USE_VAULTSIM_HMC
    addrTransMap.reserve(ACTIVE_TRANS_OPTIMUM_SIZE);
    #endif

    // etc Initialization
    onFlyHmcOps.reserve(ON_FLY_HMC_OP_OPTIMUM_SIZE);
    bankBusyMap.reserve(BANK_BOOL_MAP_OPTIMUM_SIZE);
    computeDoneCycleMap.reserve(BANK_BOOL_MAP_OPTIMUM_SIZE);
    unlockAllBanks();
    transQ.reserve(TRANS_Q_OPTIMUM_SIZE);

    currentClockCycle = 0;

    // Stats Initialization
    statTotalTransactions = registerStatistic<uint64_t>("Total_transactions", "0");  
    statTotalHmcOps       = registerStatistic<uint64_t>("Total_hmc_ops", "0");
    statTotalNonHmcOps    = registerStatistic<uint64_t>("Total_non_hmc_ops", "0");
    statTotalHmcCandidate = registerStatistic<uint64_t>("Total_candidate_hmc_ops", "0");

    statTotalNonHmcRead   = registerStatistic<uint64_t>("Total_non_hmc_read", "0");
    statTotalNonHmcWrite  = registerStatistic<uint64_t>("Total_non_hmc_write", "0");

    statTotalHmcLatency   = registerStatistic<uint64_t>("Hmc_ops_total_latency", "0");
    statIssueHmcLatency   = registerStatistic<uint64_t>("Hmc_ops_issue_latency", "0");
    statReadHmcLatency    = registerStatistic<uint64_t>("Hmc_ops_read_latency", "0");
    statWriteHmcLatency   = registerStatistic<uint64_t>("Hmc_ops_write_latency", "0");

    statTotalHmcLatencyInt = 0;
    statIssueHmcLatencyInt = 0;
    statReadHmcLatencyInt = 0;
    statWriteHmcLatencyInt = 0;

    statMemTransTotalProcessed = registerStatistic<uint64_t>("Total_memory_transaction_processed", "0");
    statMemTransTotalBegProcessed = registerStatistic<uint64_t>("Total_memory_transaction_begin_processed", "0");
    statMemTransTotalEndProcessed = registerStatistic<uint64_t>("Total_memory_transaction_end_processed", "0");
    statMemTransTotalMidProcessed = registerStatistic<uint64_t>("Total_memory_transaction_middle_processed", "0");
    statMemTransTotalConflict = registerStatistic<uint64_t>("Total_memory_trasactions_confilict", "0");
    statMemTransTotalConflictHappened = registerStatistic<uint64_t>("Total_memory_trasactions_confilict_happened", "0");
    statMemTransTotalRetired = registerStatistic<uint64_t>("Total_memory_trasactions_retired", "0");
}

void Vault::finish() 
{
    dbg.debug(_L7_, "Vault %d finished\n", id);
    //Print Statistics
    if (statsFormat == 1)
        printStatsForMacSim();
}

void Vault::readComplete(unsigned id, uint64_t addr, uint64_t cycle) 
{
    // Check for atomic
    #ifdef USE_VAULTSIM_HMC
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addr);
    #else
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.end();
    #endif

    // Not found in map, not atomic
    if (mi == onFlyHmcOps.end()) {
        // DRAMSim returns ID that is useless to us
        (*readCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p callback(read) @cycle=%lu\n", 
                id, (void*)addr, cycle);
    } 
    else { 
        // Found in atomic
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) read req answer has been received @cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        // Now in Compute Phase, set cycle done 
        issueAtomicComputePhase(mi);
        computePhaseEnabledBanks.push_back(mi->second.getBankNo());

        /* statistics */
        mi->second.readDoneCycle = currentClockCycle;
        // mi->second.setHmcOpState(READ_ANS_RECV);
    }

    /** Transaction Support */
    #ifdef USE_VAULTSIM_HMC
    unordered_map<uint64_t, transaction_c>::iterator miTrans = addrTransMap.find(addr);
    if ( miTrans != addrTransMap.end()) {
        uint64_t transId = miTrans->second.getTransId();
        vaultTransCount[transId]++;
        if ( vaultConflictedTrans.find(transId) ==  vaultConflictedTrans.end() ) {
            if (vaultTransCount[transId] == vaultTransSize[transId]) {
                vaultDoneTrans.push(transId);
                statMemTransTotalRetired->addData(1);
            }
            addrTransMap.erase(miTrans);
            dbg.debug(_L3_, "Vault %d Transaction %lu of type %s sent to logicLayer (%lu from %lu)\n", \
                    id, transId, miTrans->second.getHmcOpTypeStr(), vaultTransCount[transId], vaultTransSize[transId]);
        }
        else {
            addrTransMap.erase(miTrans);
            dbg.debug(_L3_, "Vault %d Conflicted Transaction %lu of type %s sent to logicLayer (%lu from %lu)\n", \
                    id, transId, miTrans->second.getHmcOpTypeStr(), vaultTransCount[transId], vaultTransSize[transId]);
            if (vaultTransCount[transId] == vaultTransSize[transId]) {
                vaultConflictedTransDone.push(transId);
                dbg.debug(_L3_, "Vault %d Conflicted Transaction %lu Conflicted DONE sent to logicLayer\n", id, transId);
                statMemTransTotalConflict->addData(1);
            }
        }
    }
    #endif
}

void Vault::writeComplete(unsigned id, uint64_t addr, uint64_t cycle) 
{
    // Check for atomic
    #ifdef USE_VAULTSIM_HMC
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addr);
    #else
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.end();
    #endif

    // Not found in map, not atomic
    if (mi == onFlyHmcOps.end()) {
        // DRAMSim returns ID that is useless to us
        (*writeCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p callback(write) @cycle=%lu\n", 
                id, (void*)addr, cycle);
    } 
    else {
        // Found in atomic
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) write answer has been received @cycle=%lu\n",
                id, (void*)mi->second.getAddr(),  mi->second.getBankNo(), cycle);

        // mi->second.setHmcOpState(WRITE_ANS_RECV);
        // return as a write since all hmc ops comes as read
        (*writeCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p (bank%u) callback at cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        /* statistics */
        mi->second.writeDoneCycle = currentClockCycle;
        statTotalHmcLatency->addData(mi->second.writeDoneCycle - mi->second.inCycle);
        statIssueHmcLatency->addData(mi->second.issueCycle - mi->second.inCycle);
        statReadHmcLatency->addData(mi->second.readDoneCycle - mi->second.issueCycle);
        statWriteHmcLatency->addData(mi->second.writeDoneCycle - mi->second.readDoneCycle);

        statTotalHmcLatencyInt += (mi->second.writeDoneCycle - mi->second.inCycle);
        statIssueHmcLatencyInt += (mi->second.issueCycle - mi->second.inCycle);
        statReadHmcLatencyInt += (mi->second.readDoneCycle - mi->second.issueCycle);
        statWriteHmcLatencyInt += (mi->second.writeDoneCycle - mi->second.readDoneCycle);

        // unlock
        unlockBank(mi->second.getBankNo());
        onFlyHmcOps.erase(mi);
    }

    /** Transaction Support */
    #ifdef USE_VAULTSIM_HMC
    unordered_map<uint64_t, transaction_c>::iterator miTrans = addrTransMap.find(addr);
    if ( miTrans != addrTransMap.end()) {
        uint64_t transId = miTrans->second.getTransId();
        vaultTransCount[transId]++;
        if ( vaultConflictedTrans.find(transId) ==  vaultConflictedTrans.end() ) {
            if (vaultTransCount[transId] == vaultTransSize[transId]) {
                vaultDoneTrans.push(transId);
                statMemTransTotalRetired->addData(1);
            }
            addrTransMap.erase(miTrans);
            dbg.debug(_L3_, "Vault %d Transaction %lu of type %s sent to logicLayer (%lu from %lu)\n", \
                    id, transId, miTrans->second.getHmcOpTypeStr(), vaultTransCount[transId], vaultTransSize[transId]);
        }
        else {
            addrTransMap.erase(miTrans);
            dbg.debug(_L3_, "Vault %d Conflicted Transaction %lu of type %s sent to logicLayer (%lu from %lu)\n", \
                    id, transId, miTrans->second.getHmcOpTypeStr(), vaultTransCount[transId], vaultTransSize[transId]);
            if (vaultTransCount[transId] == vaultTransSize[transId]) {
                vaultConflictedTransDone.push(transId);
                dbg.debug(_L3_, "Vault %d Conflicted Transaction %lu Conflicted DONE sent to logicLayer\n", id, transId);
                statMemTransTotalConflict->addData(1);
            }
        }
    }
    #endif
}

void Vault::update() 
{
    memorySystem->update();
    currentClockCycle++;  
    
    // If we are in compute phase, check for cycle compute done
    if (!computePhaseEnabledBanks.empty())
        for(list<unsigned>::iterator it = computePhaseEnabledBanks.begin(); it != computePhaseEnabledBanks.end(); NULL) {
            unsigned bankId = *it;
            if (currentClockCycle >= getComputeDoneCycle(bankId)) {
                uint64_t addrCompute = getAddrCompute(bankId);
                dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) compute phase has been done @cycle=%lu\n", \
                        id, (void*)addrCompute, bankId, currentClockCycle);
                addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addrCompute);
                issueAtomicSecondMemoryPhase(mi);
                it = computePhaseEnabledBanks.erase(it);
            }
            else
                it++;
        }

    // Debug long hmc ops in Queue
    if (1 == dbgOnFlyHmcOpsIsOn)
        for (auto it = onFlyHmcOps.begin(); it != onFlyHmcOps.end(); it++)
            if ( !it->second.getFlagPrintDbgHMC() )
                if (currentClockCycle - it->second.inCycle > dbgOnFlyHmcOpsThresh) {
                    it->second.setFlagPrintDbgHMC();
                    dbgOnFlyHmcOps.output(CALL_INFO, "Vault %u: Warning HMC op %p is onFly for %d cycles @cycle %lu\n", \
                                         id, (void*)it->second.getAddr(), dbgOnFlyHmcOpsThresh, currentClockCycle);
                }


    // Process Queue
    updateQueue();
}

bool Vault::addTransaction(transaction_c transaction) 
{
    unsigned newChan, newRank, newBank, newRow, newColumn;
    DRAMSim::addressMapping(transaction.getAddr(), newChan, newRank, newBank, newRow, newColumn);
    transaction.setBankNo(newBank);       //FIXME: newRank * MAX_BANK_SIZE + newBank - Why not implemented: performance issues
    // transaction.setHmcOpState(QUEUED);
    bool insertTrans = true;

    /** Transaction Support */
    #ifdef USE_VAULTSIM_HMC
    if (vaultTransActive[id]) {
        // Check for transaction conflict
        uint8_t opHMCType = transaction.getHmcOpType();
        if ( !(opHMCType == HMC_TRANS_BEG || opHMCType == HMC_TRANS_MID || opHMCType == HMC_TRANS_END) ) {
            auto it = vaultBankTrans[id].find(newBank);
            if ( it != vaultBankTrans[id].end() || !it->second.empty()) {
                for (auto itTransId = vaultBankTrans[id][newBank].begin(); itTransId!=vaultBankTrans[id][newBank].end(); ++itTransId) {
                    vaultConflictedTrans.insert(*itTransId);

                    dbg.debug(_L3_, "*CONFILICT* Vault %d Transction %p of type %s conflicted with transaction %lu (bank%u)\n", \
                            id, (void*)transaction.getAddr(), transaction.getHmcOpTypeStr(), *itTransId, newBank);
                    statMemTransTotalConflictHappened->addData(1);
                }
            }
        }
        // Save transaction address
        else {
            uint64_t transId = transaction.getTransId();
            if ( vaultConflictedTrans.find(transId) ==  vaultConflictedTrans.end() ) {
                addrTransMap.insert(pair<uint64_t, transaction_c>(transaction.getAddr(), transaction));
                dbg.debug(_L3_, "Vault %d Transction %lu of type %s received\n", id, transId, transaction.getHmcOpTypeStr());
                insertTrans = true;
            }
            // if its conflicted dump and send back answer
            else {
                vaultTransCount[transId]++;
                if (vaultTransCount[transId] == vaultTransSize[transId])
                    vaultConflictedTransDone.push(transId);
                unsigned id = 0;
                bool isWrite_ = transaction.getIsWrite();
                if (isWrite_)
                    (*writeCallback)(id, transaction.getAddr(), currentClockCycle);
                else
                    (*readCallback)(id, transaction.getAddr(), currentClockCycle);
                insertTrans = false;
                dbg.debug(_L3_, "Vault %d Conflicted Transction %lu recived & dumped\n", id, transId);
            }
            //stats
            statMemTransTotalProcessed->addData(1);
            switch (transaction.getHmcOpType()) {
                case HMC_TRANS_BEG:
                    statMemTransTotalBegProcessed->addData(1);
                    break;
                case HMC_TRANS_MID:
                    statMemTransTotalMidProcessed->addData(1);
                    break;
                case HMC_TRANS_END:
                    statMemTransTotalEndProcessed->addData(1);
                    break;
            }
        }
    }
    #endif

    /* statistics & insert to Queue*/
    if(insertTrans){
        transaction.inCycle = currentClockCycle;
        statTotalTransactions->addData(1);
        transQ.push_back(transaction);

        updateQueue();
    }

    return true;
}

void Vault::updateQueue() 
{
    // Check transaction Queue and if bank is not lock, issue transactions
    for (unsigned i = 0; i < transQ.size(); i++) { //FIXME: unoptimized erase of vector (change container or change iteration mode)
        // Bank is unlock
        if (!getBankState(transQ[i].getBankNo())) {
            if (transQ[i].getAtomic()) {
                // Lock the bank
                lockBank(transQ[i].getBankNo());

                // Add to onFlyHmcOps
                onFlyHmcOps[transQ[i].getAddr()] = transQ[i];
                addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(transQ[i].getAddr());
                dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) of type %s issued @cycle=%lu\n", 
                        id, (void*)transQ[i].getAddr(), transQ[i].getBankNo(), transQ[i].getHmcOpTypeStr(), currentClockCycle);

                // Issue First Phase
                issueAtomicFirstMemoryPhase(mi);
                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                statTotalHmcOps->addData(1);
                mi->second.issueCycle = currentClockCycle;
            } 
            else { // Not atomic op
                // Issue to DRAM
                bool isWrite_ = transQ[i].getIsWrite();
                memorySystem->addTransaction(isWrite_, transQ[i].getAddr());
                dbg.debug(_L8_, "Vault %d: %s %p (bank%u) issued @cycle=%lu\n", 
                        id, transQ[i].getIsWrite() ? "Write" : "Read", (void*)transQ[i].getAddr(), transQ[i].getBankNo(), currentClockCycle);

                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                statTotalNonHmcOps->addData(1);
                if (isWrite_)
                    statTotalNonHmcWrite->addData(1);
                else
                    statTotalNonHmcRead->addData(1);
                if (transQ[i].getHmcOpType() == HMC_CANDIDATE)
                    statTotalHmcCandidate->addData(1);
                
            }
        }
    }
}

void Vault::issueAtomicFirstMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) 1st_mem phase started @cycle=%lu\n", 
            id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

    switch (mi->second.getHmcOpType()) {
    case (HMC_CAS_equal_16B):
    case (HMC_CAS_zero_16B):
    case (HMC_CAS_greater_16B):
    case (HMC_CAS_less_16B):
    case (HMC_ADD_16B):
    case (HMC_ADD_8B):
    case (HMC_ADD_DUAL):
    case (HMC_SWAP):
    case (HMC_BIT_WR):
    case (HMC_AND):
    case (HMC_NAND):
    case (HMC_OR):
    case (HMC_XOR):
    case (HMC_FP_ADD):
    case (HMC_COMP_greater):
    case (HMC_COMP_less):
    case (HMC_COMP_equal):
        if (!mi->second.getIsWrite()) {
            dbg.fatal(CALL_INFO, -1, "Atomic operation write flag should be write\n");
        }

        memorySystem->addTransaction(false, mi->second.getAddr());
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) read req has been issued @cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(READ_ISSUED);
        break;
    case (HMC_NONE):
    case (HMC_TRANS_BEG):
    case (HMC_TRANS_MID):
    case (HMC_TRANS_END):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic\n");
        break;
    }
}

void Vault::issueAtomicSecondMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) 2nd_mem phase started @cycle=%lu\n", id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

    switch (mi->second.getHmcOpType()) {
    case (HMC_CAS_equal_16B):
    case (HMC_CAS_zero_16B):
    case (HMC_CAS_greater_16B):
    case (HMC_CAS_less_16B):
    case (HMC_ADD_16B):
    case (HMC_ADD_8B):
    case (HMC_ADD_DUAL):
    case (HMC_SWAP):
    case (HMC_BIT_WR):
    case (HMC_AND):
    case (HMC_NAND):
    case (HMC_OR):
    case (HMC_XOR):
    case (HMC_FP_ADD):
    case (HMC_COMP_greater):
    case (HMC_COMP_less):
    case (HMC_COMP_equal):
        if (!mi->second.getIsWrite()) {
            dbg.fatal(CALL_INFO, -1, "Atomic operation write flag should be write (2nd phase)\n");
        }

        memorySystem->addTransaction(true, mi->second.getAddr());
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) write has been issued (2nd phase) @cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(WRITE_ISSUED);
        break;
    case (HMC_NONE):
    case (HMC_TRANS_BEG):
    case (HMC_TRANS_MID):
    case (HMC_TRANS_END):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (2nd phase)\n");
        break;
    }
}


void Vault::issueAtomicComputePhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) compute phase started @cycle=%lu\n", 
            id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

    // mi->second.setHmcOpState(COMPUTE);
    unsigned bankNoCompute = mi->second.getBankNo();
    uint64_t addrCompute = mi->second.getAddr();
    setAddrCompute(bankNoCompute, addrCompute);

    switch (mi->second.getHmcOpType()) {
    case (HMC_CAS_equal_16B):
    case (HMC_CAS_zero_16B):
    case (HMC_CAS_greater_16B):
    case (HMC_CAS_less_16B):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostCASOps;
        break;
    case (HMC_ADD_16B):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostAdd16;
        break;
    case (HMC_ADD_8B):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostAdd8;
        break;
    case (HMC_ADD_DUAL):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostAddDual;
        break;
    case (HMC_SWAP):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostSwap;
        break;
    case (HMC_BIT_WR):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostBitW;
        break;
    case (HMC_AND):
    case (HMC_NAND):
    case (HMC_OR):
    case (HMC_XOR):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostLogicalOps;
        break;
    case (HMC_FP_ADD):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostLogicalOps;
        break;
    case (HMC_COMP_greater):
    case (HMC_COMP_less):
    case (HMC_COMP_equal):
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + HMCCostCompOps;
        break;
    case (HMC_NONE):
    case (HMC_TRANS_BEG):
    case (HMC_TRANS_MID):
    case (HMC_TRANS_END):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (compute phase)\n");
        break;
    }
}

/*
    Other Functions
*/

/*
 *  Print Macsim style output in a file
 **/

void Vault::printStatsForMacSim() {
    string name_ = "Vault" + to_string(id);
    stringstream ss;
    ss << name_.c_str() << ".stat.out";
    string filename = ss.str();

    ofstream ofs;
    ofs.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
    ofs.open(filename.c_str(), std::ios_base::out);

    float avgHmcOpsLatencyTotal = (float)statTotalHmcLatency->getCollectionCount() / statTotalHmcOps->getCollectionCount();     //FIXME: this is wrong (getCollectionCount return #of elements)
    float avgHmcOpsLatencyIssue = (float)statIssueHmcLatency->getCollectionCount() / statTotalHmcOps->getCollectionCount();     //FIXME: this is wrong (getCollectionCount return #of elements)
    float avgHmcOpsLatencyRead  = (float)statReadHmcLatency->getCollectionCount() / statTotalHmcOps->getCollectionCount();      //FIXME: this is wrong (getCollectionCount return #of elements)
    float avgHmcOpsLatencyWrite = (float)statWriteHmcLatency->getCollectionCount() / statTotalHmcOps->getCollectionCount();     //FIXME: this is wrong (getCollectionCount return #of elements)

    float avgHmcOpsLatencyTotalInt = (float)statTotalHmcLatencyInt / statTotalHmcOps->getCollectionCount();
    float avgHmcOpsLatencyIssueInt = (float)statIssueHmcLatencyInt / statTotalHmcOps->getCollectionCount();
    float avgHmcOpsLatencyReadInt = (float)statReadHmcLatencyInt / statTotalHmcOps->getCollectionCount();
    float avgHmcOpsLatencyWriteInt = (float)statWriteHmcLatencyInt / statTotalHmcOps->getCollectionCount();

    writeTo(ofs, name_, string("total_trans"),                      statTotalTransactions->getCollectionCount());
    writeTo(ofs, name_, string("total_HMC_ops"),                    statTotalHmcOps->getCollectionCount());
    writeTo(ofs, name_, string("total_non_HMC_ops"),                statTotalNonHmcOps->getCollectionCount());
    writeTo(ofs, name_, string("total_HMC_candidate_ops"),          statTotalHmcCandidate->getCollectionCount());
    ofs << "\n";
    writeTo(ofs, name_, string("total_non_HMC_read"),               statTotalNonHmcRead->getCollectionCount());
    writeTo(ofs, name_, string("total_non_HMC_write"),              statTotalNonHmcWrite->getCollectionCount());
    ofs << "\n";
    writeTo(ofs, name_, string("avg_HMC_ops_latency_total"),        avgHmcOpsLatencyTotalInt);
    writeTo(ofs, name_, string("avg_HMC_ops_latency_issue"),        avgHmcOpsLatencyIssueInt);
    writeTo(ofs, name_, string("avg_HMC_ops_latency_read"),         avgHmcOpsLatencyReadInt);
    writeTo(ofs, name_, string("avg_HMC_ops_latency_write"),        avgHmcOpsLatencyWriteInt);
    ofs << "\n";
    writeTo(ofs, name_, string("total_memory_transaction_processed"),   statMemTransTotalProcessed->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_transaction_begin_processed"),   statMemTransTotalBegProcessed->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_transaction_end_processed"),   statMemTransTotalEndProcessed->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_transaction_middle_processed"),   statMemTransTotalMidProcessed->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_trasactions_confilict"),   statMemTransTotalConflict->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_trasactions_confilict_happened"),   statMemTransTotalConflictHappened->getCollectionCount());
    writeTo(ofs, name_, string("total_memory_trasactions_retired"),   statMemTransTotalRetired->getCollectionCount());
}


// Helper function for printing statistics in MacSim format
template<typename T>
void Vault::writeTo(ofstream &ofs, string prefix, string name, T count)
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