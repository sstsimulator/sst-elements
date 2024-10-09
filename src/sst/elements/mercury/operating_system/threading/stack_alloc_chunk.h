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

#include <operating_system/threading/stack_alloc.h>

#include <algorithm>
#include <iostream>
#include <unistd.h>

namespace SST {
namespace Hg {

/**
 * A chunk of allocated memory to be divided into fixed-size stacks.
 */
class StackAlloc::chunk
{
  /// The base address of my memory region.
  char *addr_;
  /// If true we mmap twice the requested space and mprotect every other stack
  bool protect_;
  /// The total size of my allocation.
  size_t size_;
  /// The target size of each open (unprotected) stack region.
  size_t stacksize_;
  /// When unprotected step_size = stacksize, otherwise 2x stacksize
  size_t step_size_;
  /// Offset for next stack (used in get_next_stack).
  size_t next_stack_offset_ = 0;

 public:
  /// Make a new chunk.
  chunk(size_t stacksize, size_t suggested_chunk_size, bool protect);

  ~chunk();

  void*  getNextStack();

};

} // end of namespace Hg
} // end of namespace SST
