// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_ADD
#define _H_VANADIS_ADD

#include "inst/vinst.h"
// #include "inst/execute/vExecuteAddSIMTScalar.h"

namespace SST {
namespace Vanadis {


template <typename gpr_format>
class VanadisAddInstruction : public virtual VanadisInstruction
{
    public:
        VanadisAddInstruction(const uint64_t addr, const uint32_t hw_thr, 
            const VanadisDecoderOptions* isa_opts, const uint16_t dest,
            const uint16_t src_1, const uint16_t src_2) :
            VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
        {

            isa_int_regs_in[0]  = src_1;
            isa_int_regs_in[1]  = src_2;
            isa_int_regs_out[0] = dest;
        }

        VanadisAddInstruction*    clone() override { return new VanadisAddInstruction(*this); }
        VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }

        const char* getInstCode() const override
        {
                if(sizeof(gpr_format) == 8) {
                    return "ADD64";
                } else {
                    return "ADD32";
                }
        }

        void printToBuffer(char* buffer, size_t buffer_size) override
        {
            snprintf(
                buffer, buffer_size,
                "%s    %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
                getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0],
                phys_int_regs_in[0], phys_int_regs_in[1]);
        }

        

        void instOp(VanadisRegisterFile* regFile, 
        uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0, 
        uint16_t phys_int_regs_in_1)
        {
            printf("I am in ADD scalarins scalarExecute\n");
            const gpr_format src_1 = regFile->getIntReg<gpr_format>(phys_int_regs_in_0);
            const gpr_format src_2 = regFile->getIntReg<gpr_format>(phys_int_regs_in_1);
            // add(src_1, src_2);
            regFile->setIntReg<gpr_format>(phys_int_regs_out_0,src_1+src_2);
        }

        // void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
        // {
        //     printf("I am in ADD scalarins execute\n");
        //     uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        //     uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        //     uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1);
        //     log(output, 16, 65535,phys_int_regs_out_0,phys_int_regs_in_0,phys_int_regs_in_1);
        //     instOp(regFile,phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
        //     markExecuted();
        // }
};

template <typename gpr_format>
class VanadisSIMTAddInstruction : public VanadisSIMTInstruction, public VanadisAddInstruction<gpr_format>
{
    public:
        VanadisSIMTAddInstruction(const uint64_t addr, const uint32_t hw_thr, 
            const VanadisDecoderOptions* isa_opts, const uint16_t dest,
            const uint16_t src_1, const uint16_t src_2) :
            VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0),
            VanadisSIMTInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0), 
            VanadisAddInstruction<gpr_format>(addr, hw_thr, isa_opts,dest, src_1, src_2)
        {

            this->isa_int_regs_in[0]  = src_1;
            this->isa_int_regs_in[1]  = src_2;
            this->isa_int_regs_out[0] = dest;
        }

        VanadisSIMTAddInstruction*    clone() override { return new VanadisSIMTAddInstruction(*this); }

        // void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
        // {
        //     uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0,VanadisSIMTInstruction::sw_thread);
        //     uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0,VanadisSIMTInstruction::sw_thread);
        //     uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1,VanadisSIMTInstruction::sw_thread);
        //     this->log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
        //     this->instOp(regFile,phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
        // }

        // void printMyType()
        // {
        //     printf("I am SIMTAdd\n");
        // }

        // uint16_t getPhysFPRegIn(uint16_t index, uint16_t sw_thr) { return VanadisSIMTInstruction::phys_fp_regs_in_simt[sw_thr][index]; }
        // uint16_t getPhysFPRegOut(uint16_t index, uint16_t sw_thr) { return VanadisSIMTInstruction::phys_fp_regs_out_simt[sw_thr][index]; }
        // uint16_t getPhysIntRegIn(uint16_t index, uint16_t sw_thr){ 
        //     printf("I am in SIMTAdd getPhysIntRegIn sw_thr=%d\n", sw_thr);
        //     return VanadisSIMTInstruction::phys_int_regs_in_simt[sw_thr][index]; }
        // uint16_t getPhysIntRegOut(uint16_t index, uint16_t sw_thr){ 
        //     printf("I am in SIMTAdd getPhysIntRegOut sw_thr=%d\n", sw_thr);
        //     return VanadisSIMTInstruction::phys_int_regs_out_simt[sw_thr][index]; }

        // bool getIsSIMT() const { return true; }

        // void setPhysIntRegIn(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        // {
        //     printf("I am in setPhyIntRegIn SIMTInst sw_thr=%d\n", sw_thr);
        //     // if(phys_int_regs_in_simt.size()==0)
        //     // {
        //     //     printf("phys_int_regs_in_simt is empty SIMTInst\n");
        //     // }

        //     // if(phys_int_regs_in_simt[sw_thr].size()==0)
        //     // {
        //     //     printf("phys_int_regs_in_simt[%d] is empty SIMTInst\n", sw_thr);
        //     // }
        //     // phys_int_regs_in_simt[sw_thr][index]= reg; 
        //     // if(sw_thr==hw_thread)
        //     //     VanadisInstruction::setPhysIntRegIn(index, reg);
        //     VanadisSIMTInstruction::setPhysIntRegIn(index, reg, sw_thr);
        // }
        
        // void setPhysIntRegOut(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        // { 
        //     // phys_int_regs_out_simt[sw_thr][index]=reg; 
        //     // if(sw_thr==hw_thread)
        //     //     VanadisInstruction::setPhysIntRegOut(index, reg);
        //     VanadisSIMTInstruction::setPhysIntRegOut(index, reg, sw_thr);
        // }
        
        // void setPhysFPRegIn(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        // { 
        //     VanadisSIMTInstruction::phys_fp_regs_in_simt[sw_thr][index]= reg;
        //     if(sw_thr==hw_thread)
        //         VanadisInstruction::setPhysFPRegIn(index, reg);
        // }
        
        // void setPhysFPRegOut(const uint16_t index, const uint16_t reg, uint16_t sw_thr) 
        // { 
        //     VanadisSIMTInstruction::phys_fp_regs_out_simt[sw_thr][index] =reg; 
        //     if(sw_thr==hw_thread)
        //         VanadisInstruction::setPhysFPRegOut(index, reg);
        // }

        // uint16_t getNumStores()
        // {
        //     printf("VanadisSIMTInstruction getNumStores()\n");
        //     return 0;
        // }

};


} // namespace Vanadis
} // namespace SST

#endif
