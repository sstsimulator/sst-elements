// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_SNAPSHOTCUSTOMDATA_H
#define MEMHIERARCHY_SNAPSHOTCUSTOMDATA_H

#include <sst/core/interfaces/stdMem.h>
#include <string>

namespace SST { namespace MemHierarchy {
    /** All Addresses can be 64-bit */
    using Addr = uint64_t;

/**
 * CustomData subclass used to carry a mid-simulation snapshot request
 * from the Vanadis OS component into the memory hierarchy via
 * StandardMem::CustomReq.
 *
 * The L1 cache intercepts this CustomReq, saves its own state, and
 * injects a proper SnapshotAll MemEvent for the rest of the hierarchy.
 *
 * needsResponse() returns false so that StandardInterface does not
 * track this request in its outstanding-request map.
 */
class SnapshotCustomData : public SST::Interfaces::StandardMem::CustomData {
public:
    SnapshotCustomData() : seqNum_(0) {}  /* For serialization */
    SnapshotCustomData(uint32_t seqNum) : seqNum_(seqNum) {}

    uint32_t getSeqNum() const { return seqNum_; }

    Addr getRoutingAddress() override { return 0; }
    uint64_t getSize() override { return 0; }
    CustomData* makeResponse() override { return nullptr; }
    bool needsResponse() override { return false; }

    std::string getString() override {
        return "SnapshotCustomData(seq=" + std::to_string(seqNum_) + ")";
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        SST_SER(seqNum_);
    }

    ImplementSerializable(SST::MemHierarchy::SnapshotCustomData);

private:
    uint32_t seqNum_;
};

}} /* namespace SST::MemHierarchy */

#endif /* MEMHIERARCHY_SNAPSHOTCUSTOMDATA_H */
