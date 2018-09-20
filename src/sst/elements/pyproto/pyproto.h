// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include <sst/core/elementinfo.h>


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
	PyEvent() {} // For serialization only

public:
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        // TODO:  Serialize pyE
    }

    ImplementSerializable(SST::PyProtoNS::PyEvent);
};



class PyProto : public SST::Component
{
public:

    SST_ELI_REGISTER_COMPONENT(
        PyProto,
        "pyproto",
        "PyProto",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Python Prototyping Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PORTS(
        {"port%d", "Link to another object", {}}
    )

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
