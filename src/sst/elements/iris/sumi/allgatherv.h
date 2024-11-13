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

#include <sst/core/output.h>
#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/comm_functions.h>
#include <iris/sumi/allgather.h>

namespace SST::Iris::sumi {

class BruckAllgathervActor : public DagCollectiveActor
{

 public:
  BruckAllgathervActor(CollectiveEngine* engine, void *dst, void *src, int* recv_counts,
                         int type_size, int tag, int cq_id, Communicator* comm) :
    DagCollectiveActor(Collective::allgatherv, engine, dst, src, type_size, tag, cq_id, comm),
    recv_counts_(recv_counts)
  {
    total_nelems_ = 0;
    for (int i=0; i < dom_nproc_; ++i){
      if (i == dom_me_)
        my_offset_ = total_nelems_;
      total_nelems_ += recv_counts_[i];
    }
  }

  std::string toString() const override {
    return "bruck allgatherv actor";
  }

  Output output;

 private:
  void finalize() override;

  void finalizeBuffers() override;
  void initBuffers() override;
  void initDag() override;

  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

  int nelems_to_recv(int partner, int partner_gap);

  int* recv_counts_;
  int total_nelems_;
  int my_offset_;

};

class BruckAllgathervCollective :
  public DagCollective
{
 public:
  BruckAllgathervCollective(CollectiveEngine* engine, void *dst, void *src, int* recv_counts,
                              int type_size, int tag, int cq_id, Communicator* comm) :
    DagCollective(allgatherv, engine, dst, src, type_size, tag, cq_id, comm),
    recv_counts_(recv_counts)
  {
  }

  std::string toString() const override {
    return "bruck allgatherv";
  }

  DagCollectiveActor* newActor() const override {
    return new BruckAllgathervActor(engine_, dst_buffer_, src_buffer_, recv_counts_, type_size_,
                                    tag_, cq_id_, comm_);
  }

 private:
  int* recv_counts_;

};

}
