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

#ifndef _H_VANADIS_SNAPSHOT_REQ
#define _H_VANADIS_SNAPSHOT_REQ

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisSnapshotReq : public SST::Event {
public:
    VanadisSnapshotReq() : SST::Event(), coreId(-1), hwThread(-1) { }
    VanadisSnapshotReq(int coreId, int hwThread) : SST::Event(), coreId(coreId), hwThread(hwThread) { }

    ~VanadisSnapshotReq() {}

    int hwThread;
    int coreId;

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hwThread);
        SST_SER(coreId);
    }


    ImplementSerializable(SST::Vanadis::VanadisSnapshotReq);
};

class VanadisSnapshottResp : public SST::Event {
public:
    VanadisSnapshottResp() : SST::Event(), coreId(-1) { }

    VanadisSnapshottResp( int coreId ) :
        SST::Event(), coreId(coreId) {}

    ~VanadisSnapshottResp() {}

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(coreId);
    }

    ImplementSerializable(SST::Vanadis::VanadisSnapshottResp);

    int coreId;
};

} // namespace Vanadis
} // namespace SST

#endif
