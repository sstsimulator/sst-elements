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

#ifndef _H_VANADIS_CHECKPOINT_REQ
#define _H_VANADIS_CHECKPOINT_REQ

#include <sst/core/event.h>

namespace SST {
namespace Vanadis {

class VanadisCheckpointReq : public SST::Event {
public:
    VanadisCheckpointReq() : SST::Event(), coreId(-1), hwThread(-1) { }
    VanadisCheckpointReq(int coreId, int hwThread) : SST::Event(), coreId(coreId), hwThread(hwThread) { }

    ~VanadisCheckpointReq() {}

    int hwThread;
    int coreId;

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(hwThread);
        SST_SER(coreId);
    }


    ImplementSerializable(SST::Vanadis::VanadisCheckpointReq);
};

class VanadisCheckpointResp : public SST::Event {
public:
    VanadisCheckpointResp() : SST::Event(), coreId(-1) { }

    VanadisCheckpointResp( int coreId ) :
        SST::Event(), coreId(coreId) {}

    ~VanadisCheckpointResp() {}

private:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Event::serialize_order(ser);
        SST_SER(coreId);
    }

    ImplementSerializable(SST::Vanadis::VanadisCheckpointResp);

    int coreId;
};

} // namespace Vanadis
} // namespace SST

#endif
