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

#include <operating_system/process/app_id.h>
#include <operating_system/process/task_id.h>
#include <operating_system/process/thread_id.h>

#include <iostream>
#include <sstream>

namespace SST {
namespace Hg {

/**
 * A wrapper for an appid, taskid pair
 *
 */
struct SoftwareId {
  AppId app_;
  TaskId task_;
  ThreadId thread_;

  explicit SoftwareId(AppId a, TaskId t, ThreadId th = ThreadId(0))
    : app_(a), task_(t), thread_(th) {}

  SoftwareId() : app_(-1), task_(-1) { }

  std::string toString() const {
    std::stringstream ss(std::stringstream::in | std::stringstream::out);
    ss << "software_id(" << app_ << ", " << task_ << "," << thread_ << ")";
    return ss.str();
  }
};

inline bool operator==(const SoftwareId &a, const SoftwareId &b)
{
  return (a.app_ == b.app_ && a.task_ == b.task_ && a.thread_ == b.thread_);
}
inline bool operator!=(const SoftwareId &a, const SoftwareId &b)
{
  return (a.app_ != b.app_ || a.task_ != b.task_ || a.thread_ != b.thread_);
}
inline bool operator<(const SoftwareId &a, const SoftwareId &b)
{
  return (a.task_ < b.task_);
}
inline bool operator>(const SoftwareId &a, const SoftwareId &b)
{
  return (a.task_ > b.task_);
}


inline std::ostream& operator<<(std::ostream &os, const SoftwareId &n)
{
  return (os << n.toString());
}

} // end of namespace Hg
} // end of namespace SST
