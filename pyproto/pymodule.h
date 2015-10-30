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


#ifndef COMPONENTS_PYPROTO_PYMODULE_H
#define COMPONENTS_PYPROTO_PYMODULE_H

extern "C" {

void* genPyProtoPyModule(void);


typedef struct {
    PyObject_HEAD;
} PyEvent_t;


typedef struct {
    PyObject_HEAD;
    PyObject *object; /* PyProto Object */
    char *portName;
    size_t portNumber;
} PyLink_t;


typedef struct {
    PyObject_HEAD;
    char *name;
    PyObject *tcomponent;
    void *component;

    typedef std::vector<std::pair<PyObject*, std::string> > clockArray_t;
    typedef std::vector<std::pair<std::string, PyObject*> > linkArray_t;

    clockArray_t *clocks;
    linkArray_t  *links;
    bool constructed;

} PyProto_t;


}

namespace SST {
class Event;
namespace PyProtoNS {
        PyEvent_t *convertEventToPython(SST::Event *event);
}
}

#endif // COMPONENTS_PYPROTO_PYMODULE_H
