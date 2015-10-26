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

#include "DRAMSim2/DRAMSim.h"
#include "Vault.h"

using namespace std;
using namespace DRAMSim;

#define STAT_UPDATE(X,Y) if(isStatSet()) X=Y
#define STAT_UPDATE_PLUS(X,Y) if(isStatSet()) X+=Y

Vault::Vault() : id(0), currentClockCycle(0) 
{
    exit(0);
}

Vault::Vault(unsigned _id, Output* _dbg, bool _statistics, string _frequency) : 
    id(_id), currentClockCycle(0), dbg(_dbg), statistics(_statistics), frequency(_frequency) 
{
    string idStr = std::to_string(id);
    dramsim = getMemorySystemInstance(
            "DRAMSim2/ini/DDR3_micron_8M_2B_x8_sg08.ini",   //dev
            "DRAMSim2/system.ini.example",                  //sys file
            ".",                                            //pwd
            "VAULT_" + idStr + "_EPOCHS",                   //trace
            256);                                           //megs of memory

    TransactionCompleteCB *rc = 
        new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::readComplete);
    TransactionCompleteCB *wc = 
        new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::writeComplete);
    dramsim->RegisterCallbacks(rc, wc, NULL);

    onFlyHmcOps.reserve(ON_FLY_HMC_OP_OPTIMUM_SIZE);
    bankBusyMap.reserve(BANK_BUSY_MAP_OPTIMUM_SIZE);
    unlockAllBanks();
    transQ.reserve(TRANS_Q_OPTIMUM_SIZE);
    //resetComputePhase();

}

void Vault::finish() 
{
    if (statistics) printStats();
}

void Vault::printStats() 
{
    cout << "-------------------------------------------Vault #" << id;
    cout << "--------------------------------------------------" << endl;
    cout << setw(10) <<"Vault #" << id << endl;
    cout << setw(10) <<"Freqency:" << frequency << endl;
    cout << endl;

    cout << setw(35) << "Total Trans:"                        << 
            setw(30) << stats.totalTransactions << endl;
    cout << setw(35) << "Total HMC Ops:"                      << 
            setw(30) << stats.totalHmcOps << endl;
    cout << setw(35) << "Total Non-HMC Ops:"                  << 
            setw(30) << stats.totalNonHmcOps << endl;
    cout << setw(35) << "Avg HMC Ops Latency (Total):"        << 
            setw(30) << (float)stats.totalHmcLatency / stats.totalHmcOps << endl;
    cout << setw(35) << "Avg HMC Ops Latency (Issue):"        << 
            setw(30) << (float)stats.issueHmcLatency / stats.totalHmcOps << endl;
    cout << setw(35) << "Avg HMC Ops Latency (Read):"         << 
            setw(30) << (float)stats.readHmcLatency / stats.totalHmcOps << endl;
    cout << setw(35) << "Avg HMC Ops Latency (Write):"        << 
            setw(30) << (float)stats.writeHmcLatency / stats.totalHmcOps << endl;
}

void Vault::initStats() 
{
    stats.totalTransactions = 0;
    stats.totalHmcOps = 0;
    stats.totalNonHmcOps = 0;
    stats.totalHmcLatency = 0;
    stats.issueHmcLatency = 0;
    stats.readHmcLatency = 0;
    stats.writeHmcLatency = 0;

    dbg->output(CALL_INFO, " Vault %d: stats init done in cycle=%lu\n", id, currentClockCycle);
}

void Vault::readComplete(unsigned id, uint64_t addr, uint64_t cycle) 
{
    // Check for atomic
    addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addr);

    // Not found in map, not atomic
    if (mi == onFlyHmcOps.end()) {
        // DRAMSim returns ID that is useless to us
        (*readCallback)(id, addr, cycle);
    } else { 
        // Found in atomic
        dbg->output(CALL_INFO, " Vault %d:hmc: Atomic op %p (bank%u) read req answer has been received in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        /* statistics */
        STAT_UPDATE(mi->second.readDoneCycle, currentClockCycle);
        // mi->second.setHmcOpState(READ_ANS_RECV);

        // Now in Compute Phase, set cycle done 
        issueAtomicSecondMemoryPhase(mi);
        // issueAtomicComputePhase(mi);
        // setComputePhase();
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
    } else {
        // Found in atomic
        dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) write answer has been received in cycle=%lu\n",
                id, (void*)mi->second.getAddr(),  mi->second.getBankNo(), cycle);

        // mi->second.setHmcOpState(WRITE_ANS_RECV);
        // return as a read since all hmc ops comes as read
        (*readCallback)(id, addr, cycle);
        dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) done in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), cycle);

        /* statistics */
        if (isStatSet()) {
            mi->second.writeDoneCycle = currentClockCycle;
            stats.totalHmcLatency += mi->second.writeDoneCycle - mi->second.inCycle;
            stats.issueHmcLatency += mi->second.issueCycle - mi->second.inCycle;
            stats.readHmcLatency += mi->second.readDoneCycle - mi->second.issueCycle;
            stats.writeHmcLatency += mi->second.writeDoneCycle - mi->second.readDoneCycle;
        }

        unlockBank(mi->second.getBankNo());
        onFlyHmcOps.erase(mi);
    }
}

void Vault::update() 
{
    dramsim->update();
    currentClockCycle++;  
    
    updateQueue();
    
    // If we are in compute phase, check for cycle compute done
    /*if (getComputePhase()) {
        if (currentClockCycle >= getComputeDoneCycle()) {
            dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) compute phase has been done in cycle=%lu\n", 
                    id, (void*)addrCompute, bankNoCompute, currentClockCycle);
            addr2TransactionMap_t::iterator mi = onFlyHmcOps.find(addrCompute);
            issueAtomicSecondMemoryPhase(mi);
            resetComputePhase();
        }
    }*/
}

bool Vault::addTransaction(transaction_c transaction) 
{
    unsigned newChan, newRank, newBank, newRow, newColumn;
    DRAMSim::addressMapping(transaction.getAddr(), newChan, newRank, newBank, newRow, newColumn);
    transaction.setBankNo(newBank);
    // transaction.setHmcOpState(QUEUED);
    
    /* statistics */
    STAT_UPDATE(transaction.inCycle, currentClockCycle);
    stats.totalTransactions++;
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
                dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) of type %s issued in cycle=%lu\n", 
                        id, (void*)transQ[i].getAddr(), transQ[i].getBankNo(), transQ[i].getHmcOpTypeStr(), currentClockCycle);

                // Issue First Phase
                issueAtomicFirstMemoryPhase(mi);
                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                stats.totalHmcOps++;
                STAT_UPDATE(mi->second.issueCycle, currentClockCycle);
            } else { // Not atomic op
                // Issue to DRAM
                dramsim->addTransaction(transQ[i].getIsWrite(), transQ[i].getAddr());
                dbg->output(CALL_INFO, "Vault %d: %s %p (bank%u) issued in cycle=%lu\n", 
                        id, transQ[i].getIsWrite() ? "Write" : "Read", (void*)transQ[i].getAddr(), transQ[i].getBankNo(), currentClockCycle);

                // Remove from Transction Queue
                transQ.erase(transQ.begin() + i);

                /* statistics */
                stats.totalNonHmcOps++;
            }
        }
    }
}

void Vault::issueAtomicFirstMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) 1st_mem phase started in cycle=%lu\n", 
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
        mi->second.resetIsWrite(); //FIXME: check if isWrite flag conceptioally is correct in hmc2 ops
        if (mi->second.getIsWrite()) {
            dbg->fatal(CALL_INFO, -1, "Atomic operation write flag should not be write\n");
        }

        dramsim->addTransaction(mi->second.getIsWrite(), mi->second.getAddr());
        dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) read req has been issued in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(READ_ISSUED);
        break;
    case (HMC_NONE):
    default:
        dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic\n");
        break;
    }
}

void Vault::issueAtomicSecondMemoryPhase(addr2TransactionMap_t::iterator mi) 
{
    dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) 2nd_mem phase started in cycle=%lu\n", id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

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
        mi->second.setIsWrite();
        if (!mi->second.getIsWrite()) {
            dbg->fatal(CALL_INFO, -1, "Atomic operation write flag should be write (2nd phase)\n");
        }

        dramsim->addTransaction(mi->second.getIsWrite(), mi->second.getAddr());
        dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) write has been issued (2nd phase) in cycle=%lu\n", 
                id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);
        // mi->second.setHmcOpState(WRITE_ISSUED);
        break;
    case (HMC_NONE):
    default:
        dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (2nd phase)\n");
        break;
    }
}

/*
void Vault::issueAtomicComputePhase(addr2TransactionMap_t::iterator mi) 
{
    dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) compute phase started in cycle=%lu\n", 
            id, (void*)mi->second.getAddr(), mi->second.getBankNo(), currentClockCycle);

    // mi->second.setHmcOpState(COMPUTE);
    addrCompute = mi->second.getAddr();
    bankNoCompute = mi->second.getBankNo();

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
        m_computeDoneCycle = currentClockCycle + 3;
        break;
    case (HMC_NONE):
    default:
        dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (compute phase)\n");
        break;
    }
}*/
