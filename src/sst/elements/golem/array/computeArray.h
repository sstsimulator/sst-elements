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

#ifndef _COMPUTEARRAY_H
#define _COMPUTEARRAY_H

#include <cstdint>
#include <sst/core/component.h>
#include <sst/core/subcomponent.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <vector>

namespace SST {
namespace Golem {

class ArrayEvent : public SST::Event {
public:
    ArrayEvent() {} // For serialization only
    ArrayEvent(uint32_t array) : SST::Event(), arrayID(array) {}

    uint32_t getArrayID() { return arrayID; };

protected:
    uint32_t arrayID;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(arrayID);
    }
    ImplementSerializable(SST::Golem::ArrayEvent);
};

class ComputeArray : public SST::SubComponent {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(
        SST::Golem::ComputeArray,
        TimeConverter*,
        Event::HandlerBase*
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose", "Verbosity of outputs", "1"},
        {"arrayLatency", "Latency of array operation", "100ns"},
        {"numArrays", "Number of arrays", "1"},
        {"arrayInputSize", "Input size of arrays", "1"},
        {"arrayOutputSize", "Output size of arrays", "1"},
        {"inputOperandSize", "Size of input operands", "1"},
        {"outputOperandSize", "Size of output operands", "1"},
    )

    ComputeArray(ComponentId_t id, Params& params,
                 TimeConverter* tc,
                 Event::HandlerBase* handler)
        : SubComponent(id), out("", params.find<int>("verbose", 1), 0, Output::STDOUT),
          tileHandler(handler) {
        // Initialize parameters
        arrayLatency = params.find<UnitAlgebra>("arrayLatency", "100ns");
        latencyTC = getTimeConverter(arrayLatency);

        numArrays = params.find<uint64_t>("numArrays", 1);
        inputArraySize = params.find<uint64_t>("arrayInputSize", 1);
        outputArraySize = params.find<uint64_t>("arrayOutputSize", 1);
        inputOperandSize = params.find<uint64_t>("inputOperandSize", 1);
        outputOperandSize = params.find<uint64_t>("outputOperandSize", 1);
    }

    virtual ~ComputeArray() {}

    virtual void init(unsigned int phase) override {}
    virtual void setup() override {}
    virtual void finish() override {}
    virtual void emergencyShutdown() override {}

    virtual void beginComputation(uint32_t arrayID) = 0;
    virtual void handleSelfEvent(Event* ev) = 0;
    virtual SimTime_t getArrayLatency(uint32_t arrayID) = 0;
    virtual void setMatrixItem(int32_t arrayID, int32_t index, double value) = 0;
    virtual void setVectorItem(int32_t arrayID, int32_t index, double value) = 0;
    virtual void compute(uint32_t arrayID) = 0;
    virtual void moveOutputToInput(uint32_t srcArrayID, uint32_t destArrayID) = 0;
    virtual void* getInputVector(uint32_t arrayID) = 0;
    virtual void* getOutputVector(uint32_t arrayID) = 0;

protected:
    SST::Output out;
    SST::Link* selfLink = nullptr;
    SST::Event::HandlerBase* tileHandler = nullptr;
    UnitAlgebra arrayLatency;
    TimeConverter* latencyTC = nullptr;

    uint64_t numArrays;
    uint64_t inputArraySize;
    uint64_t outputArraySize;
    uint64_t inputOperandSize;
    uint64_t outputOperandSize;
};

} // namespace Golem
} // namespace SST

#endif /* _COMPUTEARRAY_H */
