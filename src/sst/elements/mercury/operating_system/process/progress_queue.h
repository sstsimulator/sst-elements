/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

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

#include <queue>
#include <map>
#include <list>
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

}
}

