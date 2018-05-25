// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_COMPRESSED_BINARY_TRACE_GEN
#define _H_SST_ARIEL_COMPRESSED_BINARY_TRACE_GEN

#include <climits>

#include <sst/core/params.h>
#include "zlib.h"
#include "arieltracegen.h"

namespace SST {
namespace ArielComponent {

class ArielCompressedBinaryTraceGenerator : public ArielTraceGenerator {

    public:

        SST_ELI_REGISTER_MODULE(ArielCompressedBinaryTraceGenerator, "ariel", "CompressedBinaryTraceGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0), "Provides tracing to compressed file capabilities", "SST::ArielComponent::ArielTraceGenerator")

        SST_ELI_DOCUMENT_PARAMS( { "trace_prefix", "Sets the prefix for the trace file", "ariel-core-" } )

        ArielCompressedBinaryTraceGenerator(Component* owner, Params& params);

        ~ArielCompressedBinaryTraceGenerator();

        void publishEntry(const uint64_t picoS, const uint64_t physAddr,
                const uint32_t reqLength, const ArielTraceEntryOperation op);

        void setCoreID(const uint32_t core);

    private:
        void copy(char* dest, const void* src, const size_t length);

        gzFile traceFile;
        std::string tracePrefix;
        uint32_t coreID;
        char* buffer;

};

}
}

#endif
