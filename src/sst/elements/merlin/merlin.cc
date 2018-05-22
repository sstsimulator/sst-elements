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

#include <sst/core/element.h>
#include "merlin.h"


/*
  This are header file only classes, so need to be included here to
  get compiled.
 */
#include "hr_router/xbar_arb_rr.h"
#include "hr_router/xbar_arb_lru.h"
#include "hr_router/xbar_arb_age.h"
#include "hr_router/xbar_arb_rand.h"
#include "hr_router/xbar_arb_lru_infx.h"

/*
  Install the python library
 */
#include <sst/core/model/element_python.h>

char pymerlin[] = {
#include "pymerlin.inc"
    0x00};

class MerlinPyModule : public SSTElementPythonModule {
public:
    MerlinPyModule(std::string library) :
        SSTElementPythonModule(library)
    {
        addPrimaryModule(pymerlin);
    }

    SST_ELI_REGISTER_PYTHON_MODULE(
        MerlinPyModule,
        "merlin",
        SST_ELI_ELEMENT_VERSION(1,0,0)
    )
};

/*
  Needed temporarily because the Factory doesn't know if this is an
  SST library without it.
 */

// extern "C" {
//     ElementLibraryInfo merlin_eli = {
//         "merlin",
//         "Flexible network components",
//         NULL,
//         NULL,   // Events
//         NULL,   // Introspectors
//         NULL,
//         NULL,
//         NULL, // partitioners,
//         NULL,  // Python Module Generator
//         NULL // generators,
//     };
// }

