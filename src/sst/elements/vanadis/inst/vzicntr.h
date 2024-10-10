
#ifndef _H_VANADIS_ZICNTR
#define _H_VANADIS_ZICNTR

#include <utility>

namespace SST::Vanadis
{
    // Tags to be used to with the regfile and VanadisReadCounterInstruction
    namespace Zicntr {
        inline constexpr std::integral_constant<uint32_t, 0> CYCLE;
        inline constexpr std::integral_constant<uint32_t, 1> TIME;
        inline constexpr std::integral_constant<uint32_t, 2> INSTRET;
    }
} // namespace Vanadis::SST

#endif
