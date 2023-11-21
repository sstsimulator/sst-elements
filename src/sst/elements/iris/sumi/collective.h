/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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

#include <output.h>
#include <sumi/timeout.h>
#include <sumi/collective_message_fwd.h>
#include <sumi/transport_fwd.h>
#include <sumi/communicator_fwd.h>
#include <sumi/collective_actor_fwd.h>
#include <sumi/comm_functions.h>
#include <sumi/options.h>
#include <sst/core/eli/elementbuilder.h>
//#include <mercury/common/factory.h>
//#include <sprockit/debug.h>
#include <list>

//DeclareDebugSlot(sumi_collective)
//DeclareDebugSlot(sumi_vote)
//DeclareDebugSlot(sumi_collective_init)

namespace SST::Iris::sumi {

class Collective
{
 public:
  typedef enum {
    alltoall,
    alltoallv,
    allreduce,
    allgather,
    allgatherv,
    bcast,
    barrier,
    gather,
    gatherv,
    reduce,
    reduce_scatter,
    scan,
    scatter,
    scatterv,
    donothing
  } type_t;

  virtual std::string toString() const = 0;

  virtual ~Collective();

  /**
   * @brief persistent
   * Some collectives are not allowed to "exit" based on the protocol
   * They have to remain active and persistent even after receving a completion ack
   * @return Whether the collective is persistent
   */
  virtual bool persistent() const {
    return false;
  }

  static const char* tostr(type_t ty);

  virtual CollectiveDoneMessage* recv(int target, CollectiveWorkMessage* msg) = 0;

  CollectiveDoneMessage* recv(CollectiveWorkMessage* msg);

  virtual void start() = 0;

  Communicator* comm() const {
    return comm_;
  }

  bool complete() const {
    return complete_;
  }

  void setComplete() {
    complete_ = true;
  }

  int tag() const {
    return tag_;
  }

  int cqId() const {
    return cq_id_;
  }

  type_t type() const {
    return type_;
  }

  void actorDone(int comm_rank, bool& generate_cq_msg, bool& delete_event);

  virtual CollectiveDoneMessage* addActors(Collective* coll);

  static const int default_nproc = -1;

  virtual void deadlockCheck(){}

  virtual void initActors(){}

  bool hasSubsequent() const {
    return subsequent_;
  }

  void setSubsequent(Collective* coll){
    subsequent_ = coll;
  }

  Collective* popSubsequent(){
    auto* ret = subsequent_;
    subsequent_ = nullptr;
    return ret;
  }

  Output output;

 protected:
  Collective(type_t type, CollectiveEngine* engine, int tag, int cq_id, Communicator* comm);

  Transport* my_api_;
  CollectiveEngine* engine_;
  int cq_id_;
  Communicator* comm_;
  int dom_me_;
  int dom_nproc_;
  bool complete_;
  int tag_;

  std::map<int, int> refcounts_;
  Collective::type_t type_;

  Collective* subsequent_;

};

class DoNothingCollective : public Collective
{
 public:
  DoNothingCollective(CollectiveEngine* engine, int tag, int cq_id, Communicator* comm) :
    Collective(donothing, engine, tag, cq_id, comm)
  {
  }

  std::string toString() const override {
    return "DoNothing collective";
  }

  CollectiveDoneMessage* recv(int  /*target*/, CollectiveWorkMessage*  /*msg*/) override { return nullptr; }

  void start() override {}

};

class DagCollective :
  public Collective
{
 public:
  CollectiveDoneMessage* recv(int target, CollectiveWorkMessage* msg) override;

  void start() override;

  void deadlockCheck() override;

  void initActors() override;

  ~DagCollective() override;

 protected:
  virtual DagCollectiveActor* newActor() const = 0;

  CollectiveDoneMessage* addActors(Collective *coll) override;

 protected:
  DagCollective(Collective::type_t ty, CollectiveEngine* engine, void *dst, void *src,
                 int type_size, int tag, int cq_id, Communicator* comm) :
    Collective(ty, engine, tag, cq_id, comm),
    src_buffer_(src), dst_buffer_(dst), type_size_(type_size), fault_aware_(false)
  {
  }

  typedef std::map<int, DagCollectiveActor*> actor_map;
  actor_map my_actors_;

  void* src_buffer_;

  void* dst_buffer_;

  int type_size_;

  bool fault_aware_;

  std::list<CollectiveWorkMessage*> pending_;
};

}
