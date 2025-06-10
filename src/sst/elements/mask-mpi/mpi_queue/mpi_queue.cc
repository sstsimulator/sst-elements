/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

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

#include <mpi_api.h>
#include <mpi_queue/mpi_queue.h>
#include <mpi_queue/mpi_queue_recv_request.h>
#include <mpi_queue/mpi_queue_probe_request.h>
#include <mpi_status.h>
#include <mpi_protocol/mpi_protocol.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>

#include <unusedvariablemacro.h>

#include <sst/core/params.h>
#include <sst/core/factory.h>
//#include <sprockit/debug.h>
#include <mercury/common/errors.h>
//#include <sprockit/statics.h>
//#include <sprockit/keyword_registration.h>
#include <mercury/common/util.h>
#include <stdint.h>

//RegisterNamespaces("traffic_matrix", "num_messages");
//RegisterKeywords(
//{ "smp_single_copy_size", "the minimum size of message for single-copy protocol" },
//{ "max_eager_msg_size", "the maximum size for using eager pt2pt protocol" },
//{ "max_vshort_msg_size", "the maximum size for mailbox protocol" },
//);

//DeclareDebugSlot(mpi_all_sends);
//RegisterDebugSlot(mpi_all_sends);

using namespace std::placeholders;

namespace SST::MASKMPI {

//static sprockit::NeedDeletestatics<MpiQueue> del_statics;

bool
MpiQueue::sortbyseqnum::operator()(MpiMessage* a, MpiMessage*b) const
{
  return (a->seqnum() < b->seqnum());
}

MpiQueue::MpiQueue(SST::Params& params, int task_id, MpiApi* api, Iris::sumi::CollectiveEngine* engine) :
  queue_(api->parent()->os()),
  taskid_(task_id),
  api_(api)
{
  max_vshort_msg_size_ = params.find<SST::UnitAlgebra>("max_vshort_msg_size", "512B").getRoundedValue();
  max_eager_msg_size_ = params.find<SST::UnitAlgebra>("max_eager_msg_size", "8192B").getRoundedValue();
  use_put_window_ = params.find<bool>("use_put_window", false);

  protocols_.resize(MpiProtocol::NUM_PROTOCOLS);
  protocols_[MpiProtocol::EAGER0] = new Eager0(params, this);
  protocols_[MpiProtocol::EAGER1] = new Eager1(params, this);
  protocols_[MpiProtocol::RENDEZVOUS_GET] = new RendezvousGet(params, this);
  protocols_[MpiProtocol::DIRECT_PUT] = new DirectPut(params, this);

  pt2pt_cq_ = api_->allocateCqId();
  coll_cq_ = api_->allocateCqId();

  api_->allocateCq(pt2pt_cq_, std::bind(&progress_queue::incoming, &queue_, pt2pt_cq_, _1));
  api_->allocateCq(coll_cq_, std::bind(&progress_queue::incoming, &queue_, coll_cq_, _1));
}

struct init_struct {
  int pt2pt_cq_id;
  int coll_cq_id;
  bool all_equal;
};

void
MpiQueue::init()
{
  auto init_fxn = [](void* out, const void* src, int nelems){
    auto* outs= (init_struct*)out;
    auto* srcs = (init_struct*)src;
    for (int i=0; i < nelems; ++i){
      auto& out = outs[i];
      auto& src = srcs[i];
      out.all_equal = src.all_equal
          && out.pt2pt_cq_id == src.pt2pt_cq_id
          && out.coll_cq_id == src.coll_cq_id;
    }
  };
  init_struct init;
  init.coll_cq_id = coll_cq_;
  init.pt2pt_cq_id = pt2pt_cq_;
  init.all_equal = true;
  auto cmsg = api_->engine()->allreduce(&init, &init, 1, sizeof(init_struct), 0, init_fxn,
                                        MpiMessage::Message::default_cq);
  if (cmsg == nullptr){
    cmsg = api_->engine()->blockUntilNext(MpiMessage::default_cq);
  }

  if (cmsg->tag() != 0){
    sst_hg_abort_printf("got bad collective done message in mpi_queue::init");
  }
  delete cmsg;

  if (!init.all_equal){
    sst_hg_abort_printf("MPI_Init: procs did not agree on completion queue ids");
  }
}

void
MpiQueue::deleteStatics()
{
}

SST::Hg::Timestamp
MpiQueue::now() const {
  return api_->now();
}

MpiQueue::~MpiQueue() throw ()
{
  for (auto* prot : protocols_){
    if (prot) delete prot;
  }
}

void
MpiQueue::send(MpiRequest *key, int count, MPI_Datatype type,
  int dest, int tag, MpiComm *comm, void *buffer)
{
  MpiType* typeobj = api_->typeFromId(type);
  if (typeobj->packed_size() < 0){
    sst_hg_throw_printf(SST::Hg::ValueError,
      "MPI_Datatype %s has negative size %ld",
      api_->typeStr(type).c_str(), typeobj->packed_size());
  }
  uint64_t bytes = count * uint64_t(typeobj->packed_size());

  int prot_id = MpiProtocol::RENDEZVOUS_GET;
  if (use_put_window_) {
    prot_id = MpiProtocol::DIRECT_PUT;
  } else if (bytes <= max_vshort_msg_size_) {
    prot_id = MpiProtocol::EAGER0;
  } else if (bytes <= max_eager_msg_size_) {
    prot_id = MpiProtocol::EAGER1;
  }

  MpiProtocol* prot = protocols_[prot_id];

//  mpi_queue_debug("starting send count=%d, type=%s, dest=%d, tag=%d, comm=%s, prot=%s",
//    count, api_->typeStr(type).c_str(), int(dest),
//    int(tag), api_->commStr(comm).c_str(),
//    prot->toString().c_str());

  TaskId dst_tid = comm->peerTask(dest);
  prot->start(buffer, comm->rank(), dest, dst_tid, count, typeobj,
              tag, comm->id(), next_outbound_[dst_tid]++, key);

}

MpiMessage*
MpiQueue::findMatchingRecv(MpiQueueRecvRequest* req)
{
  for (auto it = need_recv_match_.begin(); it != need_recv_match_.end(); ++it) {
    MpiMessage* mess = *it;
    if (req->matches(mess)) {
//      mpi_queue_debug("matched recv tag=%s,src=%s on comm=%s to send %s",
//        api_->tagStr(req->tag_).c_str(),
//        api_->srcStr(req->source_).c_str(),
//        api_->commStr(req->comm_).c_str(),
//        mess->toString().c_str());

      need_recv_match_.erase(it);
      return mess;
    }
  }
//  mpi_queue_debug("could not match recv tag=%s, src=%s to any of %d sends on comm=%s",
//    api_->tagStr(req->tag_).c_str(),
//    api_->srcStr(req->source_).c_str(),
//    need_recv_match_.size(),
//    api_->commStr(req->comm_).c_str());

  need_send_match_.push_back(req);
  return nullptr;
}

//
// Receive data.
//
void
MpiQueue::recv(MpiRequest* key, int count,
                MPI_Datatype type,
                int source, int tag,
                MpiComm* comm,
                void* buffer)
{
//  mpi_queue_debug("starting recv count=%d, type=%s, src=%s, tag=%s, comm=%s, buffer=%p",
//        count, api_->typeStr(type).c_str(), api_->srcStr(source).c_str(),
//        api_->tagStr(tag).c_str(), api_->commStr(comm).c_str(), buffer);

  MpiQueueRecvRequest* req = new MpiQueueRecvRequest(api_->now(), key, this,
                            count, type, source, tag, comm->id(), buffer);
  MpiMessage* mess = findMatchingRecv(req);
  if (mess) {
    auto* protocol = protocols_[mess->protocol()];
    protocol->incoming(mess, req);
  }
}

void
MpiQueue::finalizeRecv(MpiMessage* msg, MpiQueueRecvRequest* req)
{
  req->key_->complete(msg);
  if (req->recv_buffer_ != req->final_buffer_){
    req->type_->unpack_recv(req->recv_buffer_, req->final_buffer_, msg->count());
    delete[] req->recv_buffer_;
  }
  delete req;
}

//
// Ask for a notification when a message with the given signature arrives.
//
void
MpiQueue::probe(MpiRequest* key, MpiComm* comm,
                 int source, int tag)
{
//  mpi_queue_debug("starting probe src=%s, tag=%s, comm=%s",
//    api_->srcStr(source).c_str(), api_->tagStr(tag).c_str(),
//    api_->commStr(comm).c_str());

  mpi_queue_probe_request* req = new mpi_queue_probe_request(key, comm->id(), source, tag);
  // Figure out whether we already have a matching message.
  for (auto it = need_recv_match_.begin(); it != need_recv_match_.end(); ++it) {
    MpiMessage* mess = *it;
    if (req->matches(mess)){
      // We're good to go.
      req->complete(mess);
      return;
    }
  }
  // If we get here, we still need to wait for the message.
  probelist_.push_back(req);
}

//
// Immediate-mode probe for a message with the given signature.
//
bool
MpiQueue::iprobe(MpiComm* comm,
                  int source,
                  int tag,
                  MPI_Status* stat)
{
//  mpi_queue_debug("starting immediate probe src=%s, tag=%s, comm=%s",
//    api_->srcStr(source).c_str(), api_->tagStr(tag).c_str(),
//    api_->commStr(comm).c_str());

  mpi_queue_probe_request req(NULL, comm->id(), source, tag);
  for (auto it = need_recv_match_.begin(); it != need_recv_match_.end(); ++it) {
    MpiMessage* mess = *it;
    if (req.matches(mess)) {
      // This is it
      if (stat != MPI_STATUS_IGNORE) mess->buildStatus(stat);
      return true;
    }
  }
  return false;
}

MpiQueueRecvRequest*
MpiQueue::findMatchingRecv(MpiMessage* message)
{
  auto end = need_send_match_.end();
  for (auto it = need_send_match_.begin(); it != end;) {
    auto* req = *it;
    auto tmp = it++;
    if (req->isCancelled()) {
      need_send_match_.erase(tmp);
    } else if (req->matches(message)) {
      need_send_match_.erase(tmp);
      return req;
    }
  }
  need_recv_match_.push_back(message);
  return nullptr;
}

void
MpiQueue::incomingCollectiveMessage(Iris::sumi::Message* m)
{
  Iris::sumi::CollectiveWorkMessage* msg = safe_cast(Iris::sumi::CollectiveWorkMessage, m);
  Iris::sumi::CollectiveDoneMessage* cmsg = api_->engine()->incoming(msg);
  if (cmsg){
    MpiComm* comm = safe_cast(MpiComm, cmsg->dom());
    MpiRequest* req = comm->getRequest(cmsg->tag());
    CollectiveOpBase* op = req->collectiveData();
    api_->finishCollective(op);
    req->complete();
    delete cmsg;
  }
}

//
// Handle a new incoming message.  Can be either an eager send (with data)
// or a new handshake request
//
void
MpiQueue::incomingPt2ptMessage(Iris::sumi::Message* m)
{
  //mpi_queue_debug("incoming new message %s", m->toString().c_str());

  MpiMessage* message = safe_cast(MpiMessage, m);

  if (message->stage() > 0 || message->isNicAck()){
    //already matched - pass that on through
    handlePt2ptMessage(message);
    return;
  }

  TaskId tid(message->sender());
  auto& next_inbound = next_inbound_[tid];
  if (message->seqnum() == next_inbound){
//    mpi_queue_debug("seqnum for task %d matched expected seqnum %d and advanced to next seqnum",
//        int(tid), int(next_inbound));


    handlePt2ptMessage(message);
    ++next_inbound;

    // Handle any messages that have been freed by the arrival of this one
    if (!held_[tid].empty()) {
      hold_list_t::iterator it = held_[tid].begin(), end = held_[tid].end();
      while (it != end) {
        MpiMessage* mess = *it;
//        mpi_queue_debug("handling out-of-order message for task %d, seqnum %d",
//            int(tid), mess->seqnum());
        if (mess->seqnum() <= next_inbound_[tid]) {
          //it = held_[tid].erase(it);
          it++;
          handlePt2ptMessage(mess);
          ++next_inbound;
        } else {
          break;
        }
      }
      // Now erase all the completed messages from the held queue.
      held_[tid].erase(held_[tid].begin(), it);
    }
  } else if (message->seqnum() < next_inbound){
    sst_hg_abort_printf("message sequence went backwards on %s from %d",
                      message->toString().c_str(), next_inbound);
  } else {
//    mpi_queue_debug("message arrived out-of-order with seqnum %d, didn't match expected %d for task %d",
//        message->seqnum(), int(next_inbound), int(tid));
    held_[tid].insert(message);
  }
}

void
MpiQueue::notifyProbes(MpiMessage* message)
{
  auto pit = probelist_.begin();
  auto pend = probelist_.end();
  while (pit != pend) {
    auto tmp = pit++;
    mpi_queue_probe_request* preq = *tmp;
    if (preq->matches(message)){
      probelist_.erase(tmp);
      preq->complete(message);
      delete preq;
    }
  }
}

void
MpiQueue::handlePt2ptMessage(MpiMessage* message)
{
  MpiProtocol* protocol = protocols_[message->protocol()];
  protocol->incoming(message);
}

void
MpiQueue::incomingMessage(Iris::sumi::Message* msg)
{
  if (msg->cqId() == pt2pt_cq_){
    incomingPt2ptMessage(msg);
  } else if (msg->cqId() == coll_cq_){
    incomingCollectiveMessage(msg);
  } else {
    sst_hg_abort_printf("Got bad completion queue %d for %s",
                      msg->cqId(), msg->toString().c_str());
  }
}

void
MpiQueue::nonblockingProgress()
{
  Iris::sumi::Message* msg = api_->poll(false); //do not block
  while (msg){
    incomingMessage(msg);
    msg = api_->poll(false);
  }
}

SST::Hg::Timestamp
MpiQueue::progressLoop(MpiRequest* req)
{
  if (!req || req->isComplete()) {
    return api_->now();
  }

  //mpi_queue_debug("entering progress loop");

  SST::Hg::Timestamp wait_start = api_->now();
  while (!req->isComplete()) {
    //mpi_queue_debug("blocking on progress loop");
    Iris::sumi::Message* msg = queue_.find_any();
    if (!msg){
      sst_hg_abort_printf("polling returned null message");
    }
    req->setWaitStart(wait_start);
    incomingMessage(msg);
  }

  //mpi_queue_debug("finishing progress loop");
  return api_->now();
}

bool
MpiQueue::atLeastOneComplete(const std::vector<MpiRequest*>& req)
{
  //mpi_queue_debug("checking if any of %d requests is done", (int)req.size());
  for (int i=0; i < (int) req.size(); ++i) {
    if (req[i] && req[i]->isComplete()) {
      //mpi_queue_debug("request is done");
      return true;
    }
  }
  return false;
}

void
MpiQueue::startProgressLoop(const std::vector<MpiRequest*>& reqs)
{
  //mpi_queue_debug("starting progress loop");
  while (!atLeastOneComplete(reqs)) {
    //mpi_queue_debug("blocking on progress loop");
    Iris::sumi::Message* msg = queue_.find_any();
    if (!msg){
      sst_hg_abort_printf("polling returned null message");
    }
    incomingMessage(msg);
  }
  //mpi_queue_debug("finishing progress loop");
}

void
MpiQueue::forwardProgress(double timeout)
{
  //mpi_queue_debug("starting forward progress with timeout=%f", timeout);
  Iris::sumi::Message* msg = queue_.find_any(true, timeout); //block until timeout
  if (msg){
    incomingMessage(msg);
  }
}

void
MpiQueue::startProgressLoop(
  const std::vector<MpiRequest*>& req,
  SST::Hg::TimeDelta  /*timeout*/)
{
  startProgressLoop(req);
}

void
MpiQueue::finishProgressLoop(const std::vector<MpiRequest*>&)
{
}

void
MpiQueue::memcopy(uint64_t bytes)
{
  api_->memcopyDelay(bytes);
}

void
MpiQueue::bufferUnexpected(MpiMessage* msg)
{
  //CallGraphAppend(MPIQueueBufferUnexpectedMessage);
  api_->memcopyDelay(msg->payloadBytes());
}

}
