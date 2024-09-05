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
