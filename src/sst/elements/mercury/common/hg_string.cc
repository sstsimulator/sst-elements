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

#include <stdio.h>
#include <string>
#include <cstdarg>

namespace SST {
namespace Hg {

std::string sprintf(const char *fmt, ...)
{
  char tmpbuf[512];

  va_list args;
  va_start(args, fmt);
  vsnprintf(tmpbuf, 512, fmt, args);
  std::string strobj = tmpbuf;
  va_end(args);

  return strobj;
}

} // end namespace Hg
} // end namespace SST
