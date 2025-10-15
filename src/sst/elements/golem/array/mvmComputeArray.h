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

#ifndef _MVMCOMPUTEARRAY_H
#define _MVMCOMPUTEARRAY_H

#include <cstdint>
#include <vector>
#include <sst/elements/golem/array/computeArray.h>
#include <type_traits>

namespace SST {
namespace Golem {

template<typename T>
class MVMComputeArray : public ComputeArray {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(
            MVMComputeArray<T>,
            SST::Golem::ComputeArray,
            TimeConverter*,
            Event::HandlerBase*
    )

    MVMComputeArray(ComponentId_t id, Params& params,
                         TimeConverter* tc,
                         Event::HandlerBase* handler)
        : ComputeArray(id, params, tc, handler) {
        // Configure selfLink
        selfLink = configureSelfLink("Self", *tc, new Event::Handler2<MVMComputeArray,&MVMComputeArray::handleSelfEvent>(this));
        selfLink->setDefaultTimeBase(*latencyTC);

        // Initialize vectors
        inputVectors.resize(numArrays);
        outputVectors.resize(numArrays);
        matrixData.resize(numArrays);
        for (uint32_t i = 0; i < numArrays; i++) {
            inputVectors[i].resize(inputArraySize, T());
            outputVectors[i].resize(outputArraySize, T());
            matrixData[i].resize(inputArraySize * outputArraySize, T());
        }
    }

    virtual void beginComputation(uint32_t arrayID) override {
        SimTime_t latency = getArrayLatency(arrayID);
        ArrayEvent* ev = new ArrayEvent(arrayID);
        selfLink->send(latency, ev);
    }

    virtual void handleSelfEvent(Event* ev) override {
        ArrayEvent* aev = static_cast<ArrayEvent*>(ev);
        uint32_t arrayID = aev->getArrayID();

        compute(arrayID);

        (*tileHandler)(ev);
    }

    virtual void setMatrixItem(int32_t arrayID, int32_t index, double value) override {
        matrixData[arrayID][index] = static_cast<T>(value);
    }

    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) override {
        inputVectors[arrayID][index] = static_cast<T>(value);
    }

    virtual void compute(uint32_t arrayID) override {
        auto& inputVector = inputVectors[arrayID];
        auto& outputVector = outputVectors[arrayID];
        auto& matrix = matrixData[arrayID];

        // Ensure output vector is correctly sized
        outputVector.resize(outputArraySize);

        // Initialize output vector to zero
        std::fill(outputVector.begin(), outputVector.end(), T());

        // Print input vector
        out.verbose(CALL_INFO, 2, 0, "MVM for array %u:\n\n", arrayID);
        for (uint32_t col = 0; col < inputArraySize; col++) {
            printValue(inputVector[col]);
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");

        // Perform matrix-vector multiplication
        for (uint32_t row = 0; row < outputArraySize; row++) {
            for (uint32_t col = 0; col < inputArraySize; col++) {
                outputVector[row] += matrix[row * inputArraySize + col] * inputVector[col];
                printValue(matrix[row * inputArraySize + col]);
            }
            out.verbose(CALL_INFO, 2, 0, "  ");
            printValue(outputVector[row]);
            out.verbose(CALL_INFO, 2, 0, "\n");
        }
        out.verbose(CALL_INFO, 2, 0, "\n\n");
    }

    virtual SimTime_t getArrayLatency(uint32_t arrayID) override {
        return 1;
    }

    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) override {
        std::copy(outputVectors[srcArrayID].begin(), outputVectors[srcArrayID].end(), inputVectors[destArrayID].begin());
    }

    virtual void* getInputVector(uint32_t arrayID) override {
        return static_cast<void*>(&inputVectors[arrayID]);
    }

    virtual void* getOutputVector(uint32_t arrayID) override {
        return static_cast<void*>(&outputVectors[arrayID]);
    }

protected:
    std::vector<std::vector<T>> inputVectors;
    std::vector<std::vector<T>> outputVectors;
    std::vector<std::vector<T>> matrixData;

    void printValue(const T& value) {
        if constexpr (std::is_same<T, int64_t>::value) {
            out.verbose(CALL_INFO, 2, 0, "%" PRId64 " ", value);
        } else if constexpr (std::is_same<T, float>::value) {
            out.verbose(CALL_INFO, 2, 0, "%f ", value);
        }
    }
};

} // namespace Golem
} // namespace SST

#endif
