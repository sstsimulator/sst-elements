/*
Copyright (c) <2012>, <Georgia Institute of Technology> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted 
provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions 
and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this list of 
conditions and the following disclaimer in the documentation and/or other materials provided 
with the distribution.

Neither the name of the <Georgia Institue of Technology> nor the names of its contributors 
may be used to endorse or promote products derived from this software without specific prior 
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/


/**********************************************************************************************
 * File         : Vault.cpp
 * Author       : HPArch
 * Date         : 9/18/2015
 * Description  : Vault (DRAMsim instance wrapper)
 *********************************************************************************************/

#include "DRAMSim2/DRAMSim.h"
#include "Vault.h"

using namespace std;
using namespace DRAMSim;

#define STAT_UPDATE(X,Y) if(is_stat_set()) X=Y
#define STAT_UPDATE_PLUS(X,Y) if(is_stat_set()) X+=Y

//////////////////////////////////////////////////////////////////////////
// Constructors
//////////////////////////////////////////////////////////////////////////
Vault::Vault() : m_id(0), m_currentClockCycle(0) {
  exit(0);
}

/** VaultSim constructor
 *  Initializes a DRAMSim instance
 */

Vault::Vault(unsigned id, Output* dbg, bool statistics, string frequency) : 
  m_id(id), m_currentClockCycle(0), m_dbg(dbg), m_statistics(statistics), m_frequency(frequency) 
{
  string m_id_string = std::to_string(m_id);
  m_dramsim = getMemorySystemInstance(
      "DRAMSim2/ini/DDR3_micron_8M_2B_x8_sg08.ini",   //dev
      "DRAMSim2/system.ini.example",                  //sys file
      ".",                                            //pwd
      "VAULT_" + m_id_string + "_EPOCHS",             //trace
      256);                                           //megs of memory

  TransactionCompleteCB *rc = new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::read_complete);
  TransactionCompleteCB *wc = new DRAMSim::Callback<Vault, void, unsigned, uint64_t, uint64_t>(this, &Vault::write_complete);
  m_dramsim->RegisterCallbacks(rc, wc, NULL);

  m_onFlyHmcOps.reserve(ON_FLY_HMC_OP_OPTIMUM_SIZE);
  m_bankBusyMap.reserve(BANK_BUSY_MAP_OPTIMUM_SIZE);
  unlock_all_banks();
  m_transQ.reserve(TRANS_Q_OPTIMUM_SIZE);
  //reset_computePhase();

}

//////////////////////////////////////////////////////////////////////////
// Finish
//////////////////////////////////////////////////////////////////////////
/** Called when simualtion is done
 */

void Vault::finish() {
  if (m_statistics)
    print_stats();
}

//////////////////////////////////////////////////////////////////////////
// Stats
//////////////////////////////////////////////////////////////////////////

void Vault::print_stats() {
  cout << "-------------------------------------------Vault #" << m_id;
  cout << "--------------------------------------------------" << endl;
  cout << setw(10) <<"Vault #" << m_id << endl;
  cout << setw(10) <<"Freqency:" << m_frequency << endl;
  cout << endl;
  cout << setw(35) << "Total Trans:"                        << setw(30) << stats.total_trans << endl;
  cout << setw(35) << "Total HMC Ops:"                      << setw(30) << stats.total_hmc_ops << endl;
  cout << setw(35) << "Total Non-HMC Ops:"                  << setw(30) << stats.total_non_hmc_ops << endl;
  cout << setw(35) << "Avg HMC Ops Latency (Total):"        << setw(30) << (float)stats.total_hmc_latency / stats.total_hmc_ops << endl;
  cout << setw(35) << "Avg HMC Ops Latency (Issue):"        << setw(30) << (float)stats.issue_hmc_latency / stats.total_hmc_ops << endl;
  cout << setw(35) << "Avg HMC Ops Latency (Read):"         << setw(30) << (float)stats.read_hmc_latency / stats.total_hmc_ops << endl;
  cout << setw(35) << "Avg HMC Ops Latency (Write):"        << setw(30) << (float)stats.write_hmc_latency / stats.total_hmc_ops << endl;
}


void Vault::init_stats() {
    stats.total_trans = 0;
    stats.total_hmc_ops = 0;
    stats.total_non_hmc_ops = 0;

    stats.total_hmc_latency = 0;
    stats.issue_hmc_latency = 0;
    stats.read_hmc_latency = 0;
    stats.write_hmc_latency = 0;

    m_dbg->output(CALL_INFO, " Vault %d: stats init done in cycle=%lu\n", m_id, m_currentClockCycle);
}

//////////////////////////////////////////////////////////////////////////
// DRAMSim Callback Functions
//////////////////////////////////////////////////////////////////////////

void Vault::read_complete(unsigned id, uint64_t addr, uint64_t cycle) {
  //Check for atomic
  addr2TransactionMap_t::iterator mi = m_onFlyHmcOps.find(addr);
  //Not found in map, not atomic
  if ( mi == m_onFlyHmcOps.end()) {
    // DRAMSim returns ID that is useless to us
    (*read_callback)(id, addr, cycle);
  }
  //Found in atomic
  else {
    m_dbg->output(CALL_INFO, " Vault %d:hmc: Atomic op %p (bank%u) read req answer has been received in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), cycle);
    /* stats */
    STAT_UPDATE(mi->second.m_read_done_cycle, m_currentClockCycle);
    //mi->second.set_HMCOpState(READ_ANS_RECV);
    //Now in Compute Phase, set cycle done 
    issueAtomic_second_memory_phase(mi);
    //issueAtomic_compute_phase(mi);
    //set_computePhase();
  }
  
}

void Vault::write_complete(unsigned id, uint64_t addr, uint64_t cycle) {
  //Check for atomic
  addr2TransactionMap_t::iterator mi = m_onFlyHmcOps.find(addr);
  //Not found in map, not atomic
  if ( mi == m_onFlyHmcOps.end()) {
    // DRAMSim returns ID that is useless to us
    (*write_callback)(id, addr, cycle);
  }
  //Found in atomic
  else {
    m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) write answer has been received in cycle=%lu\n", m_id, (void*)mi->second.get_addr(),  mi->second.get_bankNo(), cycle);
    //mi->second.set_HMCOpState(WRITE_ANS_RECV);
    //return as a read since all hmc ops comes as read
    (*read_callback)(id, addr, cycle);
    m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) done in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), cycle);
    /* stats */
    if (is_stat_set()) {
      mi->second.m_write_done_cycle = m_currentClockCycle;
      stats.total_hmc_latency += mi->second.m_write_done_cycle - mi->second.m_in_cycle;
      stats.issue_hmc_latency += mi->second.m_issue_cycle - mi->second.m_in_cycle;
      stats.read_hmc_latency += mi->second.m_read_done_cycle - mi->second.m_issue_cycle;
      stats.write_hmc_latency += mi->second.m_write_done_cycle - mi->second.m_read_done_cycle;
    }

    unlock_bank(mi->second.get_bankNo());

    m_onFlyHmcOps.erase(mi);
  }
  
}

//////////////////////////////////////////////////////////////////////////
// Public Functions 
//////////////////////////////////////////////////////////////////////////
/** update
    Get called from upper class
 */

void Vault::Update() {
  m_dramsim->update();
  m_currentClockCycle++;  



  Update_queue();

  // If we are in compute phase, check for cycle compute done
  /*
  if (get_computePhase())
    if (m_currentClockCycle >= get_compute_done_cycle()) {
      m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) compute phase has been done in cycle=%lu\n", m_id, (void*)m_addrCompute, m_bankNoCompute, m_currentClockCycle);
      addr2TransactionMap_t::iterator mi = m_onFlyHmcOps.find(m_addrCompute);
      issueAtomic_second_memory_phase(mi);
      reset_computePhase();
    }
  */
}

bool Vault::AddTransaction(transaction_c transaction) {
  unsigned newTransactionChan, newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn;
  DRAMSim::addressMapping(transaction.get_addr(), newTransactionChan, newTransactionRank, newTransactionBank, newTransactionRow, newTransactionColumn);
  transaction.set_bankNo(newTransactionBank);
  //transaction.set_HMCOpState(QUEUED);
  /* stats */
  STAT_UPDATE(transaction.m_in_cycle, m_currentClockCycle);
  stats.total_trans++;

  m_transQ.push_back(transaction);

  Update_queue();

  return true;
}

//////////////////////////////////////////////////////////////////////////
// Internal update queue function
//////////////////////////////////////////////////////////////////////////

void Vault::Update_queue() {
  //Check transaction Queue and if bank is not lock, issue transactions
  for (int i=0; i<m_transQ.size(); i++) { //FIXME: unoptimized erase of vector (change container or change iteration mode)
    // Bank is unlock
    if(get_bank_state(m_transQ[i].get_bankNo()) == false) {
      if (m_transQ[i].get_atomic()) {
        // Lock the bank
        lock_bank(m_transQ[i].get_bankNo());
        // Add to onFlyHmcOps
        m_onFlyHmcOps[m_transQ[i].get_addr()] = m_transQ[i];
        addr2TransactionMap_t::iterator mi = m_onFlyHmcOps.find(m_transQ[i].get_addr());
        m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) of type %s issued in cycle=%lu\n", m_id, (void*)m_transQ[i].get_addr(), m_transQ[i].get_bankNo(), m_transQ[i].get_HMCOpType_str(), m_currentClockCycle);
        // Issue First Phase
        issueAtomic_first_memory_phase(mi);
        // Remove from Transction Queue
        m_transQ.erase(m_transQ.begin()+i);
        /* stats */
        stats.total_hmc_ops++;
        STAT_UPDATE(mi->second.m_issue_cycle, m_currentClockCycle);
      }
      else { // Not atomic op
        // Issue to DRAM
        m_dramsim->addTransaction(m_transQ[i].is_write(), m_transQ[i].get_addr());
        m_dbg->output(CALL_INFO, "Vault %d: %s %p (bank%u) issued in cycle=%lu\n",  m_id, m_transQ[i].is_write() ? "Write" : "Read", (void*)m_transQ[i].get_addr(), m_transQ[i].get_bankNo(), m_currentClockCycle);
        // Remove from Transction Queue
        m_transQ.erase(m_transQ.begin()+i);
        /* stats */
        stats.total_non_hmc_ops++;
      }
    }
  }

}

//////////////////////////////////////////////////////////////////////////
// Atomic ops Phase functions
//////////////////////////////////////////////////////////////////////////

void Vault::issueAtomic_first_memory_phase(addr2TransactionMap_t::iterator mi) {
  m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) 1st_mem phase started in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), m_currentClockCycle);

  switch (mi->second.get_HMCOpType()) {
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
      mi->second.reset_is_write(); //FIXME: check if is_write flag conceptioally is correct in hmc2 ops
      if (mi->second.is_write())
        m_dbg->fatal(CALL_INFO, -1, "Atomic operation write flag should not be write\n");
      m_dramsim->addTransaction(mi->second.is_write(), mi->second.get_addr());
      m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) read req has been issued in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), m_currentClockCycle);
      //mi->second.set_HMCOpState(READ_ISSUED);
      break;
    case (HMC_NONE):
    default:
      m_dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic\n");
      break;
  }

    

}

void Vault::issueAtomic_second_memory_phase (addr2TransactionMap_t::iterator mi) {
  m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) 2nd_mem phase started in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), m_currentClockCycle);

  switch (mi->second.get_HMCOpType()) {
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
      mi->second.set_is_write();
      if (!mi->second.is_write())
        m_dbg->fatal(CALL_INFO, -1, "Atomic operation write flag should be write (2nd phase)\n");
      m_dramsim->addTransaction(mi->second.is_write(), mi->second.get_addr());
      m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) write has been issued (2nd phase) in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), m_currentClockCycle);
      //mi->second.set_HMCOpState(WRITE_ISSUED);
      break;
    case (HMC_NONE):
    default:
      m_dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (2nd phase)\n");
      break;
  }

}

/*
void Vault::issueAtomic_compute_phase (addr2TransactionMap_t::iterator mi) {
  m_dbg->output(CALL_INFO, "Vault %d:hmc: Atomic op %p (bank%u) compute phase started in cycle=%lu\n", m_id, (void*)mi->second.get_addr(), mi->second.get_bankNo(), m_currentClockCycle);

  //mi->second.set_HMCOpState(COMPUTE);
  m_addrCompute = mi->second.get_addr();
  m_bankNoCompute = mi->second.get_bankNo();

  switch (mi->second.get_HMCOpType()) {
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
      m_computeDoneCycle = m_currentClockCycle + 3;
      break;
    case (HMC_NONE):
    default:
      m_dbg->fatal(CALL_INFO, -1, "Vault Should not get a non HMC op in issue atomic (compute phase)\n");
      break;
  }

}
*/