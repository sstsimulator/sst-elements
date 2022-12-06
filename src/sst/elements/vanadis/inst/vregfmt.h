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

#ifndef _H_VANADIS_REG_FORMAT
#define _H_VANADIS_REG_FORMAT

namespace SST {
namespace Vanadis {

enum class VanadisRegisterFormat {
    VANADIS_FORMAT_FP32,
    VANADIS_FORMAT_FP64,
    VANADIS_FORMAT_INT32,
    VANADIS_FORMAT_INT64
};

const char*
registerFormatToString(const VanadisRegisterFormat fmt)
{
    switch ( fmt ) {
    case VanadisRegisterFormat::VANADIS_FORMAT_FP32:
        return "F32";
    case VanadisRegisterFormat::VANADIS_FORMAT_FP64:
        return "F64";
    case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        return "I32";
    case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        return "I64";
    default:
        return "UNK";
    }
};

} // namespace Vanadis
} // namespace SST

#endif
