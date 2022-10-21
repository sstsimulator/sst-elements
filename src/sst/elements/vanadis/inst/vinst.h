// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_INSTRUCTION
#define _H_VANADIS_INSTRUCTION

#include "decoder/visaopts.h"
#include "inst/regfile.h"
#include "inst/vinsttype.h"
#include "inst/vregfmt.h"

#include <cstring>
#include <sst/core/output.h>

namespace SST {
namespace Vanadis {

class VanadisInstruction
{
public:
    VanadisInstruction(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out, const uint16_t c_isa_int_reg_in,
        const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in, const uint16_t c_phys_fp_reg_out,
        const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out) :
        ins_address(address),
        hw_thread(hw_thr),
        isa_options(isa_opts),
        count_phys_int_reg_in(c_phys_int_reg_in),
        count_phys_int_reg_out(c_phys_int_reg_out),
        count_isa_int_reg_in(c_isa_int_reg_in),
        count_isa_int_reg_out(c_isa_int_reg_out),
        count_phys_fp_reg_in(c_phys_fp_reg_in),
        count_phys_fp_reg_out(c_phys_fp_reg_out),
        count_isa_fp_reg_in(c_isa_fp_reg_in),
        count_isa_fp_reg_out(c_isa_fp_reg_out)
    {

        phys_int_regs_in = (count_phys_int_reg_in > 0) ? new uint16_t[count_phys_int_reg_in] : nullptr;
        zero_array(phys_int_regs_in, count_phys_int_reg_in);

        phys_int_regs_out = (count_phys_int_reg_out > 0) ? new uint16_t[count_phys_int_reg_out] : nullptr;
        zero_array(phys_int_regs_out, count_phys_int_reg_out);
  
        isa_int_regs_in = (count_isa_int_reg_in > 0) ? new uint16_t[count_isa_int_reg_in] : nullptr;
        zero_array(isa_int_regs_in, count_isa_int_reg_in);
 
        isa_int_regs_out = (count_isa_int_reg_out > 0) ? new uint16_t[count_isa_int_reg_out] : nullptr;
        zero_array(isa_int_regs_out, count_isa_int_reg_out);

        phys_fp_regs_in = (count_phys_fp_reg_in > 0) ? new uint16_t[count_phys_fp_reg_in] : nullptr;
        zero_array(phys_fp_regs_in, count_phys_fp_reg_in);

        phys_fp_regs_out = (count_phys_fp_reg_out > 0) ? new uint16_t[count_phys_fp_reg_out] : nullptr;
        zero_array(phys_fp_regs_out, count_phys_fp_reg_out);

        isa_fp_regs_in = (count_isa_fp_reg_in > 0) ? new uint16_t[count_isa_fp_reg_in] : nullptr;
        zero_array(isa_fp_regs_in, count_isa_fp_reg_in);

        isa_fp_regs_out = (count_isa_fp_reg_out > 0) ? new uint16_t[count_isa_fp_reg_out] : nullptr;
        zero_array(isa_fp_regs_out, count_isa_fp_reg_out);
  
        trapError             = false;
        hasExecuted           = false;
        hasIssued             = false;
        hasRegistersAllocated = false;
        enduOpGroup           = false;
        isFrontOfROB          = false;
        hasROBSlot            = false;
    }

    virtual ~VanadisInstruction()
    {
        if ( phys_int_regs_in != nullptr ) delete[] phys_int_regs_in;
        if ( phys_int_regs_out != nullptr ) delete[] phys_int_regs_out;
        if ( isa_int_regs_in != nullptr ) delete[] isa_int_regs_in;
        if ( isa_int_regs_out != nullptr ) delete[] isa_int_regs_out;
        if ( phys_fp_regs_in != nullptr ) delete[] phys_fp_regs_in;
        if ( phys_fp_regs_out != nullptr ) delete[] phys_fp_regs_out;
        if ( isa_fp_regs_in != nullptr ) delete[] isa_fp_regs_in;
        if ( isa_fp_regs_out != nullptr ) delete[] isa_fp_regs_out;
    }

    VanadisInstruction(const VanadisInstruction& copy_me) :
        ins_address(copy_me.ins_address),
        hw_thread(copy_me.hw_thread),
        isa_options(copy_me.isa_options),
        count_phys_int_reg_in(copy_me.count_phys_int_reg_in),
        count_phys_int_reg_out(copy_me.count_phys_int_reg_out),
        count_isa_int_reg_in(copy_me.count_isa_int_reg_in),
        count_isa_int_reg_out(copy_me.count_isa_int_reg_out),
        count_phys_fp_reg_in(copy_me.count_phys_fp_reg_in),
        count_phys_fp_reg_out(copy_me.count_phys_fp_reg_out),
        count_isa_fp_reg_in(copy_me.count_isa_fp_reg_in),
        count_isa_fp_reg_out(copy_me.count_isa_fp_reg_out)
    {

        trapError             = copy_me.trapError;
        hasExecuted           = copy_me.hasExecuted;
        hasIssued             = copy_me.hasIssued;
        hasRegistersAllocated = copy_me.hasRegistersAllocated;
        enduOpGroup           = copy_me.enduOpGroup;
        isFrontOfROB          = false;
        hasROBSlot            = false;

        phys_int_regs_in  = (count_phys_int_reg_in > 0) ? new uint16_t[count_phys_int_reg_in] : nullptr;
        phys_int_regs_out = (count_phys_int_reg_out > 0) ? new uint16_t[count_phys_int_reg_out] : nullptr;

        isa_int_regs_in  = (count_isa_int_reg_in > 0) ? new uint16_t[count_isa_int_reg_in] : nullptr;
        isa_int_regs_out = (count_isa_int_reg_out > 0) ? new uint16_t[count_isa_int_reg_out] : nullptr;

        phys_fp_regs_in  = (count_phys_fp_reg_in > 0) ? new uint16_t[count_phys_fp_reg_in] : nullptr;
        phys_fp_regs_out = (count_phys_fp_reg_out > 0) ? new uint16_t[count_phys_fp_reg_out] : nullptr;

        isa_fp_regs_in  = (count_isa_fp_reg_in > 0) ? new uint16_t[count_isa_fp_reg_in] : nullptr;
        isa_fp_regs_out = (count_isa_fp_reg_out > 0) ? new uint16_t[count_isa_fp_reg_out] : nullptr;

        for ( uint16_t i = 0; i < count_phys_int_reg_in; ++i ) {
            //assert(phys_int_regs_in != nullptr);
            phys_int_regs_in[i] = copy_me.phys_int_regs_in[i];
        }

        for ( uint16_t i = 0; i < count_phys_int_reg_out; ++i ) {
            //assert(phys_int_regs_out != nullptr);
            phys_int_regs_out[i] = copy_me.phys_int_regs_out[i];
        }

        for ( uint16_t i = 0; i < count_isa_int_reg_in; ++i ) {
            //assert(isa_int_regs_in != nullptr);
            isa_int_regs_in[i] = copy_me.isa_int_regs_in[i];
        }

        for ( uint16_t i = 0; i < count_isa_int_reg_out; ++i ) {
            //assert(isa_int_regs_out != nullptr);
            isa_int_regs_out[i] = copy_me.isa_int_regs_out[i];
        }

        for ( uint16_t i = 0; i < count_phys_fp_reg_in; ++i ) {
            phys_fp_regs_in[i] = copy_me.phys_fp_regs_in[i];
        }

        for ( uint16_t i = 0; i < count_phys_fp_reg_out; ++i ) {
            phys_fp_regs_out[i] = copy_me.phys_fp_regs_out[i];
        }

        for ( uint16_t i = 0; i < count_isa_fp_reg_in; ++i ) {
            isa_fp_regs_in[i] = copy_me.isa_fp_regs_in[i];
        }

        for ( uint16_t i = 0; i < count_isa_fp_reg_out; ++i ) {
            isa_fp_regs_out[i] = copy_me.isa_fp_regs_out[i];
        }
    }

    void writeIntRegs(char* buffer, size_t max_buff_size)
    {
        size_t index_so_far = 0;

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "in: { ");

        if ( count_isa_int_reg_in > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", isa_int_regs_in[0]);

            for ( int i = 1; i < count_isa_int_reg_in; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", isa_int_regs_in[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } -> { ");

        if ( count_phys_int_reg_in > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", phys_int_regs_in[0]);

            for ( int i = 1; i < count_phys_int_reg_in; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", phys_int_regs_in[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } / out: { ");

        if ( count_isa_int_reg_out > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", isa_int_regs_out[0]);

            for ( int i = 1; i < count_isa_int_reg_out; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", isa_int_regs_out[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } -> { ");

        if ( count_phys_int_reg_out > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", phys_int_regs_out[0]);

            for ( int i = 1; i < count_phys_int_reg_out; ++i ) {
                index_so_far += snprintf(
                    &buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", phys_int_regs_out[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " }");
    }

    void writeFPRegs(char* buffer, size_t max_buff_size)
    {
        size_t index_so_far = 0;

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "in: { ");

        if ( count_isa_fp_reg_in > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", isa_fp_regs_in[0]);

            for ( int i = 1; i < count_isa_fp_reg_in; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", isa_fp_regs_in[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } -> { ");

        if ( count_phys_fp_reg_in > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", phys_fp_regs_in[0]);

            for ( int i = 1; i < count_phys_fp_reg_in; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", phys_fp_regs_in[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } / out: { ");

        if ( count_isa_fp_reg_out > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", isa_fp_regs_out[0]);

            for ( int i = 1; i < count_isa_fp_reg_out; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", isa_fp_regs_out[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " } -> { ");

        if ( count_phys_fp_reg_out > 0 ) {
            index_so_far +=
                snprintf(&buffer[index_so_far], max_buff_size - index_so_far, "%" PRIu16 "", phys_fp_regs_out[0]);

            for ( int i = 1; i < count_phys_fp_reg_out; ++i ) {
                index_so_far +=
                    snprintf(&buffer[index_so_far], max_buff_size - index_so_far, ", %" PRIu16 "", phys_fp_regs_out[i]);
            }
        }

        index_so_far += snprintf(&buffer[index_so_far], max_buff_size - index_so_far, " }");
    }

    uint16_t countPhysIntRegIn() const { return count_phys_int_reg_in; }
    uint16_t countPhysIntRegOut() const { return count_phys_int_reg_out; }
    uint16_t countISAIntRegIn() const { return count_isa_int_reg_in; }
    uint16_t countISAIntRegOut() const { return count_isa_int_reg_out; }

    uint16_t countPhysFPRegIn() const { return count_phys_fp_reg_in; }
    uint16_t countPhysFPRegOut() const { return count_phys_fp_reg_out; }
    uint16_t countISAFPRegIn() const { return count_isa_fp_reg_in; }
    uint16_t countISAFPRegOut() const { return count_isa_fp_reg_out; }

    uint16_t* getPhysIntRegIn() { return phys_int_regs_in; }
    uint16_t* getPhysIntRegOut() { return phys_int_regs_out; }
    uint16_t* getISAIntRegIn() { return isa_int_regs_in; }
    uint16_t* getISAIntRegOut() { return isa_int_regs_out; }

    uint16_t* getPhysFPRegIn() { return phys_fp_regs_in; }
    uint16_t* getPhysFPRegOut() { return phys_fp_regs_out; }
    uint16_t* getISAFPRegIn() { return isa_fp_regs_in; }
    uint16_t* getISAFPRegOut() { return isa_fp_regs_out; }

    uint16_t getPhysIntRegIn(const uint16_t index) const { return phys_int_regs_in[index]; }
    uint16_t getPhysIntRegOut(const uint16_t index) const { return phys_int_regs_out[index]; }
    uint16_t getISAIntRegIn(const uint16_t index) const { return isa_int_regs_in[index]; }
    uint16_t getISAIntRegOut(const uint16_t index) const { return isa_int_regs_out[index]; }

    uint16_t getPhysFPRegIn(const uint16_t index) const { return phys_fp_regs_in[index]; }
    uint16_t getPhysFPRegOut(const uint16_t index) const { return phys_fp_regs_out[index]; }
    uint16_t getISAFPRegIn(const uint16_t index) const { return isa_fp_regs_in[index]; }
    uint16_t getISAFPRegOut(const uint16_t index) const { return isa_fp_regs_out[index]; }

    void setPhysIntRegIn(const uint16_t index, const uint16_t reg) { phys_int_regs_in[index] = reg; }
    void setPhysIntRegOut(const uint16_t index, const uint16_t reg) { 
        phys_int_regs_out[index] = reg;

        if(getInstFuncType() != INST_SYSCALL) {
        for(auto i = 0; i < count_phys_int_reg_in; ++i) {
            assert(phys_int_regs_in[i] != reg);
        }
        }
    }
    void setPhysFPRegIn(const uint16_t index, const uint16_t reg) { phys_fp_regs_in[index] = reg; }
    void setPhysFPRegOut(const uint16_t index, const uint16_t reg) { 
        phys_fp_regs_out[index] = reg;

        if(getInstFuncType() != INST_SYSCALL) {
        for(auto i = 0; i < count_phys_fp_reg_in; ++i) {
            assert(phys_fp_regs_in[i] != reg);
        } 
        }
    }

    virtual VanadisInstruction* clone() = 0;

    void markEndOfMicroOpGroup() { enduOpGroup = true; }
    bool endsMicroOpGroup() const { return enduOpGroup; }
    bool trapsError() const { return trapError; }

    uint64_t getInstructionAddress() const { return ins_address; }
    uint32_t getHWThread() const { return hw_thread; }

    virtual const char* getInstCode() const = 0;
    virtual void printToBuffer(char* buffer, size_t buffer_size) { snprintf(buffer, buffer_size, "%s", getInstCode()); }
    virtual VanadisFunctionalUnitType getInstFuncType() const                                    = 0;
    virtual void                      execute(SST::Output* output, VanadisRegisterFile* regFile) = 0;
    virtual void print(SST::Output* output) { output->verbose(CALL_INFO, 8, 0, "%s", getInstCode()); }

    // Is the instruction predicted (speculation point).
    // for normal instructions this is false
    // but branches and jumps will get predicted
    virtual bool isSpeculated() const { return false; }

    bool completedExecution() const { return hasExecuted; }
    bool completedIssue() const { return hasIssued; }
    bool completedRegisterAllocation() const { return hasRegistersAllocated; }

    void markExecuted() { hasExecuted = true; }
    void markIssued() { hasIssued = true; }
    void markRegistersAllocated() { hasRegistersAllocated = true; }

    bool checkFrontOfROB() { return isFrontOfROB; }
    void markFrontOfROB() { isFrontOfROB = true; }

    bool hasROBSlotIssued() const { return hasROBSlot; }
    void markROBSlotIssued() { hasROBSlot = true; }

    const VanadisDecoderOptions* getISAOptions() const { return isa_options; }

    void flagError() { trapError = true; }

    virtual bool performIntRegisterRecovery() const { return true; }

    virtual bool performFPRegisterRecovery() const { return true; }

	virtual bool updatesFPFlags() const { return false; }
    virtual void updateFPFlags() {}

protected:
    void zero_array(uint16_t* array, size_t len) const {
        for(auto i = 0; i < len; ++i) {
            array[i] = 0;
        }
    }

    const uint64_t ins_address;
    const uint32_t hw_thread;

    uint16_t count_phys_int_reg_in;
    uint16_t count_phys_int_reg_out;
    uint16_t count_isa_int_reg_in;
    uint16_t count_isa_int_reg_out;

    uint16_t count_phys_fp_reg_in;
    uint16_t count_phys_fp_reg_out;
    uint16_t count_isa_fp_reg_in;
    uint16_t count_isa_fp_reg_out;

    uint16_t* phys_int_regs_in;
    uint16_t* phys_int_regs_out;
    uint16_t* isa_int_regs_in;
    uint16_t* isa_int_regs_out;

    uint16_t* phys_fp_regs_in;
    uint16_t* phys_fp_regs_out;
    uint16_t* isa_fp_regs_in;
    uint16_t* isa_fp_regs_out;

    bool trapError;
    bool hasExecuted;
    bool hasIssued;
    bool hasRegistersAllocated;
    bool enduOpGroup;
    bool isFrontOfROB;
    bool hasROBSlot;

    const VanadisDecoderOptions* isa_options;
};

} // namespace Vanadis
} // namespace SST

#endif
