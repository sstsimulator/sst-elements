// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SHOGUN_EVENT
#define _H_SHOGUN_EVENT

#include <sst/core/event.h>
#include <sst/core/interfaces/simpleNetwork.h>

using namespace SST::Interfaces;

namespace SST {
namespace Shogun {

    class ShogunEvent : public SST::Event {

    public:
        ShogunEvent(int dst)
            : dest(dst)
        {
            src = -1;
            req = nullptr;
        }

        ShogunEvent(int dst, int source)
            : dest(dst)
            , src(source)
        {
            req = nullptr;
        }

        ShogunEvent()
            : dest(-1)
            , src(-1)
        {
            req = nullptr;
        }

        ~ShogunEvent()
        {
            if (nullptr != req) {
                delete req;
            }
        }

        ShogunEvent* clone() override
        {
            ShogunEvent* newEv = new ShogunEvent(dest, src);
            newEv->setPayload(req->clone());

            return newEv;
        }

        int getDestination() const
        {
            return dest;
        }

        int getSource() const
        {
            return src;
        }

        void setSource(int source)
        {
            src = source;
        }

        SimpleNetwork::Request* getPayload()
        {
            return req;
        }

        void unlinkPayload()
        {
            req = nullptr;
        }

        void setPayload(SimpleNetwork::Request* payload)
        {
            req = payload;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) override
        {
            Event::serialize_order(ser);
            ser& dest;
            ser& src;
            ser& req;
        }

        ImplementSerializable(SST::Shogun::ShogunEvent);

    private:
        int dest;
        int src;
        SimpleNetwork::Request* req;
    };

}
}

#endif
