// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_REG_FILE
#define _H_VANADIS_REG_FILE


#include "decoder/visaopts.h"
#include "inst/fpregmode.h"

#include <cstring>
#include <sst/core/output.h>
#include <sst/core/sst_types.h>

namespace SST {
namespace Vanadis {

class VanadisRegisterFile
{

public:
    VanadisRegisterFile(
        const uint32_t thr, const VanadisDecoderOptions* decoder_ots, const uint16_t int_regs, const uint16_t fp_regs,
        const VanadisFPRegisterMode fp_rmode) :
        hw_thread(thr),
        count_int_regs(int_regs),
        count_fp_regs(fp_regs),
        decoder_opts(decoder_ots),
        fp_reg_mode(fp_rmode)
    {

        int_reg_width = 8;

        // Registers are always 64-bits
        int_reg_storage = new char[int_reg_width * count_int_regs];
std:
        memset(int_reg_storage, 0, (int_reg_width * count_int_regs));

        switch ( fp_reg_mode ) {
        case VANADIS_REGISTER_MODE_FP32:
        {
            fp_reg_width = 4;
        } break;
        case VANADIS_REGISTER_MODE_FP64:
        {
            fp_reg_width = 8;
        } break;
        }

        fp_reg_storage = new char[fp_reg_width * count_fp_regs];
        std::memset(fp_reg_storage, 0, (fp_reg_width * count_fp_regs));
    }

    ~VanadisRegisterFile()
    {
        delete[] int_reg_storage;
        delete[] fp_reg_storage;
    }

    const VanadisDecoderOptions* getDecoderOptions() const { return decoder_opts; }

    char* getIntReg(const uint16_t reg)
    {
        assert(reg < count_int_regs);
        return int_reg_storage + (int_reg_width * reg);
    }

    char* getFPReg(const uint16_t reg)
    {
        assert(reg < count_fp_regs);
        return fp_reg_storage + (fp_reg_width * reg);
    }

    template <typename T>
    T getIntReg(const uint16_t reg)
    {
        assert(reg < count_int_regs);

        if ( reg != decoder_opts->getRegisterIgnoreWrites() ) {
            char* reg_start   = &int_reg_storage[reg * int_reg_width];
            T*    reg_start_T = (T*)reg_start;
            return *(reg_start_T);
        }
        else {
            return T();
        }
    }

    template <typename T>
    T getFPReg(const uint16_t reg)
    {
        assert(reg < count_fp_regs);

        char* reg_start   = &fp_reg_storage[reg * fp_reg_width];
        T*    reg_start_T = (T*)reg_start;
        return *(reg_start_T);
    }

    template <typename T>
    void setIntReg(const uint16_t reg, const T val, const bool sign_extend = true)
    {
        assert(reg < count_int_regs);

        if ( LIKELY(reg != decoder_opts->getRegisterIgnoreWrites()) ) {
            T*    reg_ptr_t = (T*)(&int_reg_storage[int_reg_width * reg]);
            char* reg_ptr_c = (char*)(reg_ptr_t);

            reg_ptr_t[0] = val;

            // if we need to sign extend, check if the most-significant bit is a 1, if yes then
            // fill with 0xFF, otherwise fill with 0x00
            std::memset(
                &reg_ptr_c[sizeof(T)],
                sign_extend ? ((val & (static_cast<T>(1) << (sizeof(T) * 8 - 1))) == 0) ? 0x00 : 0xFF : 0x00,
                int_reg_width - sizeof(T));
        }
    }

    template <typename T>
    void setFPReg(const uint16_t reg, const T val)
    {
        assert(reg < count_fp_regs);

        *((T*)&fp_reg_storage[fp_reg_width * reg]) = val;
    }

    uint32_t getHWThread() const { return hw_thread; }
    uint16_t countIntRegs() const { return count_int_regs; }
    uint16_t countFPRegs() const { return count_fp_regs; }

    void print(SST::Output* output)
    {
        output->verbose(CALL_INFO, 8, 0, "Integer Registers:\n");

        for ( uint16_t i = 0; i < count_int_regs; ++i ) {
            printRegister(output, true, i);
        }

        output->verbose(CALL_INFO, 8, 0, "Floating Point Registers:\n");

        for ( uint16_t i = 0; i < count_int_regs; ++i ) {
            printRegister(output, false, i);
        }
    }

private:
    void printRegister(SST::Output* output, bool isInt, uint16_t reg)
    {
        char* ptr = NULL;

        if ( isInt ) { ptr = getIntReg(reg); }
        else {
            ptr = getFPReg(reg);
        }

        char* val_string = new char[65];
        val_string[64]   = '\0';
        int index        = 0;

        const long long int v = ((long long int*)ptr)[0];

        for ( unsigned long long int i = 1L << 63L; i > 0; i = i / 2 ) {
            val_string[index++] = (v & i) ? '1' : '0';
        }

        output->verbose(CALL_INFO, 8, 0, "R[%5" PRIu16 "]: %s\n", reg, val_string);
        delete[] val_string;
    }

    const uint32_t               hw_thread;
    const uint16_t               count_int_regs;
    const uint16_t               count_fp_regs;
    const VanadisDecoderOptions* decoder_opts;

    char* int_reg_storage;
    char* fp_reg_storage;

    VanadisFPRegisterMode fp_reg_mode;
    uint32_t              fp_reg_width;
    uint32_t              int_reg_width;
};

} // namespace Vanadis
} // namespace SST

#endif
