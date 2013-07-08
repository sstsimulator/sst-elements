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

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "entry.h"

using namespace SST::Firefly;
using namespace Hermes;

SendEntry::SendEntry( Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        Hermes::RankID srcRank, Hermes::Functor* retFunc, int dtypeSize ) :
    BaseEntry( buf, count, dtype, dest, tag, group, resp, req, retFunc )
{
    hdr.tag   = tag;
    hdr.count = count;
    hdr.dtype = dtype;
    hdr.srcRank = srcRank;
    hdr.group = group;

    vec.resize(2);
    vec[0].ptr = &hdr;
    vec[0].len = sizeof(hdr);
    vec[1].ptr = buf;
    vec[1].len = count * dtypeSize;
}


RecvEntry::RecvEntry(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID source, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
        Hermes::MessageRequest* req, Hermes::Functor* retFunc ) :
    BaseEntry( buf, count, dtype, source, tag, group, resp, req, retFunc )
{
}

BaseEntry::BaseEntry(Hermes::Addr _buf, uint32_t _count,
        Hermes::PayloadDataType _dtype, Hermes::RankID _source, uint32_t _tag,
        Hermes::Communicator _group, Hermes::MessageResponse* _resp,
        Hermes::MessageRequest* _req, Hermes::Functor* _retFunc ) :
    buf( _buf ),
    count( _count ),
    dtype( _dtype ),      
    tag( _tag ),
    group( _group ),
    rank( _source ),
    req( _req ),
    resp( _resp ),
    retFunc( _retFunc ) 
{
}
