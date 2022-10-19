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

#ifndef _H_VANADIS_FPFLAGS
#define _H_VANADIS_FPFLAGS

namespace SST {
namespace Vanadis {

enum class VanadisFPRoundingMode { ROUND_NEAREST, ROUND_TO_ZERO, ROUND_DOWN, ROUND_UP, ROUND_NEAREST_TO_MAX };

uint64_t convertRoundingToInteger(VanadisFPRoundingMode mode) {
	switch(mode) {
	case VanadisFPRoundingMode::ROUND_NEAREST:
		return 0;
	case VanadisFPRoundingMode::ROUND_TO_ZERO:
		return 1;
	case VanadisFPRoundingMode::ROUND_DOWN:
		return 2;
	case VanadisFPRoundingMode::ROUND_UP:
		return 3;
	case VanadisFPRoundingMode::ROUND_NEAREST_TO_MAX:
		return 4;
	default:
		return 0;
	}
};

class VanadisFloatingPointFlags
{

public:
    VanadisFloatingPointFlags() :
        f_invalidop(false),
        f_divzero(false),
        f_overflow(false),
        f_underflow(false),
        f_inexact(false),
        round_mode(VanadisFPRoundingMode::ROUND_NEAREST)
    {}

    void update(const VanadisFloatingPointFlags& new_flags) {
		f_invalidop = f_invalidop ? true : new_flags.f_invalidop;
		f_divzero = f_divzero ? true : new_flags.f_divzero;
		f_overflow = f_overflow ? true : new_flags.f_overflow;
		f_underflow = f_underflow ? true : new_flags.f_underflow;
		f_inexact = f_inexact ? true : new_flags.f_inexact;
		round_mode = new_flags.round_mode;
    }

    void set(const VanadisFloatingPointFlags& new_flags) {
		f_invalidop = new_flags.f_invalidop;
		f_divzero = new_flags.f_divzero;
		f_overflow = new_flags.f_overflow;
		f_underflow = new_flags.f_underflow;
		f_inexact = new_flags.f_inexact;
		round_mode = new_flags.round_mode;
    }

    void clear() {
        f_invalidop = false;
        f_divzero = false;
        f_overflow = false;
        f_underflow = false;
        f_inexact = false;
    }

    void setInvalidOp() { f_invalidop = true; }
    void setDivZero() { f_divzero = true; }
    void setOverflow() { f_overflow = true; }
    void setUnderflow() { f_underflow = true; }
    void setInexact() { f_inexact = true; }
    void setRoundingMode(VanadisFPRoundingMode newMode) { round_mode = newMode; }

    void clearInvalidOp() { f_invalidop = false; }
    void clearDivZero() { f_divzero = false; }
    void clearOverflow() { f_overflow = false; }
    void clearUnderflow() { f_underflow = false; }
    void clearInexact() { f_inexact = false; }
    void clearRoundingMode() { round_mode = VanadisFPRoundingMode::ROUND_NEAREST; }

    bool                  invalidOp() const { return f_invalidop; }
    bool                  divZero() const { return f_divzero; }
    bool                  overflow() const { return f_overflow; }
    bool                  underflow() const { return f_underflow; }
    bool                  inexact() const { return f_inexact; }
    VanadisFPRoundingMode getRoundingMode() const { return round_mode; }

	void print(SST::Output* output) {
		output->verbose(CALL_INFO, 16, 0, "-> FP Status: IVLD: %c / DIV0: %c / OF: %c / UF: %c / INXCT: %c\n",
			f_invalidop ? 'y' : 'n',
			f_divzero ? 'y' : 'n',
			f_overflow ? 'y' : 'n',
			f_underflow ? 'y' : 'n',
			f_inexact ? 'y' : 'n');
	 }

protected:
    bool f_invalidop;
    bool f_divzero;
    bool f_overflow;
    bool f_underflow;
    bool f_inexact;

    VanadisFPRoundingMode round_mode;
};

} // namespace Vanadis
} // namespace SST

#endif
