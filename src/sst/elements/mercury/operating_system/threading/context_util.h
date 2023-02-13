/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#pragma once

#include <common/errors.h>
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
