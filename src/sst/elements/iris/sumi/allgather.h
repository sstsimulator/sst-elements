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

#include <sst/core/output.h>
#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/comm_functions.h>


namespace SST::Iris::sumi {

class AllgatherCollective : public DagCollective
{
 public:

 protected:
  AllgatherCollective(CollectiveEngine* engine, void* dst, void* src,
                      int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    :
    DagCollective(Collective::allgather, engine, dst, src, type_size, tag, cq_id, comm),
    nelems_(nelems)
  {
  }

  int nelems_;
};

struct BruckTree
{
  static void computeTree(int nproc, int& log2nproc, int& midpoint,
                   int& num_rounds, int& nprocs_extra_round);
};

class BruckActor : public DagCollectiveActor
{

 public:
  BruckActor(Collective::type_t ty, CollectiveEngine* engine, void* dst, void* src,
             int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollectiveActor(ty, engine, dst, src, type_size, tag, cq_id, comm),
      nelems_(nelems)
  {
  }

  std::string toString() const override {
    return std::string("bruck actor ") + Collective::tostr(type_);
  }

  Output output;

 private:
  void finalize() override;
  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;
  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

  int nelems_;

};

class BruckAllgatherCollective :
  public AllgatherCollective
{
 public:

  BruckAllgatherCollective(CollectiveEngine* engine, void* dst, void* src,
                  int nelems, int type_size, int tag, int cq_id, Communicator* comm)
   : AllgatherCollective(engine, dst, src, nelems, type_size, tag, cq_id, comm)
  {
  }

  std::string toString() const override {
    return "bruck allgather collective";
  }

  DagCollectiveActor* newActor() const override {
    return new BruckActor(Collective::allgather, engine_, dst_buffer_, src_buffer_,
                          nelems_, type_size_, tag_, cq_id_, comm_);
  }

};

class BruckBarrierCollective :
 public DagCollective
{
 public:
  BruckBarrierCollective(CollectiveEngine* engine, void* dst, void* src,
                         int tag, int cq_id, Communicator* comm)
   : DagCollective(Collective::barrier, engine, dst, src, 0, tag, cq_id, comm)
  {
  }

  std::string toString() const override {
    return "bruck barrier collective";
  }

  DagCollectiveActor* newActor() const override {
    return new BruckActor(Collective::barrier, engine_, dst_buffer_, src_buffer_, 0,
                          0, tag_, cq_id_, comm_);
  }
};

class RingAllgatherActor : public DagCollectiveActor
{
 public:
  RingAllgatherActor(CollectiveEngine* engine, void* dst, void* src,
                     int nelems, int type_size, int tag, int cq_id, Communicator* comm)
    : DagCollectiveActor(Collective::allgather, engine, dst, src, type_size, tag, cq_id, comm),
      nelems_(nelems)
  {
  }

  std::string toString() const override {
    return "bruck allgather actor";
  }

 private:
  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;
  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

  int nelems_;

};

class RingAllgatherCollective :
  public AllgatherCollective
{
 public:

  RingAllgatherCollective(CollectiveEngine* engine, void* dst, void* src,
                          int nelems, int type_size, int tag, int cq_id, Communicator* comm)
   : AllgatherCollective(engine, dst, src, nelems, type_size, tag, cq_id, comm)
  {
  }

  std::string toString() const override {
    return "ring allgather";
  }

  DagCollectiveActor* newActor() const override {
    return new RingAllgatherActor(engine_, dst_buffer_, src_buffer_, nelems_, type_size_,
                                  tag_, cq_id_, comm_);
  }

};

}
