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

#include <iostream>
#include <ostream>

#define cout0 ::SST::Hg::output::out0()
#define coutn ::SST::Hg::output::outn()
#define cerr0 ::SST::Hg::output::err0()
#define cerrn ::SST::Hg::output::errn()


namespace SST {
namespace Hg {

class output
{
 public:
  static std::ostream& out0() {
    return (*out0_);
  }

  static std::ostream& outn() {
    return (*outn_);
  }

  static std::ostream& err0() {
    return (*err0_);
  }

  static std::ostream& errn() {
    return (*errn_);
  }

  static void init_out0(std::ostream* out0){
    out0_ = out0;
  }

  static void init_outn(std::ostream* outn){
    outn_ = outn;
  }

  static void init_err0(std::ostream* err0){
    err0_ = err0;
  }

  static void init_errn(std::ostream* errn){
    errn_ = errn;
  }

 protected:
  static std::ostream* out0_;
  static std::ostream* outn_;
  static std::ostream* err0_;
  static std::ostream* errn_;

};

} // end of namespace Hg
} // end of namespace SST
