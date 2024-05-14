// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

#ifdef __cplusplus

#include <mercury/common/errors.h>
#include <mercury/operating_system/process/task_id.h>
//#include <sprockit/spkt_string.h>
//#include <sprockit/debug.h>

/** Automatically inherit the errors */
//using sprockit::IllformedError;
//using sprockit::InputError;
//using sprockit::InvalidKeyError;
//using sprockit::IOError;
//using sprockit::IteratorError;
//using sprockit::LibraryError;
//using sprockit::MemoryError;
//using sprockit::NullError;
//using sprockit::OSError;
//using sprockit::RangeError;
//using sprockit::SpktError;
#endif

namespace SST::Hg {

template <class Out, class In>
Out* __safe_cast__(const char*  /*objname*/,
              const char* file,
              int line,
              In* in,
              const char* error_msg = "error")
{
  Out* out = dynamic_cast<Out*>(in);
  if (!out) {
    sst_hg_abort_printf("%s: failed to cast object at %s:%d\n%s",
                     error_msg, file, line, "null");
                     //in ? toString(in).c_str() : "null");
  }
  return out;
}

/**
 * First entry in VA_ARGS is the obj
 * Second entry is optional being an error msg
*/
#define safe_cast(type,...) \
    ::SST::Hg::__safe_cast__<type>(#type, __FILE__, __LINE__, __VA_ARGS__)

#define test_cast(type, obj) \
    dynamic_cast<type*>(obj)

#define known_cast(type,...) \
    safe_cast(type, __VA_ARGS__)

#define interface_cast(type,obj) \
    dynamic_cast<type*>(obj)

} // end of namespace SST::Hg
