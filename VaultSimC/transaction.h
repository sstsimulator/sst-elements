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

typedef enum HMC_Type_enum {
    HMC_NONE=0,
    HMC_CAS_equal_16B=1,
    HMC_CAS_zero_16B=2,
    HMC_CAS_greater_16B=3,
    HMC_CAS_less_16B=4,
    HMC_ADD_16B=5,
    HMC_ADD_8B=6,
    HMC_ADD_DUAL=7,
    HMC_SWAP=8,
    HMC_BIT_WR=9,
    HMC_AND=10,
    HMC_NAND=11,
    HMC_OR=12,
    HMC_XOR=13,
    HMC_FP_ADD=14,
    HMC_COMP_greater=15,
    HMC_COMP_less=16,
    HMC_COMP_equal=17,
    HMC_hook=18,                // Not used in VaultSim
    HMC_unhook=19,              // Not used in VaultSim
    HMC_CANDIDATE=20,           // Not a HMC-op, it is showing if an instruction could be HMC in other scenarios
    HMC_TRANS_BEG=21,           // Transaction support
    HMC_TRANS_MID=22,           // Transaction support
    HMC_TRANS_END=23,           // Transaction support
    NUM_HMC_TYPES=24
} HMC_Type;

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
    /** 
     * Constructor
     * @param isWrite is it a write?
     * @param addr address for memory operation
     */
    transaction_c() : isWrite(false), addr(0), isAtomic(false), hmcType(HMC_NONE), transactionId(0), hmcOpState(NO_STATE), flagPrintDbgHMC(0) {}

    transaction_c(bool _isWrite, uint64_t _addr) : 
        isWrite(_isWrite), addr(_addr), isAtomic(false), hmcType(HMC_NONE), transactionId(0), hmcOpState(NO_STATE), flagPrintDbgHMC(0) {}

    /** 
     * addr Member Fuctions
     */
    uint64_t getAddr() { return addr; }
    void setAddr( uint64_t addr_) { addr = addr_; }

    /** 
     * isWrite Member Fuctions     
     */
    bool getIsWrite() { return isWrite; }
    void setIsWrite() { isWrite = true; }
    void resetIsWrite() { isWrite = false; }

    /** 
     * isAtomic Member Fuctions
     */
    bool getAtomic() { return isAtomic; }
    void setAtomic() { isAtomic = true; }
    void resetAtomic() { isAtomic = false; }

    /**
     * transactionId Member Functions
     */
    void setTransId(uint64_t transactionId_) { transactionId = transactionId_; }
    uint64_t getTransId() { return transactionId; }

    /** 
     * HMC_Type functions
     */
    void setHmcOpType(uint8_t instType) { hmcType = instType; }
    uint8_t getHmcOpType() { return hmcType; }
    const char* getHmcOpTypeStr() {
        switch (hmcType) {
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
        case HMC_FP_ADD:
            return "HMC_FP_ADD";
        case HMC_COMP_greater:
            return "HMC_COMP_greater";
        case HMC_COMP_less:
            return "HMC_COMP_less";
        case HMC_COMP_equal:
            return "HMC_COMP_equal";
        case HMC_hook:
            return "HMC_hook";
        case HMC_unhook:
            return "HMC_unhook";
        case HMC_CANDIDATE:
            return "HMC_CANDIDATE";
        case HMC_TRANS_BEG:
            return "HMC_TRANS_BEG";
        case HMC_TRANS_MID: 
            return "HMC_TRANS_MID";
        case HMC_TRANS_END:
            return "HMC_TRANS_END";
        default:
            return "THIS MUST NOT BE PRINTED";
        }
    }

    /**
     * HMC_Op_State functions
     */
    void setHmcOpState(HMC_Op_State instState) { hmcOpState = instState; }

    HMC_Op_State getHmcOpState() { return hmcOpState; }

    const char* getHmcOpStateStr() {
        switch (hmcOpState) {
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

    /**
     * BankNo functions
     */ 
    void setBankNo (unsigned _bankNo) { bankNo = _bankNo; }
    unsigned getBankNo () { return bankNo; } 

    /**
     * Stats
     */
    void setFlagPrintDbgHMC() { flagPrintDbgHMC = true; }
    void resetFlagPrintDbgHMC() { flagPrintDbgHMC = false; }
    bool getFlagPrintDbgHMC() { return flagPrintDbgHMC; }

private:
    //transaction properties
    bool isWrite;
    uint64_t addr;
    unsigned int bankNo;
    bool isAtomic;
    uint8_t hmcType;              //HMC_Type Enum

    //Transaction Support
    uint64_t transactionId;

    //stats
    HMC_Op_State hmcOpState;
    bool flagPrintDbgHMC;

public:
    //stats
    uint64_t inCycle;
    uint64_t issueCycle;
    uint64_t readDoneCycle;
    uint64_t writeDoneCycle;
    
};

#endif
