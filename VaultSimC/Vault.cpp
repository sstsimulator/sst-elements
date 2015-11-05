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
#include <AddressMapping.h>
#include "Vault.h"

using namespace std;

#define NO_STRING_DEFINED "N/A"

Vault::Vault(Component *comp, Params &params) : SubComponent(comp) 
{
    out.init("", 0, 0, Output::STDOUT);
    
    int debugLevel = params.find_integer("debug_level", 0);
    dbg.init("@R:Vault::@p():@l: ", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    if (debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");
    dbg.init("Vault", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));  

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

    onFlyHmcOps.reserve(ON_FLY_HMC_OP_OPTIMUM_SIZE);
    bankBusyMap.reserve(BANK_BOOL_MAP_OPTIMUM_SIZE);
    computePhaseMap.reserve(BANK_BOOL_MAP_OPTIMUM_SIZE);
    computeDoneCycleMap.reserve(BANK_BOOL_MAP_OPTIMUM_SIZE);
    unlockAllBanks();
    transQ.reserve(TRANS_Q_OPTIMUM_SIZE);
    resetAllComputePhase();

    // register stats
    statTotalTransactions = registerStatistic<uint64_t>("TOTAL_TRANSACTIONS", "0");  
    statTotalHmcOps       = registerStatistic<uint64_t>("TOTAL_HMC_OPS", "0");
    statTotalNonHmcOps    = registerStatistic<uint64_t>("TOTAL_NON_HMC_OPS", "0");
    statTotalHmcLatency   = registerStatistic<uint64_t>("HMC_OPS_TOTAL_LATENCY", "0");
    statIssueHmcLatency   = registerStatistic<uint64_t>("HMC_OPS_ISSUE_LATENCY", "0");
    statReadHmcLatency    = registerStatistic<uint64_t>("HMC_OPS_READ_LATENCY", "0");
    statWriteHmcLatency   = registerStatistic<uint64_t>("HMC_OPS_WRITE_LATENCY", "0");

    currentClockCycle = 0;
}

void Vault::finish() 
{
}

void Vault::readComplete(unsigned id, uint64_t addr, uint64_t cycle) 
{
    // Check for atomic
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addr);

    // Not found in map, not atomic
    if (mi == onFlyHmcOps.end()) {
        // DRAMSim returns ID that is useless to us
        (*readCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p callback(read) at cycle=%lu\n", 
                id, (void*)addr, cycle);
    } else { 
        // Found in atomic
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) read req answer has been received in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        /* statistics */
        mi->second.readDoneCycle = currentClockCycle;
        // mi->second.setHmcOpState(READ_ANS_RECV);

        // Now in Compute Phase, set cycle done 
        // issueAtomicSecondMemoryPhase(mi);
        issueAtomicComputePhase(mi);
        setComputePhase(mi->second.getBankNo());
    }
}

void Vault::writeComplete(unsigned id, uint64_t addr, uint64_t cycle) 
{
    // Check for atomic
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addr);

    // Not found in map, not atomic
    if (mi == onFlyHmcOps.end()) {
        // DRAMSim returns ID that is useless to us
        (*writeCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p callback(write) at cycle=%lu\n", 
                id, (void*)addr, cycle);
    } else {
        // Found in atomic
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) write answer has been received in cycle=%lu\n",
                id, (void*)mi->second.getAddr(),  mi->second.getBankNo(), cycle);

        // mi->second.setHmcOpState(WRITE_ANS_RECV);
        // return as a read since all hmc ops comes as read
        (*readCallback)(id, addr, cycle);
        dbg.debug(_L7_, "Vault %d:hmc: Atomic op %p (bank%u) callback at cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        /* statistics */
        mi->second.writeDoneCycle = currentClockCycle;
        statTotalHmcLatency->addData(mi->second.writeDoneCycle - mi->second.inCycle);
        statIssueHmcLatency->addData(mi->second.issueCycle - mi->second.inCycle);
        statReadHmcLatency->addData(mi->second.readDoneCycle - mi->second.issueCycle);
        statWriteHmcLatency->addData(mi->second.writeDoneCycle - mi->second.readDoneCycle);

        unlockBank(mi->second.getBankNo());
        onFlyHmcOps.erase(mi);
    }
}

void Vault::update() 
{
    memorySystem->update();
    currentClockCycle++;  
    
    // If we are in compute phase, check for cycle compute done
    for(unsigned bankId = 0; bankId < getComputePhaseSize(); bankId++) {
        if (getComputePhase(bankId)) {
            if (currentClockCycle >= getComputeDoneCycle(bankId)) {
                uint64_t addrCompute = getAddrCompute(bankId);
                dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) compute phase has been done in cycle=%lu\n", 
                        id, (void*)addrCompute, bankId, currentClockCycle);
                addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addrCompute);
                issueAtomicSecondMemoryPhase(mi);
                resetComputePhase(bankId);
            }
        }
    }

    // Process Queue
    updateQueue();
}

bool Vault::addTransaction(transaction_c transaction) 
{
    unsigned newChan, newRank, newBank, newRow, newColumn;
    DRAMSim::addressMapping(transaction.getAddr(), newChan, newRank, newBank, newRow, newColumn);
    transaction.setBankNo(newBank);
    // transaction.setHmcOpState(QUEUED);
    
    /* statistics */
    transaction.inCycle = currentClockCycle;
    statTotalTransactions->addData(1);
    transQ.push_back(transaction);

    updateQueue();
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
                dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) of type %s issued in cycle=%lu\n", 
                        id, (void*)transQ[i].getAddr(), transQ[i].getBankNo(), transQ[i].getHmcOpTypeStr(), currentClockCycle);

                // Issue First Phase
                issueAtomicFirstMemoryPhase(mi);
                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                statTotalHmcOps->addData(1);
                mi->second.issueCycle = currentClockCycle;
            } else { // Not atomic op
                // Issue to DRAM
                memorySystem->addTransaction(transQ[i].getIsWrite(), transQ[i].getAddr());
                dbg.debug(_L8_, "Vault %d: %s %p (bank%u) issued in cycle=%lu\n", 
                        id, transQ[i].getIsWrite() ? "Write" : "Read", (void*)transQ[i].getAddr(), transQ[i].getBankNo(), currentClockCycle);

                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                statTotalNonHmcOps->addData(1);
            }
        }
    }
}

void Vault::issueAtomicFirstMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) 1st_mem phase started in cycle=%lu\n", 
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
        mi->second.resetIsWrite(); //FIXME: check if isWrite flag conceptioally is correct in hmc2 ops
        if (mi->second.getIsWrite()) {
            dbg.fatal(CALL_INFO, -1, "Atomic operation write flag should not be write\n");
        }

        memorySystem->addTransaction(mi->second.getIsWrite(), mi->second.getAddr());
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) read req has been issued in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(READ_ISSUED);
        break;
    case (HMC_NONE):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic\n");
        break;
    }
}

void Vault::issueAtomicSecondMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) 2nd_mem phase started in cycle=%lu\n", id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

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
        mi->second.setIsWrite();
        if (!mi->second.getIsWrite()) {
            dbg.fatal(CALL_INFO, -1, "Atomic operation write flag should be write (2nd phase)\n");
        }

        memorySystem->addTransaction(mi->second.getIsWrite(), mi->second.getAddr());
        dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) write has been issued (2nd phase) in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(WRITE_ISSUED);
        break;
    case (HMC_NONE):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (2nd phase)\n");
        break;
    }
}


void Vault::issueAtomicComputePhase(addr2TransactionMap_t::iterator mi) 
{
    dbg.debug(_L8_, "Vault %d:hmc: Atomic op %p (bank%u) compute phase started in cycle=%lu\n", 
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
        computeDoneCycleMap[bankNoCompute] = currentClockCycle + 3;
        break;
    case (HMC_NONE):
    default:
        dbg.fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (compute phase)\n");
        break;
    }
}
