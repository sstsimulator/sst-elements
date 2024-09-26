// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_STORE
#define _H_VANADIS_STORE

#include "inst/vinst.h"
#include "inst/vmemflagtype.h"

namespace SST {
namespace Vanadis {

enum VanadisStoreRegisterType { STORE_INT_REGISTER, STORE_FP_REGISTER };

class VanadisStoreInstruction : public virtual VanadisInstruction
{

public:
    VanadisStoreInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memoryAddr,
        const int64_t offst, const uint16_t valueReg, const uint16_t store_bytes, VanadisMemoryTransaction accessT,
        VanadisStoreRegisterType regT) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 
            /*
            const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out, 
            const uint16_t c_isa_int_reg_in, const uint16_t c_isa_int_reg_out,
            const uint16_t c_phys_fp_reg_in, const uint16_t c_phys_fp_reg_out,
            const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out
            */
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0),
        store_width(store_bytes),
        offset(offst),
        memAccessType(accessT),
        regType(regT)
    {

        switch ( regT ) {
        case STORE_INT_REGISTER:
        {
            isa_int_regs_in[0] = memoryAddr;
            isa_int_regs_in[1] = valueReg;

            if ( MEM_TRANSACTION_LLSC_STORE == accessT ) { isa_int_regs_out[0] = valueReg; }

        } break;
        case STORE_FP_REGISTER:
        {
            isa_int_regs_in[0] = memoryAddr;
            isa_fp_regs_in[0]  = valueReg;

            if ( MEM_TRANSACTION_LLSC_STORE == accessT ) { isa_fp_regs_out[0] = valueReg; }
        } break;
        }
        
    }

    VanadisStoreInstruction* clone() { return new VanadisStoreInstruction(*this); }

    virtual bool isPartialStore() { return false; }

    VanadisMemoryTransaction getTransactionType() const  { return memAccessType; }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_STORE; }

    virtual const char* getInstCode() const override
    {
        switch ( memAccessType ) {
        case MEM_TRANSACTION_LLSC_STORE:
            return "LLSC_STORE";
        case MEM_TRANSACTION_LOCK:
            return "LOCK_STORE";
        case MEM_TRANSACTION_NONE:
        default:
        {
            switch ( regType ) {
            case STORE_INT_REGISTER:
                return "STORE";
                break;
            case STORE_FP_REGISTER:
                return "STOREFP";
                break;
            }
        }
        }
        
        return "STOREUNK";
    }

    virtual void printToBuffer(char* buffer, size_t buffer_size) override
    {
        switch ( regType ) {
        case STORE_INT_REGISTER:
        {
            snprintf(
                buffer, buffer_size,
                "STORE (%s, %" PRIu16 " bytes)   %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "] (phys: %5" PRIu16
                " -> memory[%5" PRIu16 " + %" PRId64 "])",
                getTransactionTypeString(memAccessType), store_width, isa_int_regs_in[1], isa_int_regs_in[0], offset,
                phys_int_regs_in[1], phys_int_regs_in[0], offset);
        } break;
        case STORE_FP_REGISTER:
        {
            snprintf(
                buffer, buffer_size,
                "STOREFP (%s, %" PRIu16 " bytes))   %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "] (phys: %5" PRIu16
                " -> memory[%5" PRIu16 " + %" PRId64 "])",
                getTransactionTypeString(memAccessType), store_width, isa_fp_regs_in[0], isa_int_regs_in[0], offset,
                phys_fp_regs_in[0], phys_int_regs_in[0], offset);
        } break;
        }
    }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        if ( memAccessType != MEM_TRANSACTION_LLSC_STORE ) { markExecuted(); }
    }

    virtual void computeStoreAddress(SST::Output* output, VanadisRegisterFile* reg, uint64_t* store_addr, uint16_t* op_width)
    {
        int64_t reg_tmp;
        uint16_t target_tid = 0;

       
        reg_tmp = reg->getIntReg<int64_t>(phys_int_regs_in[0]);

        (*store_addr) = (uint64_t)(reg_tmp + offset);
        (*op_width)   = store_width;

        switch ( regType ) {
        case STORE_INT_REGISTER:
        {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%" PRI_ADDR ") STORE addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64
                " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%" PRI_ADDR ")\n",
                getInstructionAddress(), phys_int_regs_in[0], phys_int_regs_in[1], offset, store_width, (*store_addr),
                (*store_addr));
        } break;
        case STORE_FP_REGISTER:
        {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%" PRI_ADDR ") STOREFP addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64
                " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%" PRI_ADDR ")\n",
                getInstructionAddress(), phys_int_regs_in[0], phys_fp_regs_in[0], offset, store_width, (*store_addr),
                (*store_addr));
        } break;
        }        
    }

    uint16_t getStoreWidth() const { return store_width; }

    virtual uint16_t getRegisterOffset() const { return 0; }

    uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
    uint16_t getValueRegister() const
    {
        switch ( regType ) {
        case STORE_INT_REGISTER:
            return phys_int_regs_in[1];
        case STORE_FP_REGISTER:
            return phys_fp_regs_in[0];
        }
        assert(0); // stop compiler "warning: control reaches end of non-void function [-Wreturn-type]"
    }

    virtual void markExecuted() 
    { 
        // printf("Store ins Scalar:(addr=0x%" PRI_ADDR ") markExecuted()\n", getInstructionAddress());
        hasExecuted = true;
    }

    
    VanadisStoreRegisterType getValueRegisterType() const { return regType; }

protected:
    const int64_t            offset;
    const uint16_t           store_width;
    VanadisMemoryTransaction memAccessType;
    VanadisStoreRegisterType regType;
};

class VanadisSIMTStoreInstruction : public VanadisSIMTInstruction, public virtual VanadisStoreInstruction
{
public:
    VanadisSIMTStoreInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memoryAddr,
        const int64_t offst, const uint16_t valueReg, const uint16_t store_bytes, VanadisMemoryTransaction accessT,
        VanadisStoreRegisterType regT) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisSIMTInstruction(
            addr, hw_thr, isa_opts, 
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_INT_REGISTER ? 2 : 1, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0,
            regT == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisStoreInstruction(addr, hw_thr, isa_opts, memoryAddr, offst, valueReg, store_bytes, accessT, regT)
        {
            numStores_counter = 0;
            numStores_pushed = 0;
        }
    
    VanadisSIMTStoreInstruction* clone() { return new VanadisSIMTStoreInstruction(*this); }

    void markExecuted() override
    {
        // printf("Store ins SIMT:(addr=0x%" PRI_ADDR ") markExecuted() SIMT numStores_counter:%d\n", getInstructionAddress(), numStores_counter);
        numStores_counter-=1;
        if(numStores_counter<=0) 
        {
            hasExecuted = true; 
            numStores_counter=0;
            // printf("Store ins:(addr=0x%" PRI_ADDR ") markExecuted() success\n", getInstructionAddress());
        }
        // else
            // printf("Store ins:(addr=0x%" PRI_ADDR ") markExecuted() failed SIMT numStores_counter:%d\n", getInstructionAddress(), numStores_counter);
    }
    
    virtual void computeStoreAddress(SST::Output* output, VanadisRegisterFile* reg, uint64_t* store_addr, uint16_t* op_width) override
    {
        int64_t reg_tmp;
        uint16_t target_tid = 0;

        
        target_tid = reg->getTID();
        // printf("Store SIMT target_tid = %d\n", target_tid); 
        fflush(stdout);

        reg_tmp = reg->getIntReg<int64_t>(phys_int_regs_in_simt[target_tid][0]);

        (*store_addr) = (uint64_t)(reg_tmp + offset);
        (*op_width)   = store_width;

        switch ( regType ) {
        case STORE_INT_REGISTER:
        {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%" PRI_ADDR ") STORE addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64
                " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%" PRI_ADDR ")\n",
                getInstructionAddress(), phys_int_regs_in_simt[target_tid][0], phys_int_regs_in_simt[target_tid][1], offset, store_width, (*store_addr),
                (*store_addr));
        } break;
        case STORE_FP_REGISTER:
        {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: (addr=0x%" PRI_ADDR ") STOREFP addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64
                " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%" PRI_ADDR ")\n",
                getInstructionAddress(), phys_int_regs_in_simt[target_tid][0], phys_fp_regs_in_simt[target_tid][0], offset, store_width, (*store_addr),
                (*store_addr));
        } break;
        }
    }


    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        // printf("SIMTStore execute(regfile)\n");
        if ( memAccessType != MEM_TRANSACTION_LLSC_STORE ) { markExecuted(); }
    }

    uint16_t getNumStores() { return numStores_counter;}
    
    void incrementNumStores() 
    { 
        numStores_counter+=1;
        numStores_pushed+=1;
    }
    
    void setNumStores(uint16_t num_stores)
    { 
        numStores_counter = num_stores;
        numStores_pushed = num_stores;
    }
    
    uint16_t getTotalStoresPushed() { return numStores_pushed;}

protected:
    uint16_t numStores_counter;
    uint16_t numStores_pushed;
};
} // namespace Vanadis
} // namespace SST

#endif
