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

#include <mercury/common/hg_printf.h>

#include <exception>
#include <cstdlib>
#include <iostream>

namespace SST {
namespace Hg {

//----------------------------------------------------------------------
// ASSERT
//      If condition is false,  print a message and dump core.
//      Useful for documenting assumptions in the code.
//
//      NOTE: needs to be a #define, to be able to print the location
//      where the error occurred.
//----------------------------------------------------------------------
#define SST_HG_ASSERT(condition)                                                     \
      if (!(condition)) {                                                       \
          fprintf(stderr, "Assertion failed: line %d, file \"%s\"\n",           \
                  __LINE__, __FILE__);                                          \
          fflush(stderr);                                                       \
          exit(1);                                                              \
      }

#define sst_hg_throw_printf(exc, template_str, ...) \
    throw exc(SST::Hg::sprintf(#exc ": " template_str "\n%s %d", ##__VA_ARGS__, __FILE__, __LINE__))

inline void abort(const std::string& str){
  std::cerr << str << std::endl;
  ::abort();
}

// Better to use core output objects when possible
#define sst_hg_abort_printf(template_str, ...) \
  SST::Hg::abort(SST::Hg::sprintf("error: " template_str "\n%s %d", \
    ##__VA_ARGS__, __FILE__, __LINE__));

/**
 * General errors, or base class for more specific errors.
 */
struct HgError : public std::exception {
  HgError(const std::string &msg) :
    message(msg) {
  }
  ~HgError() throw () override {}

  const char* what() const throw () override {
    return message.c_str();
  }

  const std::string message;
};

/**
 * Error indicating something was null and shouldn't have been
 */
struct NullError : public HgError {
  NullError(const std::string &msg) :
    HgError(msg) {
  }
  ~NullError() throw () override {}
};

/**
 * Error indicating some internal value was unexpected.
 */
struct ValueError : public HgError {
  ValueError(const std::string &msg) :
    HgError(msg) {
  }
  ~ValueError() throw () override {}
};

/**
 * A general error somewhere from a library
 */
struct LibraryError : public HgError {
  LibraryError(const std::string &msg) :
    HgError(msg) {
  }
  ~LibraryError() throw () override {}
};

/**
 * An error indicating something to do with the advancement of simulation time
 */
struct TimeError : public HgError {
  TimeError(const std::string &msg) :
    HgError(msg) {
  }
  ~TimeError() throw () override {}
};

/**
 * File open, read, or write error
 */
struct IOError : public HgError {
  IOError(const std::string &msg) :
    HgError(msg) {
  }
  ~IOError() throw () override {}
};

/**
 * An error indicating some format was not correct
 */
struct IllformedError : public HgError {
  IllformedError(const std::string &msg) :
    HgError(msg) {
  }
  ~IllformedError() throw () override {}
};

/**
 * An error having to do with an operating system
 */
struct OSError : public HgError {
  OSError(const std::string &msg) :
    HgError(msg) {
  }
  ~OSError() throw () override {}
};

/**
 * Something happened when allocating, deallocating, or mapping
 * a memory region.
 */
struct MemoryError : public HgError {
  MemoryError(const std::string &msg) :
    HgError(msg) {
  }

  ~MemoryError() throw () override {}
};

/**
 * Something happened when iterating over a collection
 */
struct IteratorError : public HgError {
  IteratorError(const std::string &msg) :
    HgError(msg) {
  }

  ~IteratorError() throw () override {}

};

/**
 * A function was intentionally unimplemented because it doesn't make sense,
 * or it is ongoing work
 */
struct UnimplementedError : public HgError {
  UnimplementedError(const std::string &msg) :
    HgError(msg) {
  }

  ~UnimplementedError() throw () override {}
};

/**
 * Exception indicating that chosen path is not yet ported to new framework.
 * Default to older framework.
*/
struct NotPortedError : public HgError {
  NotPortedError(const std::string& msg) :
    HgError(msg) {
  }
  ~NotPortedError() throw() override {}
};

/**
 * Key to a map was not in the map
 */
struct InvalidKeyError : public HgError {
  InvalidKeyError(const std::string &msg) :
    HgError(msg) {
  }
  ~InvalidKeyError() throw () override {}
};

/**
 * An index was out of range in a collection.
 */
struct RangeError : public HgError {
  RangeError(const std::string &msg) :
    HgError(msg) {
  }
  ~RangeError() throw () override {}
};

/**
 * Invalid user input
 */
struct InputError : public HgError {
  InputError(const std::string &msg) :
    HgError(msg) {
  }
  ~InputError() throw () override {}
};

} // end of namespace Hg
} // end of namespace sprockit
