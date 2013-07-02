// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_ENTRY_H
#define COMPONENTS_FIREFLY_ENTRY_H

#include <vector>
#include "ioapi.h"
#include "msgHdr.h"
#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

class BaseEntry {
  public:
    BaseEntry(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::MessageRequest* req, Hermes::Functor* retFunc );

    Hermes::Addr                buf;
    uint32_t                    count;
    Hermes::PayloadDataType     dtype;
    uint32_t                    tag;
    Hermes::Communicator        group;
    Hermes::RankID              rank;
    Hermes::MessageRequest*     req;
    Hermes::MessageResponse*    resp;
    Hermes::Functor*            retFunc;
};

class RecvEntry;

class MsgEntry : public IO::Entry {
  public:
    uint32_t                    srcNodeId;
    Hdr                         hdr;
    std::vector<IO::IoVec>      vec; 
    std::vector<unsigned char>  buffer;
    RecvEntry*                  recvEntry;
};

class UnexpectedMsgEntry : public MsgEntry {
};
class IncomingMsgEntry : public MsgEntry {
};


class RecvEntry : public BaseEntry {
  public:
    RecvEntry(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::MessageRequest* req, Hermes::Functor* retFunc );

    UnexpectedMsgEntry*              msgEntry;
};

class SendEntry : public BaseEntry {
  public:
    SendEntry( Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        Hermes::RankID srcRank, Hermes::Functor* retFunc, int dtypeSize );


    Hdr                         hdr;
    std::vector< IO::IoVec >    vec;
};

class IOEntry : public IO::Entry {
  public:
    BaseEntry*                  entry;
};

}
}
#endif
