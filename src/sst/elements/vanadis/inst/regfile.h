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
        fp_reg_mode(fp_rmode),
		int_reg_width(8),
		fp_reg_width( (fp_rmode == VANADIS_REGISTER_MODE_FP32) ? 4 : 8 )
    {
        // Registers are always 64-bits
        int_reg_storage = new char[int_reg_width * count_int_regs];
        fp_reg_storage = new char[fp_reg_width * count_fp_regs];

        init();
    }

    void init( ) {
        std::memset(int_reg_storage, 0, (int_reg_width * count_int_regs));
        std::memset(fp_reg_storage, 0, (fp_reg_width * count_fp_regs));
    }

    ~VanadisRegisterFile()
    {
        delete[] int_reg_storage;
        delete[] fp_reg_storage;
    }

    const VanadisDecoderOptions* getDecoderOptions() const { return decoder_opts; }

    uint32_t getIntRegWidth() const {
        return int_reg_width;
    }

    uint32_t getFPRegWidth() const {
        return fp_reg_width;
    }

    void copyFromRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len, bool is_fp) {
        if(is_fp) {
            copyFromFPRegister(reg, offset, values, len);
        } else {
            copyFromIntRegister(reg, offset, values, len);
        }
    }

    void copyFromFPRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert(reg < count_fp_regs);
        assert((offset + len) <= fp_reg_width);

        uint8_t* reg_ptr = (uint8_t*) &fp_reg_storage[reg * fp_reg_width];

        for(auto i = 0; i < len; ++i) {
            values[i] = reg_ptr[offset + i];
        }
    }

    void copyFromIntRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert(reg < count_int_regs);
        assert((offset + len) <= int_reg_width);

        uint8_t* reg_ptr = (uint8_t*) &int_reg_storage[reg * int_reg_width];

        for(auto i = 0; i < len; ++i) {
            values[i] = reg_ptr[offset + i];
        }
    }

    void copyToRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len, bool is_fp) {
        if(is_fp) {
            copyToFPRegister(reg, offset, values, len);
        } else {
            copyToIntRegister(reg, offset, values, len);
        }
    }

    void copyToIntRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert((offset + len) <= int_reg_width);
        assert(reg < count_int_regs);

        uint8_t* reg_ptr = (uint8_t*) &int_reg_storage[reg * int_reg_width];
        for(auto i = 0; i < len; ++i) {
            reg_ptr[offset + i] = values[i];
        }
    }

    void copyToFPRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert((offset + len) <= fp_reg_width);
        assert(reg < count_fp_regs);

        uint8_t* reg_ptr = (uint8_t*) &fp_reg_storage[reg * fp_reg_width];
        for(auto i = 0; i < len; ++i) {
            reg_ptr[offset + i] = values[i];
        }
    }

    template <typename T>
    T getIntReg(const uint16_t reg)
    {
        assert(reg < count_int_regs);
        assert(sizeof(T) <= int_reg_width);

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
        assert(sizeof(T) <= fp_reg_width);

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
        assert(sizeof(T) <= fp_reg_width);

        uint8_t* val_ptr = (uint8_t*) &val;

        for(auto i = 0; i < sizeof(T); ++i) {
            fp_reg_storage[fp_reg_width * reg + i] = val_ptr[i];
        }

        // Pad with extra zeros if needed
        for(auto i = sizeof(T); i < fp_reg_width; ++i) {
            fp_reg_storage[fp_reg_width * reg + i] = 0;
        }
    }

    uint32_t getHWThread() const { return hw_thread; }
    uint16_t countIntRegs() const { return count_int_regs; }
    uint16_t countFPRegs() const { return count_fp_regs; }

    void print(SST::Output* output, int level = 8 )
    {
        output->verbose(CALL_INFO, level, 0, "Integer Registers:\n");

        for ( uint16_t i = 0; i < count_int_regs; ++i ) {
            printRegister(output, true, i, level);
        }

        output->verbose(CALL_INFO, level, 0, "Floating Point Registers:\n");

        for ( uint16_t i = 0; i < count_fp_regs; ++i ) {
            printRegister(output, false, i, level);
        }
    }

private:
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

    void printRegister(SST::Output* output, bool isInt, uint16_t reg, int level = 8)
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

        for( auto i = 0; i < 64; ++i) {
            val_string[i] = '0';
        }

        for ( unsigned long long int i = 1L << (isInt ? ((int_reg_width * 8) - 1) : ((fp_reg_width * 8) - 1)); i > 0; i = i / 2 ) {
            val_string[index++] = (v & i) ? '1' : '0';
        }

        output->verbose(CALL_INFO, level, 0, "R[%5" PRIu16 "]: %s\n", reg, val_string);
        delete[] val_string;
    }

    const uint32_t               hw_thread;
    const uint16_t               count_int_regs;
    const uint16_t               count_fp_regs;
    const VanadisDecoderOptions* decoder_opts;

    char* int_reg_storage;
    char* fp_reg_storage;

    VanadisFPRegisterMode fp_reg_mode;
    const uint32_t              fp_reg_width;
    const uint32_t              int_reg_width;
};

} // namespace Vanadis
} // namespace SST

#endif
