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
