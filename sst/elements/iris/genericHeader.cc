/*
 * =====================================================================================
//       Filename:  gernericHeader.cc
//       For things in the corresponding that disturbs the build
 * =====================================================================================
 */

#ifndef  _GERNERICHEADER_CC_INC
#define  _GERNERICHEADER_CC_INC

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
