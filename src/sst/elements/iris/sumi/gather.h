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

#include <string>
#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/comm_functions.h>

namespace SST::Iris::sumi {

class BtreeGatherActor :
  public DagCollectiveActor
{

 public:
  BtreeGatherActor(CollectiveEngine* engine, int root, void* dst, void* src,
                     int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollectiveActor(Collective::gather, engine, dst, src, type_size, tag, cq_id, comm),
      root_(root), nelems_(nelems) {}

  std::string toString() const override {
    return "btree gather actor";
  }

 protected:
  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;
  void initTree() override;
  void startShuffle(Action *ac) override;
  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

 private:
  int root_;
  int nelems_;
  int midpoint_;
  int log2nproc_;

};

class BtreeGather : public DagCollective
{
 public:
  BtreeGather(CollectiveEngine* engine, int root, void* dst, void* src,
               int nelems, int type_size, int tag, int cq_id, Communicator* comm)
   : DagCollective(gather, engine, dst, src, type_size, tag, cq_id, comm),
     root_(root), nelems_(nelems) {}

  std::string toString() const override {
    return "btree gather";
  }

  DagCollectiveActor* newActor() const override {
    return new BtreeGatherActor(engine_, root_, dst_buffer_, src_buffer_, nelems_, type_size_,
                                  tag_, cq_id_, comm_);
  }

 private:
  int root_;
  int nelems_;

};

}
