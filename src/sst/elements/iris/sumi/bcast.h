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

#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/comm_functions.h>

namespace SST::Iris::sumi {

class BinaryTreeBcastActor :
  public DagCollectiveActor
{
 public:
  BinaryTreeBcastActor(CollectiveEngine* engine, int root, void *buf, int nelems,
                          int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollectiveActor(Collective::bcast, engine, buf, buf, type_size, tag, cq_id, comm),
      root_(root), nelems_(nelems)
  {}

  std::string toString() const override {
    return "bcast actor";
  }

  ~BinaryTreeBcastActor(){}

  Output output;

 private:
  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;

  void init_root(int me, int roundNproc, int nproc);
  void init_child(int me, int roundNproc, int nproc);
  void init_internal(int me, int windowSize, int windowStop, Action* recv) ;
  void bufferAction(void *dst_buffer, void *msg_buffer, Action *ac) override;

  int root_;
  int nelems_;
};

class BinaryTreeBcastCollective :
  public DagCollective
{
 public:
  BinaryTreeBcastCollective(CollectiveEngine* engine, int root, void* buf,
                               int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollective(Collective::bcast, engine, buf, buf, type_size, tag, cq_id, comm),
      root_(root), nelems_(nelems) {}

  std::string toString() const override {
    return "bcast";
  }

  DagCollectiveActor* newActor() const override {
    return new BinaryTreeBcastActor(engine_, root_, dst_buffer_, nelems_,
                                       type_size_, tag_, cq_id_, comm_);
  }

 private:
  int root_;
  int nelems_;

};

}
