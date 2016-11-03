// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_SPMV_BENCH_GEN
#define _H_SST_MIRANDA_SPMV_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class SPMVGenerator : public RequestGenerator {

public:
	SPMVGenerator( Component* owner, Params& params ) : RequestGenerator(owner, params) {
		const uint32_t verbose = params.find<uint32_t>("verbose", 0);
		out = new Output("SPMVGenerator[@p:@l]: ", verbose, 0, Output::STDOUT);

		matrixNx        = params.find<uint64_t>("matrix_nx", 10);
		matrixNy        = params.find<uint64_t>("matrix_ny", 10);

		elementWidth    = params.find<uint64_t>("element_width", 8);

		uint64_t nextStartAddr = 0;

		lhsVecStartAddr = params.find<uint64_t>("lhs_start_addr", nextStartAddr);
		nextStartAddr += matrixNx * elementWidth;

		rhsVecStartAddr = params.find<uint64_t>("rhs_start_addr", nextStartAddr);
		nextStartAddr += matrixNx * elementWidth;

		localRowStart   = params.find<uint64_t>("local_row_start", 0);
		localRowEnd     = params.find<uint64_t>("local_row_end", matrixNy);

		ordinalWidth    = params.find<uint64_t>("ordinal_width", 8);
		matrixNNZPerRow = params.find<uint64_t>("matrix_nnz_per_row", 4);

		matrixRowIndicesStartAddr = params.find<uint64_t>("matrix_row_indices_start_addr", nextStartAddr);
		nextStartAddr  += (ordinalWidth * (matrixNy + 1));

		matrixColumnIndicesStartAddr = params.find<uint64_t>("matrix_col_indices_start_addr", nextStartAddr);
		nextStartAddr  += (matrixNy * ordinalWidth * matrixNNZPerRow);

		matrixElementsStartAddr = params.find<uint64_t>("matrix_element_start_addr", nextStartAddr);

		iterations = params.find<uint64_t>("iterations", 1);
	}

	~SPMVGenerator() {
		delete out;
	}

	void generate(MirandaRequestQueue<GeneratorRequest*>* q) {
		for(uint64_t row = localRowStart; row < localRowEnd; row++) {
			out->verbose(CALL_INFO, 2, 0, "Generating access for row %" PRIu64 "\n", row);

			MemoryOpRequest* readStart = new MemoryOpRequest(matrixRowIndicesStartAddr + (ordinalWidth * row), ordinalWidth, READ);
			MemoryOpRequest* readEnd   = new MemoryOpRequest(matrixRowIndicesStartAddr + (ordinalWidth * (row + 1)), ordinalWidth, READ);

			q->push_back(readStart);
			q->push_back(readEnd);

			MemoryOpRequest* readResultCurrentValue = new MemoryOpRequest(rhsVecStartAddr +
				(row * matrixNNZPerRow), elementWidth, WRITE);
			MemoryOpRequest* writeResult = new MemoryOpRequest(rhsVecStartAddr +
				(row * matrixNNZPerRow), elementWidth, WRITE);

			writeResult->addDependency(readResultCurrentValue->getRequestID());

			q->push_back(readResultCurrentValue);

			for(uint64_t col = row; col < (row + matrixNNZPerRow); col++) {
				if(col >= matrixNx) {
					break;
				}

				out->verbose(CALL_INFO, 4, 0, "Generating access for row %" PRIu64 ", column: %" PRIu64 "\n",
					row, col);

				MemoryOpRequest* readMatElement = new MemoryOpRequest(matrixElementsStartAddr +
					(row * matrixNNZPerRow + col) * elementWidth, elementWidth, READ);
				MemoryOpRequest* readCol = new MemoryOpRequest(matrixColumnIndicesStartAddr +
					(row * matrixNNZPerRow + col) * ordinalWidth, ordinalWidth, READ);
				MemoryOpRequest* readLHSElem = new MemoryOpRequest(lhsVecStartAddr +
					(row * matrixNNZPerRow + col) * elementWidth, elementWidth, READ);

				readCol->addDependency(readStart->getRequestID());
				readCol->addDependency(readEnd->getRequestID());

				readLHSElem->addDependency(readCol->getRequestID());
				writeResult->addDependency(readLHSElem->getRequestID());
				writeResult->addDependency(readMatElement->getRequestID());

				q->push_back(readCol);
				q->push_back(readMatElement);
				q->push_back(readLHSElem);
			}

			q->push_back(writeResult);
		}

		iterations--;
	}

	bool isFinished() {
		return (0 == iterations);
	}

	void completed() {}

private:
	Output*  out;

	uint64_t iterations;
	uint64_t matrixNx;
	uint64_t matrixNy;
	uint64_t elementWidth;
	uint64_t lhsVecStartAddr;
	uint64_t rhsVecStartAddr;
	uint64_t ordinalWidth;
	uint64_t matrixNNZPerRow;
	uint64_t matrixRowIndicesStartAddr;
	uint64_t localRowStart;
	uint64_t localRowEnd;
	uint64_t matrixColumnIndicesStartAddr;
	uint64_t matrixElementsStartAddr;

};

}
}

#endif
