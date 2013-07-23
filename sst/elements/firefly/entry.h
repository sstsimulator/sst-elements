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

class SendEntry {
  public:
    SendEntry() {}

    SendEntry(Hermes::Addr _buf, uint32_t _count,
        Hermes::PayloadDataType _dtype, Hermes::RankID _dest, uint32_t _tag,
        Hermes::Communicator _group, Hermes::MessageRequest* _req ) : 
    buf( _buf ),
    count( _count ),
    dtype( _dtype ),      
    dest( _dest ),
    tag( _tag ),
    group( _group ),
    req( _req )
    {}

    Hermes::Addr                buf;
    uint32_t                    count;
    Hermes::PayloadDataType     dtype;
    Hermes::RankID              dest;
    uint32_t                    tag;
    Hermes::Communicator        group;
    Hermes::MessageRequest*     req;
};

class RecvEntry {
  public:
    RecvEntry() {}
    RecvEntry(Hermes::Addr _buf, uint32_t _count,
        Hermes::PayloadDataType _dtype, Hermes::RankID _src, uint32_t _tag,
        Hermes::Communicator _group, Hermes::MessageResponse* _resp,
        Hermes::MessageRequest* _req ) :
    buf( _buf ),
    count( _count ),
    dtype( _dtype ),      
    src( _src ),
    tag( _tag ),
    group( _group ),
    resp( _resp ),
    req( _req )
    {}

    Hermes::Addr                buf;
    uint32_t                    count;
    Hermes::PayloadDataType     dtype;
    Hermes::RankID              src;
    uint32_t                    tag;
    Hermes::Communicator        group;
    Hermes::MessageResponse*    resp;
    Hermes::MessageRequest*     req;
};

class MsgEntry : public IO::Entry {
  public:
    uint32_t                    srcNodeId;
    Hdr                         hdr;
    std::vector<unsigned char>  buffer;
};

}
}
#endif
