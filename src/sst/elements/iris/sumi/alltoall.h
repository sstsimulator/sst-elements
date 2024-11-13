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
#include <sst/core/eli/elementbuilder.h>

namespace SST::Iris::sumi {

class AllToAllCollective : public DagCollective {
 public:

  AllToAllCollective(CollectiveEngine* engine, void* dst, void* src,
                     int nelems, int type_size, int tag, int cq_id,
                     Communicator* comm) :
    DagCollective(alltoall, engine, dst, src, type_size, tag, cq_id, comm),
    nelems_(nelems)
  {
  }

 protected:
  int nelems_;
};

class BruckAlltoallActor : public DagCollectiveActor
{
 public:
  BruckAlltoallActor(CollectiveEngine* engine, void* dst, void* src,
                       int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollectiveActor(Collective::alltoall, engine, dst, src, type_size, tag, cq_id, comm),
      nelems_(nelems)
  {
  }

  std::string toString() const override {
    return "Bruck all-to-all actor";
  }

 protected:
  void finalize() override;

  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;

  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

  void startShuffle(Action* ac) override;

  void shuffle(Action *ac, void* tmpBuf, void* mainBuf, bool copyToTemp);

  int midpoint_;
  int nelems_;
};

class BruckAlltoallCollective :
  public AllToAllCollective
{
 public:

  BruckAlltoallCollective(CollectiveEngine* engine, void* dst, void* src,
                            int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : AllToAllCollective(engine, dst, src, nelems, type_size, tag, cq_id, comm)
  {
  }

  std::string toString() const override {
    return "Bruck all-to-all";
  }

  DagCollectiveActor* newActor() const override {
    return new BruckAlltoallActor(engine_, dst_buffer_, src_buffer_,
                                  nelems_, type_size_, tag_, cq_id_, comm_);
  }

};

class DirectAlltoallActor :
  public DagCollectiveActor
{
 public:
  DirectAlltoallActor(CollectiveEngine* engine, void *dst, void *src, int nelems,
                         int type_size, int tag, int cq_id, Communicator* comm) :
    DagCollectiveActor(Collective::alltoall, engine, dst, src, type_size, tag, cq_id, comm),
    nelems_(nelems)
  {}

  std::string toString() const override {
    return "direct all-to-all actor";
  }

 protected:
  void finalize() override;

  void finalizeBuffers() override;

  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

  void initBuffers() override;

  void initDag() override;

 private:
  void addAction(
    const std::vector<Action*>& actions,
    int stride_direction,
    int num_initial,
    int stride);

  int nelems_;
  int total_recv_size_;
  int total_send_size_;
};

class DirectAlltoallCollective :
  public AllToAllCollective
{
 public:

    DirectAlltoallCollective(CollectiveEngine* engine, void *dst, void *src, int nelems,
                              int type_size, int tag, int cq_id, Communicator* comm) :
      AllToAllCollective(engine, dst, src, nelems, type_size, tag, cq_id, comm)
  {}

  std::string toString() const override {
    return "direct all-to-all";
  }

  DagCollectiveActor* newActor() const override {
    return new DirectAlltoallActor(engine_, dst_buffer_, src_buffer_, nelems_,
                                    type_size_, tag_, cq_id_, comm_);
  }

};

}
