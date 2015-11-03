// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_PYPROTO_PYPROTO_H
#define COMPONENTS_PYPROTO_PYPROTO_H

#include <inttypes.h>
#include <atomic>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/event.h>

extern "C" struct PyEvent_t;
extern "C" struct PyProto_t;
extern "C" typedef struct _object PyObject;

namespace SST {
namespace PyProtoNS {

class PyEvent : public SST::Event
{
public:
    PyEvent(PyEvent_t *e);
    ~PyEvent();

    PyEvent_t* getPyObj() { return pyE; }

private:
    PyEvent_t *pyE;

#if 0   /* No serialization! */
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Event);
    }
#endif
};



class PyProto : public SST::Component
{
public:
    PyProto(SST::ComponentId_t id, SST::Params& params);
    ~PyProto();

    virtual void init(unsigned int phase);
    virtual void setup();
    virtual void finish();

    static void addComponent(PyProto_t* obj)
    {
        pyObjects.push_back(obj);
    }

    PyEvent_t* doLinkRecv(size_t linkNum);
    void doLinkSend(size_t linkNum, PyEvent_t* pyEv);


protected:
    bool clock(SST::Cycle_t cycle, PyObject *cb);
    void linkAction(Event *event, size_t linkNum);

private:
    PyProto_t *that; /* The Python-space representation of this */
    std::vector<SST::Link*> links;

    static std::vector<PyProto_t*> pyObjects;
    static std::atomic<size_t> pyObjIdx;
};


}
}

#endif
