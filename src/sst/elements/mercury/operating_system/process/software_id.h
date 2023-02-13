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
