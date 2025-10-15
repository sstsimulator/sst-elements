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
#include <vector>
#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/collective_message.h>
#include <iris/sumi/comm_functions.h>

namespace SST::Iris::sumi {

class DirectAlltoallvActor :
  public DagCollectiveActor
{
 public:
  DirectAlltoallvActor(CollectiveEngine* engine, void *dst, void *src, int* send_counts, int* recv_counts,
                         int type_size, int tag, int cq_id, Communicator* comm) :
    DagCollectiveActor(Collective::alltoallv, engine, dst, src, type_size, tag, cq_id, comm),
    send_counts_(send_counts), recv_counts_(recv_counts)
  {}

  std::string toString() const override {
    return "bruck all-to-allv actor";
  }

 protected:
  void finalize() override;

  void finalizeBuffers() override;

  void initBuffers() override;

  void initDag() override;

  void bufferAction(void *dst_buffer, void *msg_buffer, Action* ac) override;

 private:
  void addAction(
    const std::vector<Action*>& actions,
    int stride_direction,
    int num_initial,
    int stride);

  int* send_counts_;
  int* recv_counts_;
  int total_recv_size_;
  int total_send_size_;
};

class DirectAlltoallvCollective :
  public DagCollective
{
 public:
    DirectAlltoallvCollective(CollectiveEngine* engine, void *dst, void *src, int* send_counts, int* recv_counts,
                           int type_size, int tag, int cq_id, Communicator* comm) :
      DagCollective(Collective::alltoallv, engine, dst, src, type_size, tag, cq_id, comm),
      send_counts_(send_counts), recv_counts_(recv_counts)
    {}

  std::string toString() const override {
    return "all-to-all";
  }

  DagCollectiveActor* newActor() const override {
    return new DirectAlltoallvActor(engine_, dst_buffer_, src_buffer_, send_counts_, recv_counts_,
                                      type_size_, tag_, cq_id_, comm_);
  }

 private:
  int* send_counts_;
  int* recv_counts_;

};

}
