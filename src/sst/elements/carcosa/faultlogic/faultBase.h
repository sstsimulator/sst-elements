// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_FAULTBASE_H
#define SST_ELEMENTS_CARCOSA_FAULTBASE_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include "sst/core/serialization/serializable.h"
#include <random>
#include <vector>
#include <string>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;

class FaultInjectorBase;

class FaultBase : public SST::Core::Serialization::serializable {
public:
    FaultBase(Params& params, FaultInjectorBase* injector);

    FaultBase() = default;
    ~FaultBase() {}

    virtual bool faultLogic(Event*& ev);

    SST::Output* getSimulationOutput();

    SST::Output* getSimulationDebug();

    SST::MemHierarchy::MemEvent* convertMemEvent(Event*& ev);

    dataVec& getMemEventPayload(Event*& ev);

    void setMemEventPayload(Event*& ev, dataVec newPayload);
protected:
    FaultInjectorBase* injector_ = nullptr;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultBase::serialize_order(ser);
        SST_SER(injector_);
    }
    ImplementVirtualSerializable(FaultBase)
}; // class FaultBase
} // namespace SST::Carcosa

#endif