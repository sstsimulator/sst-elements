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

#ifndef _CROSSSIMINTMVMARRAY_H
#define _CROSSSIMINTMVMARRAY_H

#include <sst/core/component.h>
#include <sst/elements/golem/array/crossSimComputeArray.h>

namespace SST {
namespace Golem {

class CrossSimIntArray : public CrossSimComputeArray<int64_t> {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        CrossSimIntArray,
        "golem",
        "CrossSimIntArray",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Implements a Compute array using manual MVM with int64_ting-point representation",
        SST::Golem::CrossSimComputeArray<int64_t>
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"arrayLatency",       "Latency of array computation, including all data conversion latencies", "100ns" },
        {"verbose",            "Set the verbosity of output for the component", "0" },
        {"max_instructions",   "Set the maximum number of instructions permitted in the queue", "8" },
        {"clock",              "Clock frequency for component TimeConverter", "1GHz"},
        {"mmioAddr",           "Address of MMIO interface"},
        {"numArrays",          "Number of distinct arrays in the tile", "1"},
        {"arrayInputSize",     "Length of input vector (implies array rows)"},
        {"arrayOutputSize",    "Length of output vector (implies array columns)"},
        {"inputOperandSize",   "Number of bytes in a single input value"},
        {"outputOperandSize",  "Number of bytes in a single output value"},
        {"CrossSimJSONParameters",  "Path to CrossSim JSON parameters file."},
    )

    CrossSimIntArray(ComponentId_t id, Params& params,
        TimeConverter* tc,
        Event::HandlerBase* handler)
        : CrossSimComputeArray<int64_t>(id, params, tc, handler) {
        // Constructor can be empty if no additional initialization is required
    }
};

} // namespace Golem
} // namespace SST

#endif
