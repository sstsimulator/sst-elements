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

#include <Python.h>
#include <sst_config.h>
#include <sst/core/serialization.h>

#include <sst/core/params.h>

#include "pyproto.h"
#include "pymodule.h"


namespace SST {
namespace PyProtoNS {


PyEvent::PyEvent(PyEvent_t *e) : SST::Event()
{
    pyE = e;
    Py_XINCREF(pyE);
}

PyEvent::~PyEvent()
{
    Py_XDECREF(pyE);
}




PyProto::PyProto(SST::ComponentId_t id, SST::Params &params) : Component(id)
{
    if ( !Py_IsInitialized() ) { /* ERROR */ }
    size_t idx = pyObjIdx++;
    if ( idx >= pyObjects.size() )
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "PyProto has too many objects\n");

    that = pyObjects[idx];
    Py_XINCREF(that);
    that->component = this;

    /* Call subobject "construct" method */
    PyObject_CallMethod((PyObject*)that, (char*)"construct", (char*)"");
    /* Load up links and clocks */

    for ( auto c : (*that->clocks) ) {
        PyObject *cb = c.first;
        std::string &rate = c.second;
        registerClock(rate, new Clock::Handler<PyProto, PyObject*>(this, &PyProto::clock, cb));
    }

    size_t numLinks = that->links->size();
    for ( size_t nl = 0 ; nl < numLinks ; nl++ ) {
        std::string &port = that->links->at(nl).first;
        PyObject *cb = that->links->at(nl).second;

        SST::Link *link = NULL;
        if ( cb ) {
            link = configureLink(port, new Event::Handler<PyProto, size_t>(this, &PyProto::linkAction, nl));
        } else {
            link = configureLink(port);
        }
        links.push_back(link);
    }


    that->constructed = true;
}



PyProto::~PyProto()
{
    Py_XDECREF(that);
}

void PyProto::init(unsigned int phase) { }
void PyProto::setup() { }
void PyProto::finish() { }



PyEvent_t* PyProto::doLinkRecv(size_t linkNum)
{
    PyEvent_t *res = NULL;
    SST::Link *link = links.at(linkNum);
    Event *event = link->recv();
    PyEvent *pe = dynamic_cast<PyEvent*>(event);
    if ( pe ) {
        PyEvent_t *e = pe->getPyObj();
        res = e;
    }
    if ( event ) delete event;
    return res;
}


void PyProto::doLinkSend(size_t linkNum, PyEvent_t* pyEv)
{
    SST::Link *link = links.at(linkNum);
    link->send(new PyEvent(pyEv));
}


void PyProto::linkAction(Event *event, size_t linkNum)
{
    PyObject *cb = that->links->at(linkNum).second;
    /* Translate the Event to a Python-readable thing */
    /* Do something with the callback */
    PyEvent *pe = dynamic_cast<PyEvent*>(event);
    if ( pe ) {
        PyEvent_t *e = pe->getPyObj();
        PyObject *args = Py_BuildValue("(O)", e);
        PyObject *res = PyObject_CallObject(cb, args);
        Py_XDECREF(res);
    }
    delete event;
}


bool PyProto::clock(SST::Cycle_t cycle, PyObject *cb)
{
    PyObject *args = Py_BuildValue("(K)", cycle);
    PyObject *res = PyObject_CallObject(cb, args);
    if ( !res ) PyErr_Print();
    bool bres = (res && 1 == PyObject_IsTrue(res));
    Py_XDECREF(res);
    return bres;
}



std::vector<PyProto_t*> PyProto::pyObjects;
std::atomic<size_t> PyProto::pyObjIdx(0);

}
}
