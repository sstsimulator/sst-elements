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

#include <iris/sumi/collective.h>
#include <iris/sumi/collective_actor.h>
#include <iris/sumi/dense_rank_map.h>
#include <iris/sumi/message.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/communicator.h>
#include <sst/core/params.h>
#include <mercury/common/stl_string.h>

//using namespace sprockit::dbg;

/*
#undef debug_printf
#define debug_printf(flags, ...) \
  if (tag_ == 115 || tag_ == 114) std::cout << sprockit::spkt_printf(__VA_ARGS__) << std::endl
*/

//RegisterDebugSlot(sumi_collective_init,
// "print all debug output for collectives performed within the sumi framework")
//RegisterDebugSlot(sumi_collective,
// "print all debug output for collectives performed within the sumi framework")
//RegisterDebugSlot(sumi_vote,
// "print all debug output for fault-tolerant voting collectives within the sumi framework")

namespace SST::Iris::sumi {

#define enumcase(x) case x: return #x

const char*
Collective::tostr(type_t ty)
{
  switch (ty)
  {
    enumcase(donothing);
    enumcase(alltoall);
    enumcase(alltoallv);
    enumcase(allreduce);
    enumcase(allgather);
    enumcase(allgatherv);
    enumcase(scatter);
    enumcase(scatterv);
    enumcase(gather);
    enumcase(gatherv);
    enumcase(reduce);
    enumcase(reduce_scatter);
    enumcase(scan);
    enumcase(barrier);
    enumcase(bcast);
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
      "collective::tostr: unknown type %d", ty);
}

Collective::Collective(type_t ty, CollectiveEngine* engine, int tag, int cq_id, Communicator* comm) :
  my_api_(engine->tport()), 
  engine_(engine), 
  cq_id_(cq_id),
  comm_(comm), 
  dom_me_(comm->myCommRank()),
  dom_nproc_(comm->nproc()), 
  complete_(false), 
  tag_(tag),
  type_(ty), 
  subsequent_(nullptr)
{
  output.output("Rank %d=%d built collective of size %d in role=%d, tag=%d",
    my_api_->rank(), comm_->myCommRank(), comm_->nproc(), dom_me_, tag);
}

CollectiveDoneMessage*
Collective::addActors(Collective * /*coll*/)
{
  SST::Hg::abort("collective:add_actors: collective should not dynamically add actors");
  return nullptr;
}

void
Collective::actorDone(int comm_rank, bool& generate_cq_msg, bool& delete_collective)
{
  generate_cq_msg = false;
  delete_collective = false;

  int& refcount = refcounts_[comm_rank];
  refcount--;
  if (refcount == 0){
    refcounts_.erase(comm_rank);
    generate_cq_msg = true;
  }

  if (refcounts_.empty()){
    delete_collective = true;
  }
}

CollectiveDoneMessage*
Collective::recv(CollectiveWorkMessage* msg)
{
  return recv(msg->domTargetRank(), msg);
}

Collective::~Collective()
{
}

DagCollective::~DagCollective()
{
  actor_map::iterator it, end = my_actors_.end();
  for (it=my_actors_.begin(); it != end; ++it){
    delete it->second;
  }
}

void
DagCollective::initActors()
{
  DagCollectiveActor* actor = newActor();
  actor->init();
  my_actors_[dom_me_] = actor;
  refcounts_[dom_me_] = my_actors_.size();
}

CollectiveDoneMessage*
DagCollective::recv(int target, CollectiveWorkMessage* msg)
{
  output.output("Rank %d=%d %s got from %d on tag=%d for target %d",
    my_api_->rank(), dom_me_, Collective::tostr(type_), msg->sender(), tag_, target);

  DagCollectiveActor* vr = my_actors_[target];
  if (!vr){
    //data-centric collective - this actor does not exist
    pending_.push_back(msg);
    output.output("dag actor %d does not yet exit - queueing %s",
      target, msg->toString().c_str());
    return nullptr;
  } else {
    vr->recv(msg);
    if (vr->complete()){
      output.output("Rank %d=%d returning completion", my_api_->rank(), dom_me_);
      return vr->doneMsg();
    } else {
      output.output("Rank %d=%d not yet complete", my_api_->rank(), dom_me_);
      return nullptr;
    }
  }
}

void
DagCollective::start()
{
  actor_map::iterator it, end = my_actors_.end();
  for (it = my_actors_.begin(); it != end; ++it){
    DagCollectiveActor* actor = it->second;
    actor->start();
  }
}

void
DagCollective::deadlockCheck()
{
  std::cout << SST::Hg::sprintf("%s collective deadlocked on rank %d, tag %d",
                  tostr(type_), my_api_->rank(), tag_) << std::endl;

  actor_map::iterator it, end = my_actors_.end();
  for (it=my_actors_.begin(); it != end; ++it){
    DagCollectiveActor* actor = it->second;
    if (actor) {
      actor->deadlockCheck();
   } else {
      sst_hg_abort_printf("%s collective deadlocked on rank %d, tag %d, with NULL actor %d",
              tostr(type_), my_api_->rank(), tag_, it->first);
    }
  }
}

CollectiveDoneMessage*
DagCollective::addActors(Collective* coll)
{
  DagCollective* ar = static_cast<DagCollective*>(coll);
  { std::map<int, DagCollectiveActor*>::iterator it, end = ar->my_actors_.end();
  for (it=ar->my_actors_.begin(); it != end; ++it){
    my_actors_[it->first] = it->second;
  } }

  refcounts_[coll->comm()->myCommRank()] = ar->my_actors_.size();

  std::list<CollectiveWorkMessage*> pending = pending_;
  pending_.clear();
  CollectiveDoneMessage* msg = nullptr;
  { std::list<CollectiveWorkMessage*>::iterator it, end = pending.end();
  for (it=pending.begin(); it != end; ++it){
    msg = Collective::recv(*it);
  } }

  ar->my_actors_.clear();
  return msg;
}

}
