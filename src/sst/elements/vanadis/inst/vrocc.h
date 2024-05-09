#ifndef _H_VANADIS_ROCC_INST
#define _H_VANADIS_ROCC_INST

#include "inst/vinst.h"
#include "inst/vrocc.h"

#include "rocc/vroccinterface.h"

#include <cstdio>

namespace SST {
namespace Vanadis {

class VanadisRoCCInstruction : public VanadisInstruction
{

public:
    VanadisRoCCInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, 
        const uint16_t rs1, const int16_t rs2, const uint16_t rd, const bool xd, const bool xs1, 
        const bool xs2, uint32_t func_code7, uint8_t accelerator_id) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0] = rs1;
        isa_int_regs_in[1] = rs2;
        isa_int_regs_out[0] = rd;
        
        this->func7 = func_code7;
        this->rd = rd;
        this->xs1 = xs1;
        this->xs2 = xs2;
        this->xd = xd;

        switch (accelerator_id) {
            case 0:
                this->funcType = INST_ROCC0;
                this->instCode = "RoCC0";
                break;

            case 1:
                this->funcType = INST_ROCC1;
                this->instCode = "RoCC1";
                break;

            case 2:
                this->funcType = INST_ROCC2;
                this->instCode = "RoCC2";
                break;
            
            case 3:
                this->funcType = INST_ROCC3;
                this->instCode = "RoCC3";
                break;
            
            default:
                this->funcType = INST_ROCC0;
                this->instCode = "RoCC0";
                break;
        }

    }

    VanadisRoCCInstruction* clone() { return new VanadisRoCCInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return funcType; }

    virtual const char* getInstCode() const { return "RoCC"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size)
    {
        snprintf(buffer, buffer_size, "RoCC");
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
        markExecuted(); 
    }

    std::string instCode;
    VanadisFunctionalUnitType funcType;
    uint8_t func7;
    uint8_t rd;
    bool xs1;
    bool xs2;
    bool xd;
};

} // namespace Vanadis
} // namespace SST

#endif
