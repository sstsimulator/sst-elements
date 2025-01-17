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

#pragma once

#include <sst_element_config.h>
#include <sst/core/output.h>
#include <string>

namespace SST {
namespace Hg {

std::vector<std::string> split_path(const std::string& searchPath);

std::string loadExternPathStr();

void* loadExternLibrary(const std::string& libname, const std::string& searchPath);

void* loadExternLibrary(const std::string& libname);

void unloadExternLibrary(void* handle);

} // end namespace Hg
} // end namespace SST
