// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <Python.h>

#include "pymodule.h"

static char pymerlin[] = {
#include "pymerlin.inc"
    0x00};

void* genMerlinPyModule(void)
{
    // Must return a PyObject

    PyObject *code = Py_CompileString(pymerlin, "pymerlin", Py_file_input);
    return PyImport_ExecCodeModule("sst.merlin", code);
}

