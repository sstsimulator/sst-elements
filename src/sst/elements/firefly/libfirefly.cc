// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include <hades.h>
#include <funcSM/init.h>
#include <funcSM/fini.h>
#include <funcSM/rank.h>
#include <funcSM/size.h>
#include <funcSM/makeProgress.h>
#include <funcSM/allreduce.h>
#include <funcSM/commDestroy.h>
#include <rangeLatMod.h>
#include <scaleLatMod.h>
#include <nodePerf.h>

/*
  Install the python library
 */
#include <sst/core/model/element_python.h>

namespace SST {
namespace Firefly {

char pyfirefly[] = {
#include "pyfirefly.inc"
    0x00};

class FireflyPyModule : public SSTElementPythonModule {
public:
    FireflyPyModule(std::string library) :
        SSTElementPythonModule(library)
    {
        auto primary_module = createPrimaryModule(pyfirefly,"pyfirefly.py");
    }

    SST_ELI_REGISTER_PYTHON_MODULE(
        SST::Firefly::FireflyPyModule,
        "firefly",
        SST_ELI_ELEMENT_VERSION(1,0,0)
    )

    SST_ELI_EXPORT(SST::Firefly::FireflyPyModule)

};
}
}
