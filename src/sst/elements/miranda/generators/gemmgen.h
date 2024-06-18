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

// Lightmatter Inc 2024

#ifndef _H_SST_MIRANDA_GEMM_BENCH_GEN
#define _H_SST_MIRANDA_GEMM_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST
{
    namespace Miranda
    {

        class GEMMGenerator : public RequestGenerator
        {
        public:
            GEMMGenerator(ComponentId_t id, Params &params);
            void build(Params &params);
            ~GEMMGenerator();

            void generate(MirandaRequestQueue<GeneratorRequest *> *q);
            void generateBlock(MirandaRequestQueue<GeneratorRequest *> *q, uint64_t a_ptr, uint64_t b_ptr, uint64_t c_ptr, uint64_t blockM, uint64_t blockN, uint64_t blockK);

            bool isFinished()
            {
                return (0 == iterations);
            }
            void completed() {}

            SST_ELI_REGISTER_SUBCOMPONENT(
                GEMMGenerator,
                "miranda",
                "GEMMGenerator",
                SST_ELI_ELEMENT_VERSION(1, 0, 0),
                "Creates a blocked matrix multiplication access pattern",
                SST::Miranda::RequestGenerator)

            SST_ELI_DOCUMENT_PARAMS(
                {"M", "Sets the horizontal dimension of the matrix: M x K", "64"},
                {"N", "Sets the vertical dimension of the matrix (M x N) )", "64"},
                {"K", "Sets the horizontal dimension of the matrix (K x N)", "32"},
                {"BLOCK_SIZE_M", "Sets the block size on dimension of M", "8"},
                {"BLOCK_SIZE_N", "Sets the block size on dimension of N", "8"},
                {"BLOCK_SIZE_K", "Sets the block size on dimension of K", "4"},
                {"stride_am", "Sets the stride size on dimension of M (increment of dimension M)", "K"},
                {"stride_ak", "Sets the stride size on dimension of K (increment of dimension K)", "1"},
                {"stride_bk", "Sets the stride size on dimension of K (increment of dimension M)", "N"},
                {"stride_bn", "Sets the stride size on dimension of N (increment of dimension M)", "1"},
                {"stride_cm", "Sets the stride size on dimension of M (increment of dimension M)", "N"},
                {"stride_cn", "Sets the stride size on dimension of N (increment of dimension M)", "1"},
                {"element_width", "Sets the width of one matrix element, typically 8 for a double", "8"},
                {"start_addr", "Sets the start address of all matrix elements", "0"},
                {"a_ptr", "Sets the start address of the matrix A", "0"},
                {"b_ptr", "Sets the start address of the matrix A", "consecutive"},
                {"c_ptr", "Sets the start address of the matrix A", "consecutive"},
                {"num_threads", "Sets the total number of threads", "1"},
                {"thread_id", "Sets the current thread id", "0"},
                {"iterations", "Sets the number of repeats to perform"}, )
        private:
            Output *out;

            uint64_t iterations;
            uint64_t M, N, K;                                  // A:[M x K] * B:[N x K] = C:[M x N], assuming B <= B.T for better performance
            uint64_t BLOCK_SIZE_M, BLOCK_SIZE_N, BLOCK_SIZE_K; // block sizes
            // The stride variables represent how much to increase the ptr by when moving by 1
            // element in a particular dimension. E.g. `stride_am` is how much to increase `a_ptr`
            // by to get the element one row down (A has M rows).
            uint64_t stride_am, stride_ak, stride_bk, stride_bn, stride_cm, stride_cn;
            uint64_t elementWidth;
            // starting addresses
            uint64_t start_addr;
            uint64_t a_ptr, b_ptr, c_ptr;
            uint32_t num_threads, thread_id; // total number of threads, and the current thread id for parallel execution
        };

    }
}

#endif
