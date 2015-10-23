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
 * File         : transaction.h
 * Author       : HPArch
 * Date         : 9/18/2015
 * Description  : transaction class
 *********************************************************************************************/

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "../macsimComponent/src/hmc_types.h"

///////////////////////////////////////////////////////////////////////////////////////////////
/// \HMC Op State Enum
///////////////////////////////////////////////////////////////////////////////////////////////

typedef enum HMC_Op_State_enum {
  NO_STATE,
  QUEUED,
  READ_ISSUED,
  READ_ANS_RECV,
  COMPUTE,
  WRITE_ISSUED,
  WRITE_ANS_RECV,
  NUM_HMC_OP_STATES
} HMC_Op_State;

///////////////////////////////////////////////////////////////////////////////////////////////
/// \Transaction class
/// \Used in VaultSimC and Vault to pass MemEvents variables
///////////////////////////////////////////////////////////////////////////////////////////////

class transaction_c {
  public:
    /** Constructor
     *  @param is_write is it a write?
     *  @param addr address for memory operation
     */
    transaction_c() : m_is_write(0), m_addr(0), 
        m_atomic(false), m_hmc_type(HMC_NONE), m_hmc_op_state(NO_STATE) {}

    transaction_c(bool is_write, uint64_t addr) : 
      m_is_write(is_write), m_addr(addr), 
      m_atomic(false), m_hmc_type(HMC_NONE), m_hmc_op_state(NO_STATE) {}

    /** addr functions 
     */
    uint64_t get_addr() { return m_addr; }

    /** is_write functions      
     */
    bool is_write() { return m_is_write; }
    void set_is_write() { m_is_write = true; }
    void reset_is_write() { m_is_write = false; }

    /** m_atomic Member Fuctions
     */
    bool get_atomic() { return m_atomic; }
    void set_atomic() { m_atomic = true; }
    void reset_atomic() { m_atomic = false; }

    /** HMC_Type functions
     */
    void set_HMCOpType(uint8_t instType) { m_hmc_type = instType; }

    uint8_t get_HMCOpType() { return m_hmc_type; }

    const char* get_HMCOpType_str() {
      switch (m_hmc_type) {
        case HMC_NONE:  
          return "HMC_NONE";
        case HMC_CAS_equal_16B:
          return "HMC_CAS_equal_16B";
        case HMC_CAS_zero_16B:
          return "HMC_CAS_zero_16B";
        case HMC_CAS_greater_16B:
          return "HMC_CAS_greater_16B";
        case HMC_CAS_less_16B:
          return "HMC_CAS_less_16B";
        case HMC_ADD_16B:
          return "HMC_ADD_16B";
        case HMC_ADD_8B:
          return "HMC_ADD_8B";
        case HMC_ADD_DUAL:
          return "HMC_ADD_DUAL";
        case HMC_SWAP:
          return "HMC_SWAP";
        case HMC_BIT_WR:
          return "HMC_BIT_WR";
        case HMC_AND:
          return "HMC_AND";
        case HMC_NAND:
          return "HMC_NAND";
        case HMC_OR:
          return "HMC_OR";
        case HMC_XOR:
          return "HMC_XOR";
      }
    }


    /** HMC_Op_State functions
     */
     /*
    void set_HMCOpState(HMC_Op_State instState) { m_hmc_op_state=instState; }

    HMC_Op_State get_HMCOpState() { return m_hmc_op_state; }

    const char* get_HMCOpState_str() {
      switch (m_hmc_op_state) {
        case NO_STATE:
          return "NO_STATE";
        case QUEUED:
          return "QUEUED";
        case READ_ISSUED:  
          return "READ_ISSUED";
        case READ_ANS_RECV:
          return "READ_ANS_RECV";
        case COMPUTE:
          return "COMPUTE";
        case WRITE_ISSUED:
          return "WRITE_ISSUED";
        case WRITE_ANS_RECV:
          return "WRITE_ANS_RECV";
      }
    }
    */

    /** BankNo functions
     */ 
    void set_bankNo (unsigned bankNo) { m_bankNo = bankNo; }
    unsigned get_bankNo () { return m_bankNo; } 

  private:
    bool m_is_write;
    uint64_t m_addr;
    unsigned int m_bankNo;

    bool m_atomic;
    uint8_t m_hmc_type;              //HMC_Type Enum
    HMC_Op_State m_hmc_op_state;

  public:
    /*Stat Gathering*/
    uint64_t m_in_cycle;
    uint64_t m_issue_cycle;
    uint64_t m_read_done_cycle;       // Same as write issue cycle (without computing)
    uint64_t m_write_done_cycle;      // Same as done cycle 

};

#endif
