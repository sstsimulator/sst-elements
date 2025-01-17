// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_ISA_TABLE
#define _H_VANADIS_ISA_TABLE

#include "decoder/visaopts.h"
#include "inst/regfile.h"

#include <cstdint>

#ifndef PRI_ADDR
#define PRI_ADDR PRIx64
#endif

namespace SST {
namespace Vanadis {

class VanadisISATable
{
public:
    VanadisISATable( const std::string& name, const VanadisDecoderOptions* decoder_o, const uint16_t int_reg, const uint16_t fp_reg) :
        decoder_opts(decoder_o),
        count_int_reg(int_reg),
        count_fp_reg(fp_reg),
        tblName(name)
    {
        int_reg_ptr = new uint16_t[int_reg];
        fp_reg_ptr  = new uint16_t[fp_reg];

        int_reg_pending_read = new uint32_t[int_reg];
        fp_reg_pending_read  = new uint32_t[fp_reg];

        int_reg_pending_write = new uint32_t[int_reg];
        fp_reg_pending_write  = new uint32_t[fp_reg];

        for ( uint16_t i = 0; i < count_int_reg; ++i ) { int_reg_ptr[i] = 0; }
        for ( uint16_t i = 0; i < count_fp_reg; ++i )  { fp_reg_ptr[i]  = 0; }
        resetPendingCnts();
    }

    void resetPendingCnts() {
        for ( uint16_t i = 0; i < count_int_reg; ++i ) {
            int_reg_pending_read[i]  = 0;
            int_reg_pending_write[i] = 0;
        }

        for ( uint16_t i = 0; i < count_fp_reg; ++i ) {
            fp_reg_pending_read[i]  = 0;
            fp_reg_pending_write[i] = 0;
        }
    }

    ~VanadisISATable()
    {
        delete int_reg_ptr;
        delete int_reg_pending_read;
        delete int_reg_pending_write;
        delete fp_reg_ptr;
        delete fp_reg_pending_read;
        delete fp_reg_pending_write;
    }

    int getNumIntRegs() { return count_int_reg; }
    int getNumFpRegs() { return count_fp_reg; }

    bool pendingIntReads(const uint16_t int_reg) { return int_reg_pending_read[int_reg] > 0; }

    bool pendingIntWrites(const uint16_t int_reg) { return int_reg_pending_write[int_reg] > 0; }

    bool pendingFPReads(const uint16_t fp_reg) { return fp_reg_pending_read[fp_reg] > 0; }

    bool pendingFPWrites(const uint16_t fp_reg) { return fp_reg_pending_write[fp_reg] > 0; }

    void incIntRead(const uint16_t int_reg)
    {
        if ( int_reg != decoder_opts->getRegisterIgnoreWrites() ) { int_reg_pending_read[int_reg]++; }
    }

    void incIntWrite(const uint16_t int_reg)
    {
        if ( int_reg != decoder_opts->getRegisterIgnoreWrites() ) { int_reg_pending_write[int_reg]++; }
    }

    void decIntRead(const uint16_t int_reg)
    {
        if ( int_reg != decoder_opts->getRegisterIgnoreWrites() ) { int_reg_pending_read[int_reg]--; }
    }

    void decIntWrite(const uint16_t int_reg)
    {
        if ( int_reg != decoder_opts->getRegisterIgnoreWrites() ) { int_reg_pending_write[int_reg]--; }
    }

    void incFPRead(const uint16_t fp_reg) { fp_reg_pending_read[fp_reg]++; }

    void incFPWrite(const uint16_t fp_reg) { fp_reg_pending_write[fp_reg]++; }

    void decFPRead(const uint16_t fp_reg) { fp_reg_pending_read[fp_reg]--; }

    void decFPWrite(const uint16_t fp_reg) { fp_reg_pending_write[fp_reg]--; }

    void setIntPhysReg(const uint16_t int_reg, const uint16_t phys_reg) { int_reg_ptr[int_reg] = phys_reg; }

    void setFPPhysReg(const uint16_t fp_reg, const uint16_t phys_reg) { fp_reg_ptr[fp_reg] = phys_reg; }

    uint16_t getIntPhysReg(const uint16_t int_reg) { assert(int_reg < count_int_reg); return int_reg_ptr[int_reg]; }

    uint16_t getFPPhysReg(const uint16_t fp_reg) { assert(fp_reg < count_fp_reg); return fp_reg_ptr[fp_reg]; }

    void reset(VanadisISATable* tbl)
    {
        for ( uint16_t i = 0; i < count_int_reg; ++i ) {
            int_reg_ptr[i]           = tbl->int_reg_ptr[i];
            int_reg_pending_read[i]  = tbl->int_reg_pending_read[i];
            int_reg_pending_write[i] = tbl->int_reg_pending_write[i];
        }

        for ( uint16_t i = 0; i < count_fp_reg; ++i ) {
            fp_reg_ptr[i]           = tbl->fp_reg_ptr[i];
            fp_reg_pending_read[i]  = tbl->fp_reg_pending_read[i];
            fp_reg_pending_write[i] = tbl->fp_reg_pending_write[i];
        }
    }

    void print(SST::Output* output, VanadisRegisterFile* regFile, bool print_int, bool print_fp)
    {
        print(output, regFile, print_int, print_fp, 16);
    }

    void print(SST::Output* output, VanadisRegisterFile* regFile, bool print_int, bool print_fp, uint32_t output_v)
    {
        if ( print_int ) {
            output->verbose(CALL_INFO, output_v, 0, "[Thread: %d]: %s: Integer Registers (Count=%" PRIu16 ")\n", 
                regFile ? regFile->getHWThread() : -1, tblName.c_str(), count_int_reg);
            for ( uint16_t i = 0; i < count_int_reg; ++i ) {
                if ( nullptr == regFile ) {
                    output->verbose(
                        CALL_INFO, output_v, 0,
                        "| isa:%5" PRIu16 " -> phys:%5" PRIu16 " | r:%6" PRIu32 " | w:%6" PRIu32 " |\n", i,
                        int_reg_ptr[i], int_reg_pending_read[i], int_reg_pending_write[i]);
                }
                else {
                    output->verbose(
                        CALL_INFO, output_v, 0,
                        "[Thread: %d]: | isa:%5" PRIu16 " -> phys:%5" PRIu16 " | r:%5" PRIu32 " | w:%5" PRIu32
                        " | v: 0x%016" PRI_ADDR " | v: %" PRIu64 " / %" PRId64 "\n",
                        regFile-> getHWThread(), i, int_reg_ptr[i], int_reg_pending_read[i], int_reg_pending_write[i],
                        regFile->getIntReg<int64_t>(int_reg_ptr[i]), regFile->getIntReg<uint64_t>(int_reg_ptr[i]),
                        regFile->getIntReg<int64_t>(int_reg_ptr[i]));
                }
            }
        }

        if ( print_fp ) {

            output->verbose(CALL_INFO, output_v, 0, "Floating-Point Registers (Count=%" PRIu16 ")\n", count_fp_reg);
            for ( uint16_t i = 0; i < count_fp_reg; ++i ) {
                if ( nullptr == regFile ) {
                    output->verbose(
                        CALL_INFO, output_v, 0,
                        "| isa:%5" PRIu16 " -> phys:%5" PRIu16 " | r:%5" PRIu32 " | w:%5" PRIu32 " |\n", i,
                        fp_reg_ptr[i], fp_reg_pending_read[i], fp_reg_pending_write[i]);
                }
                else {
                    if ( VANADIS_REGISTER_MODE_FP32 == decoder_opts->getFPRegisterMode() ) {
                        output->verbose(
                            CALL_INFO, output_v, 0,
                            "| isa:%5" PRIu16 " -> phys:%5" PRIu16 " | r:%5" PRIu32 " | w:%5" PRIu32
                            " | v: 0x%08x |\n",
                            i, fp_reg_ptr[i], fp_reg_pending_read[i], fp_reg_pending_write[i],
                            regFile->getFPReg<uint32_t>(fp_reg_ptr[i]));
                    }
                    else {
                        output->verbose(
                            CALL_INFO, output_v, 0,
                            "| isa:%5" PRIu16 " -> phys:%5" PRIu16 " | r:%5" PRIu32 " | w:%5" PRIu32
                            " | v: 0x%016" PRI_ADDR " |\n",
                            i, fp_reg_ptr[i], fp_reg_pending_read[i], fp_reg_pending_write[i],
                            regFile->getFPReg<uint64_t>(fp_reg_ptr[i]));
                    }
                }
            }
        }

        // delete[] reg_bin_str;
    }

    void print(SST::Output* output, bool print_int, bool print_fp) { print(output, nullptr, print_int, print_fp); }

    void toBinaryString(char* buffer, int64_t val)
    {
        int index = 0;

        for ( uint64_t i = 1L << 63L; i > 0; i = i / 2 ) {
            buffer[index++] = (val & i) ? '1' : '0';
        }

        buffer[index] = '\0';
    }

    bool physIntRegInUse(const uint16_t reg) { return findInRegSet(reg, int_reg_ptr, count_int_reg); }
    bool physFPRegInUse(const uint16_t reg) { return findInRegSet(reg, fp_reg_ptr, count_fp_reg); }

protected:
    bool findInRegSet(const uint16_t reg, const uint16_t* reg_set, const uint16_t reg_count) const
    {
        bool found = false;

        for ( uint16_t i = 0; i < reg_count; ++i ) {
            if ( reg == reg_set[i] ) { found = true; }
        }

        return found;
    }

    const uint16_t count_int_reg;
    const uint16_t count_fp_reg;

    uint16_t* int_reg_ptr;
    uint16_t* fp_reg_ptr;

    uint32_t* int_reg_pending_read;
    uint32_t* fp_reg_pending_read;

    uint32_t* int_reg_pending_write;
    uint32_t* fp_reg_pending_write;

    const VanadisDecoderOptions* decoder_opts;

    std::string tblName; 
};

} // namespace Vanadis
} // namespace SST

#endif
