/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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

#include <functional>

namespace SST::Iris::sumi {

typedef std::function<void(void*,const void*,int)> reduce_fxn;
typedef std::function<void(int&, const int&)> vote_fxn;

template <typename data_t>
struct Add {
  typedef data_t type;
    static void
    op(data_t& dst, const data_t& src){
        dst += src;
    }
};

template <typename data_t>
struct Prod {
  typedef data_t type;
    static void
    op(data_t& dst, const data_t& src){
        dst *= src;
    }
};

// To work around Wint-in-bool-context
template <>
struct Prod<bool> {
  typedef bool type;
    static void
    op(bool& dst, const bool& src){
        dst = dst && src;
    }
};


template <typename data_t>
struct Min
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst < src ? dst : src;
  }
};

template <typename data_t>
struct LXOr
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = bool(dst) ^ bool(src);
  }
};

template <typename data_t>
struct BXOr
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst ^ src;
  }
};

template <typename data_t>
struct BAnd
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst & src;
  }
};

template <typename data_t>
struct BOr
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst | src;
  }
};

template <typename data_t>
struct Or
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst || src;
  }
};

template <typename data_t>
struct Max
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst < src ? src : dst;
  }
};

struct Null
{
  static void
  op(void* /*dst_buffer*/, const void* /*src_buffer*/, int /*nelems*/){}
};


template <typename data_t>
struct And
{
  typedef data_t type;
  static void
  op(data_t& dst, const data_t& src){
    dst = dst && src;
  }
};

template <template <typename> class Fxn, typename data_t>
struct ReduceOp
{
  static void
  op(void* dst_buffer, const void* src_buffer, int nelems){
    data_t* dst = reinterpret_cast<data_t*>(dst_buffer);
    const data_t* src = reinterpret_cast<const data_t*>(src_buffer);
    for (int i=0; i < nelems; ++i, ++src, ++dst){
        Fxn<data_t>::op(*dst, *src);
    }
  }
};

}
