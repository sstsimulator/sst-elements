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


#ifndef COMPONENTS_PYPROTO_PYMODULE_H
#define COMPONENTS_PYPROTO_PYMODULE_H

extern "C" {

void* genPyProtoPyModule(void);


struct PyEvent_t {
    PyObject_HEAD;
    PyObject *type;
    PyObject *dict; /* Holds elements from Events */
};


struct PyLink_t {
    PyObject_HEAD;
    PyObject *object; /* PyProto Object */
    char *portName;
    size_t portNumber;
};


struct PyProto_t {
    PyObject_HEAD;
    char *name;
    PyObject *tcomponent;
    void *component;

    typedef std::vector<std::pair<PyObject*, std::string> > clockArray_t;
    typedef std::vector<std::pair<std::string, PyObject*> > linkArray_t;

    clockArray_t *clocks;
    linkArray_t  *links;
    bool constructed;

};


}

namespace SST {
class Event;
namespace PyProtoNS {
        PyEvent_t *convertEventToPython(SST::Event *event);
        PyTypeObject* getEventObject();
        PyTypeObject* getPyProtoObject();
        PyTypeObject* getPyLinkObject();
}
}

#endif // COMPONENTS_PYPROTO_PYMODULE_H
