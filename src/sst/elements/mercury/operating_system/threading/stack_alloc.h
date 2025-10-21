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

#include <cstddef>
#include <sst/core/params.h>

#include <cstring>
#include <vector>

namespace SST {
namespace Hg {

/**
 * A management type to handle dividing mmap-ed memory for use
 * as ucontext stack(s).  This is basically a very simple malloc
 * which allocates uniform-size chunks (with the NX bit unset)
 * and sets guard pages on each side of the allocated stacks.
 *
 * This allocator does not return memory to the system until it is
 * deleted, but regions can be allocated and free-d repeatedly.
 */
class StackAlloc
{
 public:
  class chunk;
  struct chunk_set {
    std::vector<chunk*> allocations;
    std::vector<void*> available;
    ~chunk_set(){
      clear();
    }
    void clear();
  };
 private:
  static chunk_set chunks_;
  /// Each chunk is of this suggested size.
  static size_t suggested_chunk_;
  /// Each stack request is of this size:
  static size_t stacksize_;
  /// Optionally added a protected stack between each stack we return
  static bool protect_stacks_;

 public:
  static size_t stacksize() {
    return stacksize_;
  }

  ~StackAlloc(){
    clear();
  }

  static size_t chunksize() {
    return suggested_chunk_;
  }

  static void init(SST::Params& params);

  static void* alloc();

  static void free(void*);

  static void clear();

};

} // end of namespace Hg
} // end of namespace SST

