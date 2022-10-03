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

#ifndef _H_VANADIS_PARTIAL_LOAD
#define _H_VANADIS_PARTIAL_LOAD

namespace SST {
namespace Vanadis {

class VanadisPartialLoadInstruction : public VanadisLoadInstruction
{

public:
    VanadisPartialLoadInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t memAddrReg,
        const int64_t offst, const uint16_t tgtReg, const uint16_t load_bytes, const bool extend_sign,
        const bool isLowerLoad, VanadisLoadRegisterType regT) :
        VanadisLoadInstruction(
            addr, hw_thr, isa_opts, memAddrReg, offst, tgtReg, load_bytes, extend_sign, MEM_TRANSACTION_NONE, regT),
        is_load_lower(isLowerLoad)
    {

        // We need an extra in register here

        delete[] phys_int_regs_in;
        delete[] isa_int_regs_in;

        count_isa_int_reg_in  = 2;
        count_phys_int_reg_in = 2;

        phys_int_regs_in = new uint16_t[count_phys_int_reg_in];
        isa_int_regs_in  = new uint16_t[count_isa_int_reg_in];

        isa_int_regs_out[1] = tgtReg;
        isa_int_regs_in[0]  = memAddrReg;
        isa_int_regs_in[1]  = tgtReg;

        register_offset = 0;
    }

    VanadisPartialLoadInstruction* clone() { return new VanadisPartialLoadInstruction(*this); }

    bool isPartialLoad() const { return true; }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_LOAD; }
    virtual const char*               getInstCode() const { return "PARTLOAD"; }

    uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
    uint16_t getTargetRegister() const { return phys_int_regs_in[1]; }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) { markExecuted(); }

    virtual void printToBuffer(char* buffer, size_t buffer_size)
    {
        snprintf(
            buffer, buffer_size,
            "PARTLOAD  %5" PRIu16 " <- memory[ %5" PRIu16 " + %20" PRId64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16
            " + %20" PRId64 "]) / width: %" PRIu16 "\n",
            isa_int_regs_out[0], isa_int_regs_in[0], offset, phys_int_regs_out[0], phys_int_regs_in[0], offset,
            load_width);
    }

    void computeLoadAddress(SST::Output* output, VanadisRegisterFile* regFile, uint64_t* out_addr, uint16_t* width)
    {
        const uint64_t mem_addr_reg_val = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0, "[execute-partload]: reg[%5" PRIu16 "]: %" PRIu64 " / 0x%llx\n", phys_int_regs_in[0],
            mem_addr_reg_val, mem_addr_reg_val);
        output->verbose(
            CALL_INFO, 16, 0, "[execute-partload]: offset           : %" PRIu64 " / 0x%llx\n", offset, offset);
        output->verbose(
            CALL_INFO, 16, 0, "[execute-partload]: (add)            : %" PRIu64 " / 0x%llx\n",
            (mem_addr_reg_val + offset), (mem_addr_reg_val + offset));
#endif

        computeLoadAddress(regFile, out_addr, width);

#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0, "[execute-partload]: full width: %" PRIu16 "\n", load_width);
        output->verbose(
            CALL_INFO, 16, 0, "[execute-partload]: (lower/upper load ? %s)\n", is_load_lower ? "lower" : "upper");
        output->verbose(
            CALL_INFO, 16, 0, "[execute-partload]: load-addr: %" PRIu64 " / 0x%0llx / load-width: %" PRIu16 "\n",
            (*out_addr), (*out_addr), (*width));
        output->verbose(CALL_INFO, 16, 0, "[execute-partload]: register-offset: %" PRIu16 "\n", register_offset);
#endif
    }

    uint16_t getLoadWidth() const { return load_width; }

    VanadisLoadRegisterType getValueRegisterType() const { return LOAD_INT_REGISTER; }

    uint16_t getRegisterOffset() const { return register_offset; }

protected:
    void computeLoadAddress(VanadisRegisterFile* reg, uint64_t* out_addr, uint16_t* width)
    {
        const uint64_t width_64 = (uint64_t)load_width;

        int64_t reg_tmp = reg->getIntReg<int64_t>(getMemoryAddressRegister());

        const uint64_t base_address =
            is_load_lower ? (uint64_t)(reg_tmp + offset) : (uint64_t)(reg_tmp + offset) - width_64;

        const uint64_t read_lower_addr = base_address;
        const uint64_t read_lower_len =
            (base_address % width_64) == 0 ? width_64 : width_64 - (base_address % width_64);
        const uint64_t read_upper_addr =
            (read_lower_len == width_64) ? read_lower_addr : read_lower_addr + read_lower_len;
        const uint64_t read_upper_len = (read_lower_len == width_64) ? width_64 : width_64 - read_lower_len;

        if ( is_load_lower ) {
            (*out_addr) = read_lower_addr;
            (*width)    = read_lower_len;

            register_offset = 0;
        }
        else {
            (*out_addr) = read_upper_addr;
            (*width)    = read_upper_len;

            if ( read_lower_len == width_64 ) { register_offset = 0; }
            else {
                register_offset = read_lower_len;
            }
        }
    }

    uint16_t   register_offset;
    const bool is_load_lower;
};

} // namespace Vanadis
} // namespace SST

#endif
