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

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/elements/miranda/generators/gemmgen.h>
#include <sst/core/output.h>

#include <queue>

// using namespace SST::RNG;
using namespace SST::Miranda;

uint64_t getAddr(uint64_t a_ptr, uint64_t i, uint64_t j, uint64_t stride_y, uint64_t stride_x = 1)
{
	return (a_ptr + (i * stride_y) + (j * stride_x));
}

GEMMGenerator::GEMMGenerator(ComponentId_t id, Params &params) : RequestGenerator(id, params)
{
	build(params);
}

void GEMMGenerator::build(Params &params)
{
	const uint32_t verbose = params.find<uint32_t>("verbose", 0);
	out = new Output("GEMMGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

	// A:[M x K] * B:[K x N] = C:[M x N]
	M = params.find<uint64_t>("matrix_M", 64);
	N = params.find<uint64_t>("matrix_N", 64);
	K = params.find<uint64_t>("matrix_K", 32);

	BLOCK_SIZE_M = params.find<uint64_t>("BLOCK_SIZE_M", 8);
	BLOCK_SIZE_N = params.find<uint64_t>("BLOCK_SIZE_N", 8);
	BLOCK_SIZE_K = params.find<uint64_t>("BLOCK_SIZE_K", 4);

	elementWidth = params.find<uint64_t>("element_width", sizeof(float));

	stride_am = K;
	stride_ak = 1;
	stride_bn = K;
	stride_bk = 1;
	stride_cm = N;
	stride_cn = 1;

	thread_id = params.find<uint64_t>("thread_id", 0);
	num_threads = params.find<uint64_t>("num_threads", 1);

	start_addr = params.find<uint64_t>("start_addr", 0);
	uint64_t nextStartAddr = start_addr;
	out->verbose(CALL_INFO, 2, 0, "starting address: %" PRIu64 "\n", start_addr);

	// Matrix A has MxK elements of size elementWidth
	a_ptr = params.find<uint64_t>("a_ptr", nextStartAddr);
	nextStartAddr += M * K * elementWidth;

	// Matrix B has KxN elements of size elementWidth
	b_ptr = params.find<uint64_t>("b_ptr", nextStartAddr);
	nextStartAddr += K * N * elementWidth;

	// Matrix C has MxN elements of size elementWidth
	c_ptr = params.find<uint64_t>("c_ptr", nextStartAddr);
	nextStartAddr += M * N * elementWidth;

	iterations = params.find<uint64_t>("iterations", 1);
}

GEMMGenerator::~GEMMGenerator()
{
	delete out;
}

void GEMMGenerator::generate(MirandaRequestQueue<GeneratorRequest *> *q)
{
	uint64_t start_i = thread_id * M / num_threads;
	uint64_t end_i = (thread_id + 1) * M / num_threads;
	if (thread_id == num_threads - 1) // last thread
		end_i = M;
	for (uint64_t i = start_i; i < end_i; i += BLOCK_SIZE_M)
	{
		out->verbose(CALL_INFO, 2, 0, "Thread %" PRIu32 " generating access for row %" PRIu64 "\n", thread_id, i);
		for (uint64_t j = 0; j < N; j += BLOCK_SIZE_N)
		{
			out->verbose(CALL_INFO, 4, 0, "Generating access for row %" PRIu64 ", column: %" PRIu64 "\n",
						 i, j);
			auto block_c_addr = getAddr(c_ptr, i, j, stride_cm, stride_cn);
			for (uint64_t k = 0; k < K; k += BLOCK_SIZE_K)
			{
				auto block_a_addr = getAddr(a_ptr, i, k, stride_am, stride_ak);
				auto block_b_addr = getAddr(b_ptr, j, k, stride_bn, stride_bk);
				generateBlock(q, block_a_addr, block_b_addr, block_c_addr, BLOCK_SIZE_M, BLOCK_SIZE_N, BLOCK_SIZE_K);
			}
		}
	}
	iterations--;
}

void GEMMGenerator::generateBlock(MirandaRequestQueue<GeneratorRequest *> *q, uint64_t a_ptr, uint64_t b_ptr, uint64_t c_ptr, uint64_t blockM, uint64_t blockN, uint64_t blockK)
{
	for (uint64_t i = 0; i < blockM; i++)
	{
		for (uint64_t j = 0; j < blockN; j++)
		{
			auto a_start_addr = getAddr(a_ptr, i, 0, blockK, blockM);
			;
			auto b_start_addr = getAddr(b_ptr, j, 0, blockK, blockN);
			;
			auto c_start_addr = getAddr(c_ptr, i, j, blockN, blockM);

			MemoryOpRequest *readA = new MemoryOpRequest(a_start_addr, blockK * elementWidth, READ);
			MemoryOpRequest *readB = new MemoryOpRequest(b_start_addr, blockK * elementWidth, READ);
			q->push_back(readA);
			q->push_back(readB);

			// for (uint64_t k = 0; k < blockK; k++){  // sum up K elements and write to a single address
			MemoryOpRequest *writeC = new MemoryOpRequest(c_start_addr, elementWidth, WRITE);
			writeC->addDependency(readA->getRequestID());
			writeC->addDependency(readB->getRequestID());
			q->push_back(writeC);
			//}
		}
	}
}
