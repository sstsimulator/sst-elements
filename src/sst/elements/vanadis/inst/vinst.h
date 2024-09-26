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
// #include "inst/execute/vExecute.h"
#include <cstring>
#include <map>
#include <sst/core/output.h>
#include <string>
#include <sstream>
// #include <simt/simt_data_structure.h>


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
            isSIMT                = false;
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
            isSIMT                = copy_me.isSIMT;
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

        // common 
        uint16_t countPhysIntRegIn() const { return count_phys_int_reg_in; }
        uint16_t countPhysIntRegOut() const { return count_phys_int_reg_out; }
        uint16_t countPhysFPRegIn() const { return count_phys_fp_reg_in; }
        uint16_t countPhysFPRegOut() const { return count_phys_fp_reg_out; }

        // common
        uint16_t countISAIntRegIn() const { 
            // // printf("I am in countISAIntRegIn() %d\n", count_isa_int_reg_in);
            return count_isa_int_reg_in; }
        uint16_t countISAIntRegOut() const { return count_isa_int_reg_out; }
        uint16_t countISAFPRegIn() const { return count_isa_fp_reg_in; }
        uint16_t countISAFPRegOut() const { return count_isa_fp_reg_out; }

        uint16_t getPhysIntRegIn(const uint16_t index) const { return phys_int_regs_in[index]; }
        uint16_t getPhysIntRegOut(const uint16_t index) const { return phys_int_regs_out[index]; }
        uint16_t getISAIntRegIn(const uint16_t index) const { 
            // // printf("getISAIntRegIn len(isa_int_regs_in)=%d\n", isa_int_regs_in[index]);
            return isa_int_regs_in[index]; 
            }
        uint16_t getISAIntRegOut(const uint16_t index) const { return isa_int_regs_out[index]; }


        uint16_t getPhysFPRegIn(const uint16_t index) const { return phys_fp_regs_in[index]; }
        uint16_t getPhysFPRegOut(const uint16_t index) const { return phys_fp_regs_out[index]; }
        uint16_t getISAFPRegIn(const uint16_t index) const { return isa_fp_regs_in[index]; }
        uint16_t getISAFPRegOut(const uint16_t index) const { return isa_fp_regs_out[index]; }
        
        void setPhysIntRegIn(const uint16_t index, const uint16_t reg) { phys_int_regs_in[index] = reg; }
        void setPhysIntRegOut(const uint16_t index, const uint16_t reg) { phys_int_regs_out[index] = reg; }
        void setPhysFPRegIn(const uint16_t index, const uint16_t reg) { phys_fp_regs_in[index] = reg; }
        void setPhysFPRegOut(const uint16_t index, const uint16_t reg) { phys_fp_regs_out[index] = reg; }

        // different
       
        // same
        virtual VanadisInstruction* clone() = 0;
        // same
        void markEndOfMicroOpGroup() { enduOpGroup = true; }
        bool endsMicroOpGroup() const { return enduOpGroup; }
        bool trapsError() const { return trapError; }

        // same
        uint64_t getInstructionAddress() const { return ins_address; }
        uint32_t getHWThread() const { return hw_thread; }
        
        // same
        void setSWThread(uint32_t thr) { sw_thread=thr;}
        uint32_t getSWThread() {return sw_thread;}
        // same
        // virtual const char* getInstCode() const
        // {
        //     return "INST";
        // }

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
            // printf("VanadisInstruction scalarExecute\n");
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
            // printf("I am in VINST log\n");
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

        virtual bool getIsSIMT() { return false; } 

        // different
        uint16_t getNumStores()
        {
            // printf("VanadisInstruction getNumStores()\n");
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

        bool isSIMT;
        

        const VanadisDecoderOptions* isa_options;
        // std::vector<VanadisRegisterFile*>* registerFiles;

        uint32_t sw_thread;
        // std::vector<uint64_t> addresses_ldst;

        // std::vector<std::vector<uint16_t>> phys_int_regs_in_simt;
        // std::vector<std::vector<uint16_t>> phys_int_regs_out_simt;
        // std::vector<std::vector<uint16_t>> phys_fp_regs_out_simt;
        // std::vector<std::vector<uint16_t>> phys_fp_regs_in_simt;

        uint16_t* isa_int_regs_in;
        uint16_t* isa_int_regs_out;
        uint16_t* isa_fp_regs_in;
        uint16_t* isa_fp_regs_out;

        // different
        uint16_t* phys_int_regs_in;
        uint16_t* phys_int_regs_out;
        uint16_t* phys_fp_regs_in;
        uint16_t* phys_fp_regs_out;
        
        
};


class VanadisSIMTInstruction : public virtual VanadisInstruction
{
    
    public:
        VanadisSIMTInstruction(const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
            const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out, const uint16_t c_isa_int_reg_in,
            const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in, const uint16_t c_phys_fp_reg_out,
            const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out): 
        VanadisInstruction(address,hw_thr, isa_opts, c_phys_int_reg_in,c_phys_int_reg_out,c_isa_int_reg_in, c_isa_int_reg_out, c_phys_fp_reg_in,c_phys_fp_reg_out, c_isa_fp_reg_in,c_isa_fp_reg_out)
            {
                isSIMT = true;
                phys_int_regs_in_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_int_reg_in)); 
                phys_int_regs_out_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_int_reg_out)); 
                phys_fp_regs_out_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_fp_reg_out)); 
                phys_fp_regs_in_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_fp_reg_in)); 
            }
        
        virtual ~VanadisSIMTInstruction()
        {
            
            phys_int_regs_in_simt.clear();
            phys_int_regs_out_simt.clear();
            
            phys_fp_regs_in_simt.clear();
            phys_fp_regs_out_simt.clear();
            
        }

        VanadisSIMTInstruction(const VanadisSIMTInstruction& copy_me): VanadisInstruction(copy_me)
        {
            ;
            phys_int_regs_in_simt = copy_me.phys_int_regs_in_simt;
            phys_int_regs_out_simt = copy_me.phys_int_regs_out_simt;
            phys_fp_regs_in_simt = copy_me.phys_fp_regs_in_simt;
            phys_fp_regs_out_simt = copy_me.phys_fp_regs_out_simt;
        }

        uint16_t getPhysFPRegIn(uint16_t index, uint16_t sw_thr) { 
            // printf("I am in getPhysFPRegIn SIMTInst\n");
            if(phys_fp_regs_in_simt.size()==0)
            {
                // printf("phys_fp_regs_in_simt is empty SIMTInst\n");
            }

            if(phys_fp_regs_in_simt[sw_thr].size()==0)
            {
                // printf("phys_fp_regs_in_simt[%d] is empty SIMTInst\n", sw_thr);
            }
            return phys_fp_regs_in_simt[sw_thr][index]; 
            }
            
        uint16_t getPhysFPRegOut(uint16_t index, uint16_t sw_thr) { 
            // printf("I am in getPhysFPRegOut SIMTInst\n");
            if(phys_fp_regs_out_simt.size()==0)
            {
                // printf("phys_fp_regs_out_simt is empty SIMTInst\n");
            }

            if(phys_fp_regs_out_simt[sw_thr].size()==0)
            {
                // printf("phys_fp_regs_out_simt[%d] is empty SIMTInst\n", sw_thr);
            }
            return phys_fp_regs_out_simt[sw_thr][index]; 
            }
        uint16_t getPhysIntRegIn(uint16_t index, uint16_t sw_thr)
            {
                // printf("I am in getPhyIntRegIn SIMTInst\n");
                if(phys_int_regs_in_simt.size()==0)
                {
                    // printf("phys_int_regs_in_simt is empty SIMTInst\n");
                }

                if(phys_int_regs_in_simt[sw_thr].size()==0)
                {
                    // printf("phys_int_regs_in_simt[%d] is empty SIMTInst\n", sw_thr);
                }  
                return phys_int_regs_in_simt[sw_thr][index]; 
            }
            
        uint16_t getPhysIntRegOut(uint16_t index, uint16_t sw_thr){ 
            // printf("I am in getPhyIntRegOut SIMTInst\n");
            if(phys_int_regs_out_simt.size()==0)
            {
                // printf("phys_int_regs_out_simt is empty SIMTInst\n");
            }

            if(phys_int_regs_out_simt[sw_thr].size()==0)
            {
                // printf("phys_int_regs_out_simt[%d] is empty SIMTInst\n", sw_thr);
            }
            return phys_int_regs_out_simt[sw_thr][index]; }

        bool getIsSIMT() override { return true; }

        void setPhysIntRegIn(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        {
            // printf("I am in setPhyIntRegIn SIMTInst\n");
            if(phys_int_regs_in_simt.size()==0)
            {
                // printf("phys_int_regs_in_simt is empty SIMTInst\n");
            }

            if(phys_int_regs_in_simt[sw_thr].size()==0)
            {
                // printf("phys_int_regs_in_simt[%d] is empty SIMTInst\n", sw_thr);
            }
            phys_int_regs_in_simt[sw_thr][index]= reg; 
            if(sw_thr==hw_thread)
                VanadisInstruction::setPhysIntRegIn(index, reg);
        }
        
        void setPhysIntRegOut(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        { 
            // printf("I am in setPhyIntRegOut SIMTInst\n");
            phys_int_regs_out_simt[sw_thr][index]=reg; 
            if(sw_thr==hw_thread)
                VanadisInstruction::setPhysIntRegOut(index, reg);
        }
        
        void setPhysFPRegIn(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        { 
            // printf("I am in setPhyFPRegIn SIMTInst\n");
            phys_fp_regs_in_simt[sw_thr][index]= reg;
            if(sw_thr==hw_thread)
                VanadisInstruction::setPhysFPRegIn(index, reg);
        }
        
        void setPhysFPRegOut(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        { 
            // printf("I am in setPhyFPRegOut SIMTInst\n");
            phys_fp_regs_out_simt[sw_thr][index] =reg; 
            if(sw_thr==hw_thread)
                VanadisInstruction::setPhysFPRegOut(index, reg);
        }

        void setPhysIntRegIn(uint16_t sw_thr)
        {
            for(uint16_t i=0; i<count_phys_int_reg_in; i++)
            {
                phys_int_regs_in[i] = phys_int_regs_in_simt[sw_thr][i];
            }
        }

        void setPhysIntRegOut(uint16_t sw_thr)
        {
            for(uint16_t i=0; i<count_phys_int_reg_out; i++)
            {
                phys_int_regs_out[i] = phys_int_regs_out_simt[sw_thr][i];
            }
        }

        void setPhysFPRegIn(uint16_t sw_thr)
        {
            for(uint16_t i=0; i<count_phys_fp_reg_in; i++)
            {
                phys_fp_regs_in[i] = phys_fp_regs_in_simt[sw_thr][i];
            }
        }

        void setPhysFPRegOut(uint16_t sw_thr)
        {
            for(uint16_t i=0; i<count_phys_fp_reg_out; i++)
            {
                phys_fp_regs_out[i] = phys_fp_regs_out_simt[sw_thr][i];
            }
        }

        void setPhysReg(uint16_t sw_thr)
        {
            setPhysIntRegIn(sw_thr);
            setPhysIntRegOut(sw_thr);
            setPhysFPRegOut(sw_thr);
            setPhysFPRegIn(sw_thr);
        }

        void ResetPhysReg()
        {
            setPhysIntRegIn(hw_thread);
            setPhysIntRegOut(hw_thread);
            setPhysFPRegOut(hw_thread);
            setPhysFPRegIn(hw_thread);
        }

        uint16_t getNumStores()
        {
            // printf("VanadisSIMTInstruction getNumStores()\n");
            return 0;
        }

        void setSWThread(uint32_t thr) { sw_thread=thr;}
        uint32_t getSWThread() {return sw_thread;}
        

        virtual void                      execute(SST::Output* output, std::vector<VanadisRegisterFile*>& regFiles) override
        {
            // printf("I am in VanadisInstruction::execute regFiles\n");
            for(uint16_t t = 0; t < (WARP_SIZE); t++)
            {
                // if((t+sw_thr)>=regFile->getThreadCount())
                // {
                //     break;
                // }
                if((t+hw_thread) >= 5)
                {
                    break;
                }
                setSWThread(t+hw_thread);
                simtExecute(output, regFiles[t+hw_thread]);

            }
            this->markExecuted();
            
        }

        virtual void simtExecute(SST::Output* output, VanadisRegisterFile* regFile)
        {
            // // printf("virtual simtExecute\n");
            uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0,sw_thread);
            uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0,sw_thread);
            uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1,sw_thread);
            log(output, 16,sw_thread,phys_int_regs_out_0,phys_int_regs_in_0,phys_int_regs_in_1);
            instOp(regFile,phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
        }

        void setIsSIMT()  
         { 
            isSIMT=true; 
            phys_int_regs_in_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_int_reg_in)); 
            phys_int_regs_out_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_int_reg_out)); 
            phys_fp_regs_out_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_fp_reg_out)); 
            phys_fp_regs_in_simt.resize(WARP_SIZE, std::vector<uint16_t>(count_phys_fp_reg_in)); 
        }
        
        void resetIsSIMT() { isSIMT=false; }

    protected:
        // bool isSIMT;
        uint32_t sw_thread;
        active_mask_t inst_mask;
        std::vector<uint64_t> addresses_ldst;
        std::vector<std::vector<uint16_t>> phys_int_regs_in_simt;
        std::vector<std::vector<uint16_t>> phys_int_regs_out_simt;
        std::vector<std::vector<uint16_t>> phys_fp_regs_out_simt;
        std::vector<std::vector<uint16_t>> phys_fp_regs_in_simt;
        // static std::vector<VanadisRegisterFile*> regFiles;

};

} // namespace Vanadis
} // namespace SST

#endif
