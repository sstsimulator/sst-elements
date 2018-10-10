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
#include <structmember.h>

#include <vector>

#include <sst/core/output.h>

#include "pymodule.h"
#include "pyproto.h"

extern "C" {

static PyMethodDef moduleMethods[] = {
    { NULL, NULL, 0, NULL }
};

static int pyEvent_Init(PyEvent_t *self, PyObject *args, PyObject *kwds);
static void pyEvent_Dealloc(PyEvent_t *self);
static PyMethodDef pyEventMethods[] = {
    { NULL, NULL, 0, NULL }
};
static PyMemberDef pyEventMembers[] = {
    { (char*)"type", T_OBJECT, offsetof(PyEvent_t, type), 0, (char*)"Type of Event"},
    { (char*)"sst", T_OBJECT, offsetof(PyEvent_t, dict), 0, (char*)"SST Event members"},
    { NULL, 0, 0, 0, NULL }
};

static PyTypeObject PyEventDef = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.pyproto.PyEvent",           /* tp_name */
    sizeof(PyEvent_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)pyEvent_Dealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "PyProto Event",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pyEventMethods,            /* tp_methods */
    pyEventMembers,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pyEvent_Init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};



static int pyLink_Init(PyLink_t *self, PyObject *args, PyObject *kwds);
static void pyLink_Dealloc(PyLink_t *self);
static PyObject* pyLink_recv(PyObject *self, PyObject *args);
static PyObject* pyLink_send(PyObject *self, PyObject *args);


static PyMethodDef pyLinkMethods[] = {
    {   "recv", pyLink_recv, METH_NOARGS, "Receive from a link"},
    {   "send", pyLink_send, METH_VARARGS, "Send on a link"},
    {   NULL, NULL, 0, NULL }
};


static PyTypeObject PyLinkDef = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.pyproto.PyLink",           /* tp_name */
    sizeof(PyLink_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)pyLink_Dealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "PyProto Link",            /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pyLinkMethods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pyLink_Init,     /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};



static int pyProto_Init(PyProto_t *self, PyObject *args, PyObject *kwds);
static void pyProto_Dealloc(PyProto_t *self);
static PyObject* pyProto_addLink(PyObject *self, PyObject *args);
static PyObject* pyProto_addClock(PyObject *self, PyObject *args);
static PyObject* pyProto_construct(PyObject *self, PyObject *args);
static PyObject* pyProto_init(PyObject *self, PyObject *args);
static PyObject* pyProto_setup(PyObject *self, PyObject *args);
static PyObject* pyProto_finish(PyObject *self, PyObject *args);

static PyMethodDef pyProtoMethods[] = {
    {   "addLink", pyProto_addLink, METH_VARARGS, "Add a Link"},
    {   "addClock", pyProto_addClock, METH_VARARGS, "Add a clock handler"},
    {   "construct", pyProto_construct, METH_NOARGS, "Called during Construction"},
    {   "init", pyProto_init, METH_O, "Called during init"},
    {   "setup", pyProto_setup, METH_NOARGS, "Called during setup"},
    {   "finish", pyProto_finish, METH_NOARGS, "Called during finish"},
    {   NULL, NULL, 0, NULL }
};

static PyTypeObject PyProtoDef = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "sst.pyproto.PyProto",           /* tp_name */
    sizeof(PyProto_t),     /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)pyProto_Dealloc,   /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /* tp_flags */
    "PyProto Prototype",       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pyProtoMethods,            /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)pyProto_Init,    /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};



void* genPyProtoPyModule(void)
{
    // Must return a PyObject

    /* Initialize Types */
    PyEventDef.tp_new = PyType_GenericNew;
    PyLinkDef.tp_new = PyType_GenericNew;
    PyProtoDef.tp_new = PyType_GenericNew;

    if ( ( PyType_Ready(&PyEventDef) < 0 ) ||
         ( PyType_Ready(&PyLinkDef) < 0 ) ||
         ( PyType_Ready(&PyProtoDef) < 0 ) )
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Failed to instantiate PyProto Python Module");

    PyObject *module = Py_InitModule("sst.pyproto", moduleMethods);
    if ( !module )
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Failed to instantiate PyProto Python Module");

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    Py_INCREF(&PyEventDef);
    Py_INCREF(&PyLinkDef);
    Py_INCREF(&PyProtoDef);
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

    PyModule_AddObject(module, "PyEvent", (PyObject*)&PyEventDef);
    PyModule_AddObject(module, "PyLink", (PyObject*)&PyLinkDef);
    PyModule_AddObject(module, "PyProto", (PyObject*)&PyProtoDef);

    Py_INCREF(module); /* Want to return a NEW reference */
    return module;
}





/*****      PyEvent      *****/

static int pyEvent_Init(PyEvent_t *self, PyObject *args, PyObject *kwds)
{
    self->type = PyString_FromString("Python");
    self->dict = PyDict_New();
    PyErr_Print();
    return 0;
}


static void pyEvent_Dealloc(PyEvent_t *self)
{
    Py_XDECREF(self->type);
    Py_XDECREF(self->dict);
    self->ob_type->tp_free((PyObject*)self);
}



/*****      PyLink      *****/

static int pyLink_Init(PyLink_t *self, PyObject *args, PyObject *kwds)
{
    char *p;
    if ( !PyArg_ParseTuple(args, "Osn",
                &self->object, &p, &self->portNumber) )
        return -1;

    self->portName = strdup(p);
    Py_XINCREF(self->object);

    return 0;
}


static void pyLink_Dealloc(PyLink_t *self)
{
    Py_XDECREF(self->object);
    free(self->portName);
    self->ob_type->tp_free((PyObject*)self);
}


static PyObject* pyLink_recv(PyObject *self, PyObject *args)
{
    PyLink_t* l = (PyLink_t*)self;
    SST::PyProtoNS::PyProto *p = (SST::PyProtoNS::PyProto*)((PyProto_t*)l->object)->component;
    PyEvent_t *e = p->doLinkRecv(l->portNumber);
    if ( e ) {
        return (PyObject*)e;
    } else {
        Py_XINCREF(Py_None);
        return Py_None;
    }
}

static PyObject* pyLink_send(PyObject *self, PyObject *args)
{
    PyLink_t* l = (PyLink_t*)self;
    SST::PyProtoNS::PyProto *p = (SST::PyProtoNS::PyProto*)((PyProto_t*)l->object)->component;

    PyObject *event = NULL;
    if ( !PyArg_ParseTuple(args, "O!", &PyEventDef, &event) )
        return NULL;

    Py_XINCREF(event);
    p->doLinkSend(l->portNumber, (PyEvent_t*)event);
    return PyInt_FromLong(0);
}



/*****      PyProto      *****/

static int pyProto_Init(PyProto_t *self, PyObject *args, PyObject *kwds)
{
    char *name;
    if ( !PyArg_ParseTuple(args, "s", &name) )
        return -1;

    self->name = strdup(name);
    self->clocks = new PyProto_t::clockArray_t();
    self->links = new PyProto_t::linkArray_t();
    self->constructed = false;

    PyObject* sys_mod_dict = PyImport_GetModuleDict();
    PyObject* sst_mod = PyMapping_GetItemString(sys_mod_dict, (char*)"sst");
    self->tcomponent = PyObject_CallMethod(sst_mod, (char*)"Component",
            (char*)"ss", self->name, (char*)"pyproto.PyProto");
    if ( !self->tcomponent )
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Failed to load sst.Component\n");

    SST::PyProtoNS::PyProto::addComponent(self);

    return 0;
}

static void pyProto_Dealloc(PyProto_t *self)
{
    Py_XDECREF((PyObject*)self->tcomponent);
    delete self->links;
    delete self->clocks;
    free(self->name);
    self->ob_type->tp_free((PyObject*)self);
}



static PyObject* pyProto_addLink(PyObject *self, PyObject *args)
{
    PyProto_t *pself = (PyProto_t*)self;
    if ( pself->constructed ) {
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Cannot add a Link once construction complete.");
    }

    PyObject *slink = NULL;
    char *lat = NULL;
    PyObject *cb = NULL;
    if ( !PyArg_ParseTuple(args, "Os|O", &slink, &lat, &cb) ) {
        return NULL;
    }

    size_t pnum = pself->links->size();
    char port[16] = {0};
    snprintf(port, 15, "port%zu", pnum);
    PyObject_CallMethod(pself->tcomponent, (char*)"addLink",
            (char*)"Oss", slink, port, lat);

    /* Push the callback (or NULL) onto the stack */
    Py_XINCREF(cb);
    pself->links->push_back(std::make_pair(port, cb));
    if ( (pnum+1) != pself->links->size() )
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Looks like a threading bug!\n");


    /* Allocate new Python-facing object */
    PyObject* sys_mod_dict = PyImport_GetModuleDict();
    PyObject* module = PyMapping_GetItemString(sys_mod_dict, (char*)"sst.pyproto");
    PyObject *nlink = PyObject_CallMethod(module, (char*)"PyLink",
            (char*)"Osn", pself, port, pnum);

    return nlink;
}


static PyObject* pyProto_addClock(PyObject *self, PyObject *args)
{
    PyProto_t *pself = (PyProto_t*)self;
    if ( pself->constructed ) {
        SST::Output::getDefaultObject().fatal(CALL_INFO, -1,
                "Cannot add a clock once construction complete.");
    }

    PyObject *cb = NULL;
    char *freq = NULL;

    if ( !PyArg_ParseTuple(args, "Os", &cb, &freq) ) {
        SST::Output::getDefaultObject().output("Bad arguments for function PyProto.addClock()\n");
        return NULL;
    }

    Py_INCREF(cb);
    pself->clocks->push_back(std::make_pair(cb, freq));

    return PyInt_FromLong(0);
}


static PyObject* pyProto_construct(PyObject *self, PyObject *args)
{
    return PyInt_FromLong(0);
}


static PyObject* pyProto_init(PyObject *self, PyObject *args)
{
    return PyInt_FromLong(0);
}


static PyObject* pyProto_setup(PyObject *self, PyObject *args)
{
    return PyInt_FromLong(0);
}


static PyObject* pyProto_finish(PyObject *self, PyObject *args)
{
    return PyInt_FromLong(0);
}


} /* extern "C" */


namespace SST {
namespace PyProtoNS {
    PyTypeObject* getEventObject() { return &PyEventDef; }
    PyTypeObject* getPyProtoObject() { return &PyProtoDef; }
    PyTypeObject* getPyLinkObject() { return &PyLinkDef; }
}
}


