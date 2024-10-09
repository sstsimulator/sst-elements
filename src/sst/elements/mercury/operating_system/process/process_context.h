// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

#include <string>

namespace SST {
namespace Hg {

struct ProcessContext {
  long ctxt;

  ProcessContext(long id) : ctxt(id) {}

  void operator=(long id){
    ctxt = id;
  }

  operator long() const {
    return ctxt;
  }

  bool operator==(long id) const {
    return ctxt == id;
  }

  static const long none = -1;
};

} // end namespace Hg
} // end namespace SST

