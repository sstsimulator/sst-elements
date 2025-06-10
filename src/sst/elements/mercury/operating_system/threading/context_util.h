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

#include <mercury/common/errors.h>
#include <sst_hg_config.h>

#include <assert.h>

namespace SST {
namespace Hg {

/**
 * A set of utilities to deal with the odd arguments to makecontext.
 */

// We assume that we can always fit a pointer (void* or void (*ptr)(void*))
// into two integers.  This should ideally be tested by autoconf.
struct intpair {
  int a;
  int b;
};

/// Pack a void* argument into a pair of integers.
union  voidptr {
  intpair  vpair;
  void    *vpointer;
  voidptr() {
    vpair.a = 0;
    vpair.b = 0;
    vpointer = 0;
  }
  voidptr(void *ptr) {
    assert(sizeof(void*) <= (2*sizeof(int)));
    vpair.a = 0;
    vpair.b = 0;
    vpointer = ptr;
  }
  voidptr(int a, int b) {
    vpointer = 0;
    vpair.a = a;
    vpair.b = b;
  }
};

/// Pack a function pointer of the form void(*)(void*) as a pair of integers.
union funcptr {
  intpair fpair;
  void (*fpointer)(void*);
  funcptr() {
    fpair.a  = 0;
    fpair.b  = 0;
    fpointer = 0;
  }
  funcptr(void (*ptr)(void*)) {
    assert(sizeof(void(*)(void*)) <= (2*sizeof(int)));
    fpair.a  = 0;
    fpair.b  = 0;
    fpointer = ptr;
  }
  funcptr(int a, int b) {
    fpointer = 0;
    fpair.a  = a;
    fpair.b  = b;
  }
  void call(const voidptr &arg) {
    if(fpointer == 0) {
      SST::Hg::abort("union functpr::call(const voidptr&): NULL function pointer");
    }
    (*fpointer)(arg.vpointer);
  }
};

/// Springboard routine to wrap a makecontext call.
/// We have to be able to take a pointer to this function
/// (so no inline).
void context_springboard(int func_ptr_a, int func_ptr_b,
                         int arg_ptr_a,  int arg_ptr_b);

} // end of namespace Hg
} // end of namespace SST
