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

#include <mercury/common/component.h>
#include <mercury/components/node_base_fwd.h>
#include <mercury/operating_system/launch/app_launcher_fwd.h>
#include <mercury/operating_system/launch/app_launch_request.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/process/mutex.h>
#include <mercury/operating_system/libraries/event_library.h>
#include <mercury/hardware/network/network_message.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

class OperatingSystemBase : public SST::Hg::SubComponent {

public:

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::OperatingSystemBase)

  OperatingSystemBase(ComponentId_t id, SST::Params& params);

  virtual ~OperatingSystemBase() {}

  virtual void handleEvent(SST::Event *ev) = 0;

  virtual bool clockTic(SST::Cycle_t) = 0;

  virtual std::function<void(NetworkMessage*)> nicDataIoctl() = 0;

  virtual std::function<void(NetworkMessage*)> nicCtrlIoctl() = 0;

  /**
   * @brief block Block the currently running thread context.
   * This must be called from an application thread, NOT the DES thread
   * @param req [in-out] A key that will store blocking data that will be needed
   *                     for later unblocking
   * @return
   */
  virtual void block() = 0;

  virtual void unblock(Thread* thr) = 0;


  virtual void blockTimeout(TimeDelta delay) = 0;
  // void sleep(TimeDelta time);
  // void compute(TimeDelta time);

  virtual void startApp(App* theapp, const std::string&  /*unique_name*/) = 0;
  virtual void startThread(Thread* t) = 0;
  virtual void joinThread(Thread* t) = 0;
  virtual void scheduleThreadDeletion(Thread* thr) = 0;
  virtual void switchToThread(Thread* tothread) = 0;
  virtual void completeActiveThread() = 0;

  virtual Thread* activeThread() const = 0;

  virtual UniqueEventId allocateUniqueId() = 0;

  virtual NodeBase* node() const = 0;

  virtual Thread* getThread(uint32_t threadId) const = 0;

  virtual NodeId addr() const = 0;

  /**
   * Allocate a unique ID for a mutex variable
   * @return The unique ID
   */
  virtual int allocateMutex() = 0;

  /**
   * Fetch a mutex object corresponding to their ID
   * @param id
   * @return The mutex object corresponding to the ID. Return NULL if no mutex is found.
   */
  virtual mutex_t* getMutex(int id) = 0;

  virtual bool eraseMutex(int id) = 0;

  virtual int allocateCondition() = 0;

  virtual bool eraseCondition(int id) = 0;

  virtual condition_t* getCondition(int id) = 0;

  virtual void set_nranks(int32_t ranks) = 0;

  virtual int32_t nranks() = 0;

//
// EVENT LIBRARIES
//

  // Library* currentEventLibrary(const std::string &name);

  virtual EventLibrary* eventLibrary(const std::string& name) const = 0;

  virtual void registerEventLib(EventLibrary* lib) = 0;

  virtual void unregisterEventLib(EventLibrary* lib) = 0;

  virtual bool handleEventLibraryRequest(const std::string& name, Request* req) = 0;

  virtual void handleRequest(Request* req) = 0;

};

} // namespace Hg
} // namespace SST
