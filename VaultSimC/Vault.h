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
 * File         : Vault.h
 * Author       : HPArch
 * Date         : 9/18/2015
 * Description  : Vault (DRAMsim instance wrapper)
 *********************************************************************************************/

#ifndef VAULT_H
#define VAULT_H

#include <string.h>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <sst/core/output.h>
#include <sst/core/params.h>
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
    /** Constructor
    */
    Vault();

    /** Constructor
     *  @param id Vault id
     *  @param dbg VaultSimC() class wrapper sst debuger
     */
    Vault(unsigned id, Output* dbg, bool statistics, string frequency);

    void finish();

    /** Update
     *  Vaultsim handle to update DramSim, it also increases the cycle 
     */
    void Update();

    /** RegisterCallback
    */
    void RegisterCallback(callback_t *rc, callback_t *wc) { read_callback = rc; write_callback = wc; }

    /** AddTransaction
     *  @param transaction
     *  Adds a transaction to transaction queue, with some local initialization for transaction
     */
    bool AddTransaction(transaction_c transaction);

    /** read_complete
     *  DRAMSim calls this function when it is done with a read
     */
    void read_complete(unsigned id, uint64_t addr, uint64_t cycle);

    /** write_complete
     *  DRAMSim calls this function when it is done with a write
     */
    void write_complete(unsigned id, uint64_t addr, uint64_t cycle);

    unsigned getId() { return m_id; }

  private:
    /** Update_queue
     *  update transaction queue and issue read/write to DRAM
     */
    void Update_queue();

    void issueAtomic_first_memory_phase(addr2TransactionMap_t::iterator mi);
    void issueAtomic_second_memory_phase (addr2TransactionMap_t::iterator mi);
    //void issueAtomic_compute_phase (addr2TransactionMap_t::iterator mi);

    void print_stats();
    void init_stats();

    /** m_bankBusyMap Functions
     */
    inline bool get_bank_state (unsigned bankid) { return m_bankBusyMap[bankid]; }
    inline void unlock_bank (unsigned bankid) { m_bankBusyMap[bankid] = false; }
    inline void lock_bank (unsigned bankid) { m_bankBusyMap[bankid] = true; }
    inline void unlock_all_banks () {
      for (int i=0; i<BANK_BUSY_MAP_OPTIMUM_SIZE; i++)
        m_bankBusyMap[i] = false;
    }

    /** m_statistics functions
     */
    inline bool is_stat_set() { return m_statistics; } 

    /** m_computePhase Functions 
     */
    //inline void set_computePhase() { m_computePhase = true; }
    //inline void reset_computePhase() { m_computePhase = false; }
    //inline bool get_computePhase() { return m_computePhase; }

    /** m_computeDoneCycle Functions    
     */
    //inline void set_compute_done_cycle( uint64_t cycle ) { m_computeDoneCycle = cycle; }
    //inline uint64_t get_compute_done_cycle () { return m_computeDoneCycle; }

  public:
    unsigned m_id;
    uint64_t m_currentClockCycle;

    callback_t *read_callback;
    callback_t *write_callback;

    DRAMSim::MultiChannelMemorySystem* m_dramsim;

  private:
    Output* m_dbg;                                 // VaulSimC wrapper dbg, for printing debuging commands
    bool m_statistics;                             // Print statistics knob
    string m_frequency;                            // Vault Frequency

    addr2TransactionMap_t m_onFlyHmcOps;           // Currently issued atomic ops
    bank2busyMap_t m_bankBusyMap;                  // Current Busy Banks
    transQ_t m_transQ;                             // Transaction Queue

    struct stat_type {
        uint64_t total_trans;
        uint64_t total_hmc_ops;
        uint64_t total_non_hmc_ops;


        uint64_t total_hmc_latency;
        uint64_t issue_hmc_latency;
        uint64_t read_hmc_latency;
        uint64_t write_hmc_latency;
    } stats;

    //bool m_computePhase;
    //uint64_t m_computeDoneCycle;
    //unsigned int m_bankNoCompute;
    //uint64_t m_addrCompute;

};
#endif
