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

#include <list>
#include <map>
#include <queue>
#include <vector>
#include <mercury/common/errors.h>
#include <mercury/common/timestamp.h>
#include <mercury/operating_system/process/thread_fwd.h>
#include <mercury/components/operating_system_fwd.h>

namespace SST {
namespace Hg {

struct ProgressQueue {
  OperatingSystem* os;

  ProgressQueue(OperatingSystem* os) : os(os)
  {
  }

  void block(std::list<Thread*>& q, double timeout);
  void unblock(std::list<Thread*>& q);

};

template <class Item>
struct SingleProgressQueue : public ProgressQueue {
  std::queue<Item*> items;
  std::list<Thread*> pending_threads;

  SingleProgressQueue(OperatingSystem* os) :
    ProgressQueue(os)
  {
  }

  Item* front(bool blocking = true, double timeout = -1){
    if (items.empty()){
      if (blocking){
        block(pending_threads, timeout);
      } else {
        return nullptr;
      }
    }
    if (items.empty()){
#if SST_HG_SANITY_CHECK
      if (timeout <= 0){
        spkt_abort_printf("not given a timeout, but unblocked to find no items");
      }
#endif
      return nullptr;
    } else {
      auto it = items.front();
      return it;
    }
  }

  void pop(){
    items.pop();
  }

  void incoming(Item* i){
    items.push(i);
    if (!pending_threads.empty()){
      unblock(pending_threads);
    }
  }


};

struct PollingQueue : public ProgressQueue {
  public:
   PollingQueue(OperatingSystem* os) :
     ProgressQueue(os),
     num_empty_calls_(0)
   {
   }

   void block();

   void unblock();

   bool blocked() const {
     return !pending_threads_.empty();
   }

 private:
  SST::Hg::Timestamp last_check_;
  int num_empty_calls_;
  std::list<Thread*> pending_threads_;
};

template <class Item>
struct PollingProgressQueue : public PollingQueue {
  PollingProgressQueue(OperatingSystem* os) :
    PollingQueue(os)
  {
  }

  void incoming(Item* item){
    items.push(item);
    if (blocked()){
      unblock();
    } else {
      for (auto* q : partners_){
        if (q->blocked()){
          q->unblock();
          break; //only unblock one
        }
      }
    }
  }

  void addPartner(PollingProgressQueue<Item>* partner){
    partners_.push_back(partner);
  }

  Item* front() {
    if (items.empty()){
      block();
      //this can unblock with nothing
      if (items.empty()){
        return nullptr;
      } else {
        return items.front();
      }
    } else {
      return items.front();
    }
  }

  void pop() {
    items.pop();
  }

 private:
  std::vector<PollingProgressQueue<Item>*> partners_;
  std::queue<Item*> items;

};

template <class Item>
struct MultiProgressQueue : public ProgressQueue {
  std::list<Thread*> any_threads;
  std::map<int,std::queue<Item*>> queues;
  std::map<int,std::list<Thread*>> pending_threads;
  OperatingSystem* os;

  MultiProgressQueue(OperatingSystem* os) : ProgressQueue(os)
  {
  }

  Item* find_any(bool blocking = true, double timeout = -1){
    for (auto& pair : queues){
      if (!pair.second.empty()){
        auto it = pair.second.front();
        pair.second.pop();
        return it;
      }
    }
    if (blocking){
      block(any_threads, timeout);
    } else {
      return nullptr;
    }

    for (auto& pair : queues){
      if (!pair.second.empty()){
        auto it = pair.second.front();
        pair.second.pop();
        return it;
      }
    }

#if SST_HG_SANITY_CHECK
    if (timeout <= 0){
      spkt_abort_printf("unblocked on CQ without timeout, but there are no messages");
    }
#endif
    return nullptr;
  }

  Item* find(int cq, bool blocking = true, double timeout = -1){
    if (queues[cq].empty()){
      if (blocking){
        block(pending_threads[cq], timeout);
      } else {
        return nullptr;
      }
    }


    if (queues[cq].empty()){
#if SST_HG_SANITY_CHECK
      if (timeout <= 0){
        spkt_abort_printf("unblocked on CQ with no timeout, but there are no items");
      }
#endif
      return nullptr;
    } else {
      auto it = queues[cq].front();
      queues[cq].pop();
      return it;
    }
  }

  void incoming(int cq, Item* it){
    queues[cq].push(it);
    if (!pending_threads[cq].empty()){
      unblock(pending_threads[cq]);
    } else if (!any_threads.empty()){
      unblock(any_threads);
    } else {
      //pass, nothing to do
    }
  }

};

} // end namespace Hg
} // end namespace SST

