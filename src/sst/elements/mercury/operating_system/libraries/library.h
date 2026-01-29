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

#include <sst/core/event.h>
#include <sst/core/subcomponent.h>
#include <sst/core/eli/elementbuilder.h>
#include <mercury/common/component.h>
#include <mercury/common/events.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/operating_system/process/app_fwd.h>
#include <mercury/operating_system/process/thread_fwd.h>
#include <mercury/common/timestamp.h>
#include <mercury/common/node_address.h>
#include <sys/time.h>

namespace SST {
namespace Hg {

class Request;
class OperatingSystemAPI;

void apiLock();
void apiUnlock();

class Library
{
 public:
  SST_ELI_DECLARE_BASE(Library)
  SST_ELI_DECLARE_DEFAULT_INFO()
  SST_ELI_DECLARE_CTOR(SST::Params&,SST::Hg::App*)

  virtual ~Library();

  SoftwareId sid() const;

  NodeId addr() const;

  App* parent() const {
    return api_parent_app_;
  }

  Thread* activeThread();

  virtual void init(){}

  virtual void finish(){}

  Timestamp now() const;

  void schedule(Timestamp t, ExecutionEvent* ev);

  void scheduleDelay(TimeDelta t, ExecutionEvent* ev);

  /**
   * @brief start_api_call
   * Enter a call such as MPI_Send. Any perf counters or time counters
   * collected since the last API call can then advance time or
   * increment statistics.
   */
  void startLibraryCall();

  /**
   * @brief end_api_call
   * Exit a call such as MPI_Send. Perf counters or time counters
   * collected since the last API call can then clear counters for
   * the next time window.
   */
  void endLibraryCall();

  const std::string& libName() const {
    return libname_;
  }

  std::string toString() const {
    return libname_;
  }

  virtual void incomingRequest(Request* req);
  virtual void incomingEvent(Event* ev);

  Library(const std::string& libname, SoftwareId sid, OperatingSystemAPI* os);

 protected:
  OperatingSystemAPI* os_;
  SoftwareId sid_;
  NodeId addr_;

  Library(SST::Params& params, App* parent);
  App* api_parent_app_;

 private:
  std::string libname_;
};

} // end namespace Hg
} // end namespace SST
