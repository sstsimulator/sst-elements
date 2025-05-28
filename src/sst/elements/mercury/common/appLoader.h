// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/subcomponent.h>
#include <mercury/common/loaderAPI.h>

namespace SST {
namespace Hg {

#ifdef ssthg_app_name
#define CREATE_STRING(x) #x
#define EXPAND_STRING(x) CREATE_STRING(x)
#define CLASS_NAME loader
#define CONCAT(x, y) x ## y
#define EXPAND_CONCAT(x,y) CONCAT(x,y)

class EXPAND_CONCAT(CLASS_NAME, ssthg_app_name): public SST::Hg::loaderAPI
{
public:
EXPAND_CONCAT(CLASS_NAME, ssthg_app_name)(SST::ComponentId_t id, SST::Params& params) : SST::Hg::loaderAPI(id, params) {}
~EXPAND_CONCAT(CLASS_NAME, ssthg_app_name)() {}

SST_ELI_REGISTER_SUBCOMPONENT(
    EXPAND_CONCAT(CLASS_NAME, ssthg_app_name), 
    EXPAND_STRING(ssthg_app_name), 
    "loader" , 
    SST_ELI_ELEMENT_VERSION(1, 0, 0), 
    "description", 
    SST::Hg::loaderAPI)
};
#endif //MERCURY_LIB


} // end namespace Hg
} // end namespace SST

