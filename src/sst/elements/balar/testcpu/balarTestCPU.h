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

#ifndef BALAR_BALAR_TEST_CPU_H
#define BALAR_BALAR_TEST_CPU_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/rng/marsaglia.h>

#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "cuda.h"

#include <sst/elements/memHierarchy/util.h>
#include "util.h"
#include <iostream>
#include <fstream>
#include <queue>

using namespace SST::Statistics;
using namespace SST::MemHierarchy;
namespace SST {
namespace BalarComponent { 
using Req = SST::Interfaces::StandardMem::Request;

class BalarTestCPU : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(BalarTestCPU, "balar", "BalarTestCPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple demo CPU for testing balar", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "1"},
        {"clock",                   "(UnitAlgebra/string) Clock frequency", "1GHz"},
        {"gpu_addr",                "(uint) Base address of the BalarMMIO component. 0 means not present.", "0"},
        {"scratch_mem_addr",        "(uint) Base address of the scratch memory to pass balarMMIO packets", "0"},
        {"trace_file",              "(string) CUDA API calls trace file path"},
        {"cuda_executable",         "(string) CUDA executable file path to extract PTX info"},
        {"enable_memcpy_dump",      "(bool) Enable memD2Hcpy dump or not", "false"} )

    SST_ELI_DOCUMENT_STATISTICS( 
        {"total_memD2H_bytes", "Number of total memD2Hcpy bytes", "count", 1},
        {"correct_memD2H_bytes", "Number of total correct memD2Hcpy bytes", "count", 1},
        {"correct_memD2H_ratio", "Ratio of correct/total memD2Hcpy bytes for each individual memD2Hcpy call", "count", 1},
    )

    /* Slot for a memory interface. This must be user defined (aka defined in Python config) */
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( { "memory", "Interface to memory hierarchy", "SST::Interfaces::StandardMem" } )

/* Begin class definition */
    BalarTestCPU(SST::ComponentId_t id, SST::Params& params);
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void emergencyShutdown() override;

private:
    void handleEvent( Interfaces::StandardMem::Request *ev );
    virtual bool clockTic( SST::Cycle_t );

    Output out;
    uint64_t scratchMemAddr;
    uint64_t gpuAddr;
    uint64_t clock_ticks;
    bool enable_memcpy_dump;
    Statistic<uint64_t>* total_memD2H_bytes;
    Statistic<uint64_t>* correct_memD2H_bytes;
    Statistic<double>* correct_memD2H_ratio;

    // Pending requests
    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;

    Interfaces::StandardMem *memory;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

    // Create request to call CUDA API
    Interfaces::StandardMem::Request* createGPUReqFromPacket(BalarCudaCallPacket_t pack);

    // Create a handler class here to handle incoming requests
    // Like the one for the balarMMIO
    // In charge of initiate CUDA api calls
    class mmioHandlers : public Interfaces::StandardMem::RequestHandler {
        public:
            friend class BalarTestCPU;
            mmioHandlers(BalarTestCPU* cpu, SST::Output* out) : Interfaces::StandardMem::RequestHandler(out), cpu(cpu) {}

            virtual ~mmioHandlers() {}
            // Only need to handle read and write response
            // virtual void handle(StandardMem::Read* read) override;
            // virtual void handle(StandardMem::Write* write) override;
            virtual void handle(Interfaces::StandardMem::ReadResp* resp) override;
            virtual void handle(Interfaces::StandardMem::WriteResp* resp) override;
            void UInt64ToData(uint64_t num, vector<uint8_t>* data) {
                data->clear();
                for (size_t i = 0; i < sizeof(uint64_t); i++) {
                    data->push_back(num & 0xFF);
                    num >>=8;
                }
            }
            uint64_t dataToUInt64(std::vector<uint8_t>* data) {
                uint64_t retval = 0;
                for (int i = data->size(); i > 0; i--) {
                    retval <<= 8;
                    retval |= (*data)[i-1];
                }
                return retval;
            }

            BalarTestCPU* cpu;
    };
    
    mmioHandlers* gpuHandler;

    // Trace parser
    class CudaAPITraceParser {
        public:
            friend class BalarTestCPU;
            CudaAPITraceParser(BalarTestCPU* cpu, SST::Output* out, std::string& traceFile, std::string& cudaExecutable);
            virtual ~CudaAPITraceParser() {}

            Interfaces::StandardMem::Request* getNextCall();

            BalarTestCPU* cpu;
            SST::Output* out;
            std::string cudaExecutable;
            std::string traceFileBasePath;
            std::ifstream traceStream;
            std::queue<Interfaces::StandardMem::Request*>* initReqs;

            /**
             * @brief A map of pointers to device pointer
             * 
             */
            std::map<std::string, CUdeviceptr*>* dptr_map;

            /**
             * @brief A map of kernel function name and pointer
             *        used for registering functions in GPGPUSIM
             * 
             */
            std::map<std::string, uint64_t>* func_map;


            // TODO Only on cubin here?
            unsigned fatCubinHandle;
    };
    CudaAPITraceParser* trace_parser;
};

}
}
#endif /* BALAR_BALAR_TEST_CPU_H */
