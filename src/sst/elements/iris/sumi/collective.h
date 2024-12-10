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

#include <output.h>
#include <iris/sumi/timeout.h>
#include <iris/sumi/collective_message_fwd.h>
#include <iris/sumi/transport_fwd.h>
#include <iris/sumi/communicator_fwd.h>
#include <iris/sumi/collective_actor_fwd.h>
#include <iris/sumi/comm_functions.h>
#include <iris/sumi/options.h>
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
