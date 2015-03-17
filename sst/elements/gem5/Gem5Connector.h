// Copyright 2009-2015 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package.  For lucense
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_GEM5_GEM5CONNECTOR_H
#define SST_GEM5_GEM5CONNECTOR_H

#include <sst/core/serialization.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>

class SimObject;
class ExtConnector;
class Packet;


namespace SST {
class Link;
class Event;
namespace gem5 {

class Gem5Comp;

class Gem5Connector {

    struct Blob {
        Interfaces::SimpleMem::Addr addr;
        size_t size;
        std::vector<uint8_t> data;
        Blob(Interfaces::SimpleMem::Addr _addr, size_t _size, uint8_t *_data)
            : addr(_addr), size(_size)
        {
            data.resize(size);
            for ( size_t i = 0 ; i < size ; ++i ) {
                data[i] = _data[i];
            }
        }
    };

    enum Phase { CONSTRUCTION, INIT, RUN };

    Gem5Comp *comp;
    Output &out;
    Interfaces::SimpleMem *sstlink;
    ExtConnector *conn;
    Phase simPhase;
    std::vector<Blob> initBlobs;

    typedef std::map<Interfaces::SimpleMem::Request::id_t, ::Packet*> PacketMap_t;
    PacketMap_t g5packets;

public:
    Gem5Connector(Gem5Comp *g5Comp, Output &out, SimObject *g5obj, std::string &linkName);
    void init(unsigned int phase);
    void setup();

    void handleRecvFromG5(::Packet *pkt);
    void handleRecvFromSST(Interfaces::SimpleMem::Request *event);

};

}
}

#endif
