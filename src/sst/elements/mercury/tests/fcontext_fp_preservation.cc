// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#define ssthg_app_name fcontext_fp_preservation

#include <cstdio>
#include <cstdlib>

#include <mercury/components/operating_system_impl.h>
#include <mercury/common/skeleton.h>

using namespace SST::Hg;

namespace {

#if defined(__aarch64__)
__attribute__((noinline)) bool
registersSurviveSleep(double base, unsigned int sleepNs)
{
    register double value8 asm("d8") = base + 8.0;
    register double value9 asm("d9") = base + 9.0;
    register double value10 asm("d10") = base + 10.0;
    register double value11 asm("d11") = base + 11.0;
    register double value12 asm("d12") = base + 12.0;
    register double value13 asm("d13") = base + 13.0;
    register double value14 asm("d14") = base + 14.0;
    register double value15 asm("d15") = base + 15.0;

    // Keep every AAPCS64 callee-saved FP register live across the context switch.
    asm volatile("" : "+w"(value8), "+w"(value9), "+w"(value10),
        "+w"(value11), "+w"(value12), "+w"(value13), "+w"(value14),
        "+w"(value15));
    ssthg_nanosleep(sleepNs);
    asm volatile("" : "+w"(value8), "+w"(value9), "+w"(value10),
        "+w"(value11), "+w"(value12), "+w"(value13), "+w"(value14),
        "+w"(value15));

    return value8 == base + 8.0 && value9 == base + 9.0 &&
        value10 == base + 10.0 && value11 == base + 11.0 &&
        value12 == base + 12.0 && value13 == base + 13.0 &&
        value14 == base + 14.0 && value15 == base + 15.0;
}
#endif

} // namespace

int
main(int, char**)
{
#if defined(__aarch64__)
    const int rank = static_cast<int>(OperatingSystemImpl::currentOs()->addr());

    // Rank 0 holds one register set until t=100 ns. Rank 1 installs a distinct
    // set at t=10 ns and yields until after rank 0 resumes. Without per-context
    // FP state, rank 0 deterministically observes rank 1's values.
    if ( rank == 1 ) ssthg_nanosleep(10);
    const bool pass = registersSurviveSleep(
        rank == 0 ? 100.0 : 200.0, rank == 0 ? 100 : 200);

    std::printf("Mercury fcontext FP preservation rank %d %s\n",
        rank, pass ? "PASS" : "FAIL");
    std::fflush(stdout);
    if ( !pass ) std::abort();
#endif
    return 0;
}
