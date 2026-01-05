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

#include <mercury/components/compute_library/operating_system_cl_api.h>

namespace SST {
namespace Hg {

OperatingSystemCLAPI::OperatingSystemCLAPI(ComponentId_t id, SST::Params &params)
    : OperatingSystemAPI(id,params) {}

} // namespace Hg
} // namespace SST
