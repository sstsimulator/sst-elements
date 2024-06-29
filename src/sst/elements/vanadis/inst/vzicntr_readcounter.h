
#ifndef _H_VANADIS_ZICNTR_READ_COUNTER
#define _H_VANADIS_ZICNTR_READ_COUNTER

#include "inst/vinst.h"
#include "inst/vzicntr.h"

namespace SST::Vanadis
{
    namespace Zicntr
    {
        template <uint32_t id, size_t XLEN = 64, bool H = false>
        class VanadisReadCounterInstruction : public VanadisInstruction
        {
            static_assert( id < 3 );
            static_assert( XLEN == 64 or XLEN == 32 );
            static_assert( XLEN != 64 or H == false );
    
          public:
            VanadisReadCounterInstruction(
                    const std::integral_constant<uint32_t, id>,
                    const uint64_t addr,
                    const uint32_t hw_thr,
                    const VanadisDecoderOptions* isa_opts,
                    const uint16_t dest)
                    : VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0)
            {
                isa_int_regs_out[0] = dest;
            }

            VanadisReadCounterInstruction* clone() override
            {
                return new VanadisReadCounterInstruction(*this);
            }
    
            VanadisFunctionalUnitType getInstFuncType() const override
            {
                return INST_INT_ARITH;                  // Is this appropriate?
            }

            const char* getInstCode() const override
            {
                switch ( id ) {
                  case Zicntr::CYCLE:   return H ? "RDCYCLEH"   : "RDCYCLE";
                  case Zicntr::TIME:    return H ? "RDTIMEH"    : "RDTIME";
                  case Zicntr::INSTRET: return H ? "RDINSTRETH" : "RDINSTRET";
                }
                __builtin_unreachable();
            }

            void printToBuffer(char* const buffer, const size_t buffer_size) override
            {
                snprintf(
                        buffer, buffer_size,
                        "%s    %5" PRIu16 " (phys: %5" PRIu16 ")",
                        getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0]);
            }

            void execute(SST::Output* const output, VanadisRegisterFile* const regFile) override
            {
#ifdef VANADIS_BUILD_DEBUG
                if(output->getVerboseLevel() >= 16) {
                    output->verbose(
                            CALL_INFO, 16, 0,
                            "Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 ", isa: out=%" PRIu16 "\n",
                            getInstructionAddress(), getInstCode(), phys_int_regs_out[0], isa_int_regs_out[0]);
                }
#endif

                static constexpr uint64_t mask = 0x00000000'FFFFFFFF;
                const uint64_t count64  = regFile->getCounter(id);
                const uint32_t count32  = count64 & mask;
                const uint32_t count32H = count64 >> 32;
        
                if constexpr ( XLEN == 64 ) {
                    regFile->setIntReg(phys_int_regs_out[0], count64);
                }
                else if constexpr ( XLEN == 32 and H ) {
                    regFile->setIntReg(phys_int_regs_out[0], count32H);
                }
                else {
                    regFile->setIntReg(phys_int_regs_out[0], count32);
                }
        
                markExecuted();
            }
        };
    } // namespace Zicntr
} // namespace Vanadis::SST

#endif
