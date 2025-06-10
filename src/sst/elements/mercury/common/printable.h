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

#include <string>

namespace SST {
namespace Hg {

  class printable {
   public:
    virtual std::string toString() const = 0;
  };

  template <class T>
  std::string toString(T* t){
    printable* p = dynamic_cast<printable*>(t);
    if (p) return p->toString();
    else return " (no string) ";
  }

} // end namespace Hg
} // end namespace SST
