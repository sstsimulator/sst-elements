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

#pragma once

namespace SST {
namespace Hg {

class loaderAPI : public SST::SubComponent
{
public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::loaderAPI)

    loaderAPI(SST::ComponentId_t id, SST::Params& params);
    virtual ~loaderAPI();

};

} // end namespace Hg
} // end namespace SST
