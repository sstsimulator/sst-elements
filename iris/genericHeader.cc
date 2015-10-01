/*
 * =====================================================================================
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
 * =====================================================================================
 */

/*
 * =====================================================================================
//       Filename:  gernericHeader.cc
//       For things in the corresponding that disturbs the build
 * =====================================================================================
 */

#ifndef  _GERNERICHEADER_CC_INC
#define  _GERNERICHEADER_CC_INC

#include <sst_config.h>
#include        "router_params.h"

namespace SST {
namespace Iris {

const char* message_class_string[]= { "INVALID_PKT", "PROC_REQ", "MC_RESP" };
Router_params* r_param = new Router_params();

const char* flit_type_string[] = {"UNK", "HEAD", "BODY", "TAIL" };

const char* routing_scheme_string[] = { 
    "TWONODE_ROUTING",  
    "XY", 
    "TORUS_ROUTING", 
    "RING_ROUTING" 
};

const char* Router_PS_string[]= {
    "INVALID",
        "EMPTY",
        "IB",
        "RC",
        "VCA_REQUESTED",
        "VCA",
        "SWA_REQUESTED",
        "SWA",
        "ST"
};

}
}

#endif   /* ----- #ifndef _GERNERICHEADER_CC_INC  ----- */
