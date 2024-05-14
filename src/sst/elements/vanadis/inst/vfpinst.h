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

#ifndef _H_VANADIS_FP_INSTRUCTION
#define _H_VANADIS_FP_INSTRUCTION

#include <fenv.h>

#include "decoder/visaopts.h"
#include "inst/regfile.h"
#include "inst/vinst.h"
#include "inst/vinsttype.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"
#include "vfpflags.h"

#include <cstring>
#include <cmath>
#include <sst/core/output.h>

namespace SST {
namespace Vanadis {

#define READ_FP_REG \
    if ( sizeof(fp_format) > regFile->getFPRegWidth() ) { \
        src_1  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]); \
    } else { \
        src_1  = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]); \
    }

#define READ_2FP_REGS \
    if ( sizeof(fp_format) > regFile->getFPRegWidth() ) { \
        src_1  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]); \
        src_2  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]); \
    } else { \
        src_1  = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]); \
        src_2  = regFile->getFPReg<fp_format>(phys_fp_regs_in[1]); \
    }

#define READ_3FP_REGS \
    if ( sizeof(fp_format) > regFile->getFPRegWidth() ) { \
        src_1  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[0], phys_fp_regs_in[1]); \
        src_2  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[2], phys_fp_regs_in[3]); \
        src_3  = combineFromRegisters<fp_format>(regFile, phys_fp_regs_in[4], phys_fp_regs_in[5]); \
    } else { \
        src_1  = regFile->getFPReg<fp_format>(phys_fp_regs_in[0]); \
        src_2  = regFile->getFPReg<fp_format>(phys_fp_regs_in[1]); \
        src_3  = regFile->getFPReg<fp_format>(phys_fp_regs_in[2]); \
    }

#define WRITE_FP_REGS \
    if ( sizeof(fp_format) > regFile->getFPRegWidth() ) { \
        fractureToRegisters<fp_format>(regFile, phys_fp_regs_out[0], phys_fp_regs_out[1], result);\
    } else { \
        regFile->setFPReg<fp_format>(phys_fp_regs_out[0], result); \
    }

class VanadisFloatingPointInstruction : public VanadisInstruction
{
public:
    VanadisFloatingPointInstruction(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fp_flags, const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out,
        const uint16_t c_isa_int_reg_in, const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in,
        const uint16_t c_phys_fp_reg_out, const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out) :
        VanadisInstruction(
            address, hw_thr, isa_opts, c_phys_int_reg_in, c_phys_int_reg_out, c_isa_int_reg_in, c_isa_int_reg_out,
            c_phys_fp_reg_in, c_phys_fp_reg_out, c_isa_fp_reg_in, c_isa_fp_reg_out),
        pipeline_fpflags(fp_flags),
        update_fp_flags(false), set_fp_flags(false), m_set_rm(false), m_update_rm(false)
    {}

    VanadisFloatingPointInstruction(const VanadisFloatingPointInstruction& copy_me) :
        VanadisInstruction(copy_me),
        update_fp_flags(copy_me.update_fp_flags),
        set_fp_flags(copy_me.set_fp_flags),
        fpflags(copy_me.fpflags),
		pipeline_fpflags(copy_me.pipeline_fpflags),
        m_set_rm(copy_me.m_set_rm),
        m_update_rm(copy_me.m_update_rm)
    {}

    virtual bool updatesFPFlags() const override { 
        return update_fp_flags || set_fp_flags || m_set_rm | m_update_rm; 
    }

    virtual void updateFPFlags() override {
        if(LIKELY(update_fp_flags)) {
            pipeline_fpflags->update_flags(fpflags);
        } else if(UNLIKELY(set_fp_flags)) {
            pipeline_fpflags->set_flags(fpflags);
        }

        if(UNLIKELY(m_update_rm)) {
            pipeline_fpflags->update_rm(fpflags);
        } else if(UNLIKELY(m_set_rm)) {
            pipeline_fpflags->set_rm(fpflags);
        }
    }

protected:
    bool                       update_fp_flags;
    bool                       set_fp_flags;
    bool                       m_set_rm;
    bool                       m_update_rm;

	VanadisFloatingPointFlags* pipeline_fpflags;
    VanadisFloatingPointFlags  fpflags;

	template<typename T>
    void performFlagChecks(const T value) {
		if(std::fpclassify(value) == FP_INFINITE) {
			fpflags.setOverflow();
			update_fp_flags = true;
		}

		if(std::fpclassify(value) == FP_SUBNORMAL) {
			fpflags.setUnderflow();
			update_fp_flags = true;
		}
    }

    template<bool SetFRM, bool SetFFLAGS>
    void updateFP_flags( uint32_t value, int mode ) {
        if constexpr( true == SetFFLAGS ) {
            set_fflags( value & 0x1f, mode );
            if constexpr( true == SetFRM) {
                set_rm( ( value >> 5 ) & 0x7, mode );
            }
        } else if constexpr( true == SetFRM ) {
            set_rm( value, mode );
        }
    }

    
  private:
    std::string getModeStr(  int mode ) {
        switch( mode ) {
            case 1: return "Write";
            case 2: return "Set";
            case 3: return "Clear";
            default: assert(0);
        }
    }
    void set_fflags( uint32_t flags, int mode ) {

        if ( 0x3 == ( mode & 0x3 ) ) {
            fpflags.set_flags(*pipeline_fpflags);
        }

        if ( (flags & 0x1) != 0 ) {
            if ( 0x3 == ( mode & 0x3 ) ) {
                fpflags.clearInexact();
            } else {
                fpflags.setInexact();
            }
        }

        if ( (flags & 0x2) != 0 ) {
            if ( 0x3 == ( mode & 0x3 ) ) {
                fpflags.clearUnderflow();
            } else {
                fpflags.setUnderflow();
            }
        }

        if ( (flags & 0x4) != 0 ) {
            if ( 0x3 == ( mode & 0x3 ) ) {
                fpflags.clearOverflow();
            } else {
                fpflags.setOverflow();
            }
        }

        if ( (flags & 0x8) != 0 ) {
            if ( 0x3 == ( mode & 0x3 ) ) {
                fpflags.clearDivZero();
            } else {
                fpflags.setDivZero();
            }
        }

        if ( (flags & 0x10) != 0 ) {
            if ( 0x3 == ( mode & 0x3 ) ) {
                fpflags.clearInvalidOp();
            } else {
                fpflags.setInvalidOp();
            }
        }

        if ( 0x2 == mode  ) {
            update_fp_flags = true;
        } else {
            set_fp_flags = true;
        }
    }

    void set_rm( uint32_t rm, int mode ) {
        fpflags.setRoundingMode( (VanadisFPRoundingMode)( rm ) );

        if ( 0x2 == mode  ) {
            m_update_rm = true;
        } else {
            m_set_rm = true;
        }
    }

  protected:

    void check_IEEE754_except() {
        int raised = fetestexcept (FE_INEXACT | FE_DIVBYZERO | FE_UNDERFLOW | FE_OVERFLOW | FE_INVALID );

        if (raised & FE_INEXACT) {
            fpflags.setInexact();
            update_fp_flags = true;
        }
        if (raised & FE_DIVBYZERO) {
            fpflags.setDivZero();
            update_fp_flags = true;
        }
        if (raised & FE_UNDERFLOW) {
            fpflags.setUnderflow();
            update_fp_flags = true;
        }
        if (raised & FE_OVERFLOW) {
            fpflags.setOverflow();
            update_fp_flags = true;
        }
        if (raised & FE_INVALID) {
            fpflags.setInvalidOp();
            update_fp_flags = true;
        }
     }

    void clear_IEEE754_except() {
        feclearexcept(FE_ALL_EXCEPT);
    }

    /*
     * These values come the RISCV tests it's the values that the test what
     * qNaNh 0x7e00
     * sNaNh 0x7c01
     * qNaNf 0x7fc00000
     * sNaNf 0x7f800001
     * qNaN 0x7ff8000000000000
     * sNaN 0x7ff0000000000001
    */

    static const uint64_t IEEE754_EXP64 =  0x7ff0000000000000;
    static const uint64_t IEEE754_FRC64 =  0x000fffffffffffff;
    static const uint64_t IEEE754_FRC64S = 0x0008000000000000;
    static const uint32_t IEEE754_EXP32 =  0x7f800000;
    static const uint32_t IEEE754_FRC32 =  0x007fffff;
    static const uint32_t IEEE754_FRC32S = 0x00400000;


    template <typename T>
    bool isNaN_boxed( T src ) {
        if constexpr (std::is_same_v<T,uint64_t>) {
            if ( src == 0 ) return true;
            return ((src & 0xffffffff00000000) == 0xffffffff00000000); 
        } else {
            assert(0);
        }
    }

    template <typename T>
    bool isSignBitSet( T src ) {
        if constexpr (std::is_same_v<T,uint64_t>) {
            return (src & ( (T) 1 << 63)); 
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return (src & ( (T) 1 << 31)); 
        } else {
            assert(0);
        }
    }

    template <typename T>
    T setSignBit( T src ) {
        if constexpr (std::is_same_v<T,uint64_t>) {
            return src | ( (T) 1 << 63); 
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return src | ( (T) 1 << 31); 
        } else {
            assert(0);
        }
    }

    template <typename T>
    T clearSignBit( T src ) {
        if constexpr (std::is_same_v<T,uint64_t>) {
            return src & ~( (T) 1 << 63); 
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return src & ~( (T) 1 << 31); 
        } else {
            assert(0);
        }
    }

    template <typename T>
    T int64To( uint64_t src ) {
        return *(T*) &src;
    }

    template <typename T1, typename T2 >
    T1 convertTo( T2 src ) {
        return *(T1*) &src;
    }

    template <typename T>
    T NaN( ) {
        if constexpr (std::is_same_v<T,double> ) {
            uint64_t tmp = 0x7ff8000000000000;
            return *(T*) &tmp; 
        } else if constexpr (std::is_same_v<T,float>) {
            uint32_t tmp = 0x7fc00000; 
            return *(T*) &tmp;
        } else if constexpr ( std::is_same_v<T,uint64_t>) {
            return 0x7ff8000000000000;
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return 0x7fc00000; 
        } else {
            assert(0);
        }
    }

    template <typename T>
    bool isInf( T v ) {
        if constexpr (std::is_same_v<T,double>) {
            auto tmp = *(uint64_t*) & v;
            return ( tmp == IEEE754_EXP64 );
        } else if constexpr (std::is_same_v<T,uint64_t>) {
            return ( v == IEEE754_EXP64 );
        } else if constexpr (std::is_same_v<T,float>) {
            auto tmp = *(uint32_t*) & v;
            return ( tmp == IEEE754_EXP32 );
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return ( v == IEEE754_EXP32 );
        } else {
            assert(0);
        }
    }

    template <typename T>
    bool isNaN( T v ) {
        if constexpr (std::is_same_v<T,double>) {
            auto tmp = *(uint64_t*) & v;
            return ( (tmp & IEEE754_EXP64) == IEEE754_EXP64 ) && (tmp & IEEE754_FRC64);
        } else if constexpr (std::is_same_v<T,float>) {
            auto tmp = *(uint32_t*) & v;
            return ( (tmp & IEEE754_EXP32) == IEEE754_EXP32 ) && (tmp & IEEE754_FRC32);
        } else if constexpr (std::is_same_v<T,int64_t> || std::is_same_v<T,uint64_t>) {
            return ( (v & IEEE754_EXP64) == IEEE754_EXP64 ) && (v & IEEE754_FRC64);
        } else if constexpr (std::is_same_v<T,int32_t> || std::is_same_v<T,uint32_t>) {
            return ( (v & IEEE754_EXP32) == IEEE754_EXP32 ) && (v & IEEE754_FRC32);
        } else {
            assert(0);
        }
    }

    template <typename T>
    bool isNaNs( T v ) {
        if constexpr (std::is_same_v<T,double>) {
            auto tmp = *(uint64_t*) & v;
            return ( (tmp & IEEE754_EXP64) == IEEE754_EXP64 ) && (tmp & IEEE754_FRC64) && ( (tmp & IEEE754_FRC64S) == 0 );
        } else if constexpr (std::is_same_v<T,float>) {
            auto tmp = *(uint32_t*) & v;
            return ( (tmp & IEEE754_EXP32) == IEEE754_EXP32 ) && (tmp & IEEE754_FRC32) && ( (tmp & IEEE754_FRC32S) == 0 );
        } else {
            assert(0);
        }
    }
    
    template <typename T>
    T getMaxInt() {
        if constexpr (std::is_same_v<T,int32_t>) {
            return ~(1 << 31);
        } else if constexpr (std::is_same_v<T,int64_t>) {
            return ~((uint64_t) 1 << 63);
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return ~0;
        } else if constexpr (std::is_same_v<T,uint64_t>) {
            return ~0L;
        } else {
            assert(0);
        }
    }
    template <typename T>
    T getMinInt() {
        if constexpr (std::is_same_v<T,int32_t>) {
            return (uint32_t) 1 << 31;
        } else if constexpr (std::is_same_v<T,int64_t>) {
            return (uint64_t) 1 << 63;
        } else if constexpr (std::is_same_v<T,uint32_t>) {
            return 0;
        } else if constexpr (std::is_same_v<T,uint64_t>) {
            return 0;
        } else {
            assert(0);
        }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
