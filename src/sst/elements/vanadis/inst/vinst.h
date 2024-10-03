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

#ifndef _H_VANADIS_INSTRUCTION
#define _H_VANADIS_INSTRUCTION

#include "decoder/visaopts.h"
#include "inst/regfile.h"
#include "inst/regstack.h"
#include "inst/vinsttype.h"
#include "inst/vregfmt.h"

#include <cstring>
#include <map>
#include <sst/core/output.h>
#include <string>
#include <sstream>



namespace SST {
namespace Vanadis {
static std::vector<VanadisRegisterFile*> core_regFiles;
class VanadisInstruction
{
    public:
        VanadisInstruction(const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
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
            std::memset(phys_int_regs_in, 0, count_phys_int_reg_in * sizeof( uint16_t ));

            phys_int_regs_out = (count_phys_int_reg_out > 0) ? new uint16_t[count_phys_int_reg_out] : nullptr;
            std::memset(phys_int_regs_out, 0, count_phys_int_reg_out * sizeof( uint16_t ) );
    
            isa_int_regs_in = (count_isa_int_reg_in > 0) ? new uint16_t[count_isa_int_reg_in] : nullptr;
            std::memset(isa_int_regs_in, 0, count_isa_int_reg_in * sizeof( uint16_t ));
    
            isa_int_regs_out = (count_isa_int_reg_out > 0) ? new uint16_t[count_isa_int_reg_out] : nullptr;
            std::memset(isa_int_regs_out, 0, count_isa_int_reg_out * sizeof( uint16_t ) );

            phys_fp_regs_in = (count_phys_fp_reg_in > 0) ? new uint16_t[count_phys_fp_reg_in] : nullptr;
            std::memset(phys_fp_regs_in, 0, count_phys_fp_reg_in * sizeof( uint16_t ));

            phys_fp_regs_out = (count_phys_fp_reg_out > 0) ? new uint16_t[count_phys_fp_reg_out] : nullptr;
            std::memset(phys_fp_regs_out, 0, count_phys_fp_reg_out * sizeof( uint16_t ));

            isa_fp_regs_in = (count_isa_fp_reg_in > 0) ? new uint16_t[count_isa_fp_reg_in] : nullptr;
            std::memset(isa_fp_regs_in, 0, count_isa_fp_reg_in * sizeof( uint16_t ) );

            isa_fp_regs_out = (count_isa_fp_reg_out > 0) ? new uint16_t[count_isa_fp_reg_out] : nullptr;
            std::memset(isa_fp_regs_out, 0, count_isa_fp_reg_out * sizeof( uint16_t ));

            trapError             = false;
            hasExecuted           = false;
            hasIssued             = false;
            enduOpGroup           = false;
            isFrontOfROB          = false;
            hasROBSlot            = false;
            sw_thread = hw_thr;
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
            enduOpGroup           = copy_me.enduOpGroup;
            isFrontOfROB          = false;
            hasROBSlot            = false;
            sw_thread             = copy_me.sw_thread;

            phys_int_regs_in  = (count_phys_int_reg_in > 0) ? new uint16_t[count_phys_int_reg_in] : nullptr;
            phys_int_regs_out = (count_phys_int_reg_out > 0) ? new uint16_t[count_phys_int_reg_out] : nullptr;

            isa_int_regs_in  = (count_isa_int_reg_in > 0) ? new uint16_t[count_isa_int_reg_in] : nullptr;
            isa_int_regs_out = (count_isa_int_reg_out > 0) ? new uint16_t[count_isa_int_reg_out] : nullptr;

            phys_fp_regs_in  = (count_phys_fp_reg_in > 0) ? new uint16_t[count_phys_fp_reg_in] : nullptr;
            phys_fp_regs_out = (count_phys_fp_reg_out > 0) ? new uint16_t[count_phys_fp_reg_out] : nullptr;

            isa_fp_regs_in  = (count_isa_fp_reg_in > 0) ? new uint16_t[count_isa_fp_reg_in] : nullptr;
            isa_fp_regs_out = (count_isa_fp_reg_out > 0) ? new uint16_t[count_isa_fp_reg_out] : nullptr;

            for ( uint16_t i = 0; i < count_phys_int_reg_in; ++i ) {
                phys_int_regs_in[i] = copy_me.phys_int_regs_in[i];
            }

            for ( uint16_t i = 0; i < count_phys_int_reg_out; ++i ) {
                phys_int_regs_out[i] = copy_me.phys_int_regs_out[i];
            }

            for ( uint16_t i = 0; i < count_isa_int_reg_in; ++i ) {
                isa_int_regs_in[i] = copy_me.isa_int_regs_in[i];
            }

            for ( uint16_t i = 0; i < count_isa_int_reg_out; ++i ) {
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

        // different
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

        // different
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
        uint16_t countPhysFPRegIn() const { return count_phys_fp_reg_in; }
        uint16_t countPhysFPRegOut() const { return count_phys_fp_reg_out; }

        
        uint16_t countISAIntRegIn() const { return count_isa_int_reg_in; }
        uint16_t countISAIntRegOut() const { return count_isa_int_reg_out; }
        uint16_t countISAFPRegIn() const { return count_isa_fp_reg_in; }
        uint16_t countISAFPRegOut() const { return count_isa_fp_reg_out; }

        uint16_t getPhysIntRegIn(const uint16_t index) const { return phys_int_regs_in[index]; }
        uint16_t getPhysIntRegOut(const uint16_t index) const { return phys_int_regs_out[index]; }
        uint16_t getISAIntRegIn(const uint16_t index) const { return isa_int_regs_in[index]; }
        uint16_t getISAIntRegOut(const uint16_t index) const { return isa_int_regs_out[index]; }


        uint16_t getPhysFPRegIn(const uint16_t index) const { return phys_fp_regs_in[index]; }
        uint16_t getPhysFPRegOut(const uint16_t index) const { return phys_fp_regs_out[index]; }
        uint16_t getISAFPRegIn(const uint16_t index) const { return isa_fp_regs_in[index]; }
        uint16_t getISAFPRegOut(const uint16_t index) const { return isa_fp_regs_out[index]; }
        
        void setPhysIntRegIn(const uint16_t index, const uint16_t reg) { phys_int_regs_in[index] = reg; }
        void setPhysIntRegOut(const uint16_t index, const uint16_t reg) { phys_int_regs_out[index] = reg; }
        void setPhysFPRegIn(const uint16_t index, const uint16_t reg) { phys_fp_regs_in[index] = reg; }
        void setPhysFPRegOut(const uint16_t index, const uint16_t reg) { phys_fp_regs_out[index] = reg; }

        virtual VanadisInstruction* clone() = 0;

        void markEndOfMicroOpGroup() { enduOpGroup = true; }
        bool endsMicroOpGroup() const { return enduOpGroup; }
        bool trapsError() const { return trapError; }

        uint64_t getInstructionAddress() const { return ins_address; }
        uint32_t getHWThread() const { return hw_thread; }

        void setSWThread(uint32_t thr) { sw_thread=thr;}
        uint32_t getSWThread() {return sw_thread;}

        virtual const char* getInstCode() const = 0 ;
        
        virtual void printToBuffer(char* buffer, size_t buffer_size) { snprintf(buffer, buffer_size, "%s", getInstCode()); }
        
        virtual VanadisFunctionalUnitType getInstFuncType() const = 0;
       

        virtual void instOp(VanadisRegisterFile* regFile, 
                            uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0, 
                            uint16_t phys_int_regs_in_1)
        {
            printf("VanadisInstruction instOp virtual function\n");
        }

        virtual void instOp(VanadisRegisterFile* regFile, 
                                uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0)
        {
             printf("VanadisInstruction instOp virtual function: immediate version\n");
        }
        
        virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile)
        {
            uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
            uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
            uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1);
            log(output, 16, 65535,phys_int_regs_out_0,phys_int_regs_in_0,phys_int_regs_in_1);
            instOp(regFile,phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
            markExecuted();
        }

        virtual void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0)
        {
            #ifdef VANADIS_BUILD_DEBUG
            if(output->getVerboseLevel() >= verboselevel) {

                std::ostringstream ss;
                ss << "hw_thr="<<getHWThread()<<" sw_thr=" <<sw_thr;
                ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
                ss << " phys: out=" <<  phys_int_regs_out_0 << " in=" << phys_int_regs_in_0;
                // ss << " imm=" << imm_value;
                ss << ", isa: out=" <<  isa_int_regs_out[0]  << " in=" << isa_int_regs_in[0];
                output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
            }
            #endif
        }

        virtual void  execute(SST::Output* output, std::vector<VanadisRegisterFile*>& regFiles)
        {
            scalarExecute(output, regFiles[hw_thread]);
        }

        virtual void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,
                                    uint16_t phys_int_regs_in_1)
        {
            #ifdef VANADIS_BUILD_DEBUG
            if(output->getVerboseLevel() >= verboselevel) {
                std::string instcode = getInstCode();
                std::string fpinst = "FP";
                if(instcode.find(fpinst) != std::string::npos )
                {
                    output->verbose(
                    CALL_INFO, verboselevel, 0,
                    "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                    " / in=%" PRIu16 ", %" PRIu16 "\n",
                    getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_int_regs_out_0, phys_int_regs_in_0,
                    phys_int_regs_in_1, isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1]);
                }
                else
                output->verbose(
                    CALL_INFO, verboselevel, 0,
                    "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
                    " / in=%" PRIu16 ", %" PRIu16 "\n",
                    getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_int_regs_out_0, phys_int_regs_in_0,
                    phys_int_regs_in_1, isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
            }
            #endif
        }
        
        
        virtual void print(SST::Output* output) { output->verbose(CALL_INFO, 8, 0, "%s", getInstCode()); }

        // Is the instruction predicted (speculation point).
        // for normal instructions this is false
        // but branches and jumps will get predicte
        virtual bool isSpeculated() const { return false; }

        bool completedExecution() const { return hasExecuted; }
        bool completedIssue() const { return hasIssued; }

        virtual void markExecuted() { hasExecuted = true; }
        void markIssued() { hasIssued = true; }

        bool checkFrontOfROB() const { return isFrontOfROB; }
        void markFrontOfROB() { isFrontOfROB = true; }

        bool hasROBSlotIssued() const { return hasROBSlot; }
        void markROBSlotIssued() { hasROBSlot = true; }

        const VanadisDecoderOptions* getISAOptions() const { return isa_options; }

        void flagError() { trapError = true; }

        virtual bool performIntRegisterRecovery() const { return true; }
        virtual bool performFPRegisterRecovery() const { return true; }

        virtual bool updatesFPFlags() const { return false; }
        virtual void updateFPFlags() {}

        virtual void returnOutRegs( VanadisRegisterStack* int_stack, VanadisRegisterStack* fp_stack ) 
        {
            for ( auto i = 0; i < countPhysIntRegOut(); i++ ) {
                int_stack->push( getPhysIntRegOut(i) );
            }
            for ( auto i = 0; i < countPhysFPRegOut(); i++ ) {
                fp_stack->push( getPhysFPRegOut(i) );
            }
        }

        uint16_t getNumStores()
        {
            return 0;
        }
        
        

    protected:
        
        const uint64_t ins_address;
        const uint32_t hw_thread;

        uint16_t count_isa_int_reg_in;
        uint16_t count_isa_int_reg_out;
        uint16_t count_isa_fp_reg_in;
        uint16_t count_isa_fp_reg_out;

        
        uint16_t count_phys_int_reg_in;
        uint16_t count_phys_int_reg_out;
        uint16_t count_phys_fp_reg_in;
        uint16_t count_phys_fp_reg_out;

        bool trapError;
        bool hasExecuted;
        bool hasIssued;
        bool enduOpGroup;
        bool isFrontOfROB;
        bool hasROBSlot;        

        const VanadisDecoderOptions* isa_options;
        uint32_t sw_thread;
        uint16_t* isa_int_regs_in;
        uint16_t* isa_int_regs_out;
        uint16_t* isa_fp_regs_in;
        uint16_t* isa_fp_regs_out;

        uint16_t* phys_int_regs_in;
        uint16_t* phys_int_regs_out;
        uint16_t* phys_fp_regs_in;
        uint16_t* phys_fp_regs_out;
        
        
};

} // namespace Vanadis
} // namespace SST

#endif
