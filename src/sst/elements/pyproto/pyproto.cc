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

#include <sst_config.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#include <Python.h>
#pragma clang diagnostic pop

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


void PyProto::init(unsigned int phase)
{
    PyObject *res = PyObject_CallMethod((PyObject*)that, (char*)"init", (char*)"I", phase);
    Py_XDECREF(res);
}


void PyProto::setup()
{
    PyObject *res = PyObject_CallMethod((PyObject*)that, (char*)"setup", (char*)"");
    Py_XDECREF(res);
}


void PyProto::finish()
{
    PyObject *res = PyObject_CallMethod((PyObject*)that, (char*)"finish", (char*)"");
    Py_XDECREF(res);
}



PyEvent_t* PyProto::doLinkRecv(size_t linkNum)
{
    PyEvent_t *res = NULL;
    SST::Link *link = links.at(linkNum);
    Event *event = link->recv();
    if ( event ) {
        res = convertEventToPython(event);
        delete event;
    }
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
    PyEvent_t *pe = convertEventToPython(event);
    if ( pe ) {
        PyObject *args = Py_BuildValue("(O)", pe);
        PyObject *res = PyObject_CallObject(cb, args);
        Py_XDECREF(res);
        Py_XDECREF(pe);
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

