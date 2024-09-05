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

#include <output.h>
#include <cstring>
#include <iris/sumi/transport.h>
#include <iris/sumi/allreduce.h>
#include <iris/sumi/reduce_scatter.h>
#include <iris/sumi/reduce.h>
#include <iris/sumi/allgather.h>
#include <iris/sumi/allgatherv.h>
#include <iris/sumi/alltoall.h>
#include <iris/sumi/alltoallv.h>
#include <iris/sumi/communicator.h>
#include <iris/sumi/bcast.h>
#include <iris/sumi/gather.h>
#include <iris/sumi/scatter.h>
#include <iris/sumi/gatherv.h>
#include <iris/sumi/scatterv.h>
#include <iris/sumi/scan.h>
#include <mercury/common/stl_string.h>
#include <sst/core/params.h>
//#include <sprockit/keyword_registration.h>
#include <mercury/common/component.h>
#include <mercury/common/util.h>
//#include <sstmac/common/stats/stat_spyplot.h>
#include <mercury/common/request.h>
#include <mercury/operating_system/libraries/api.h>
#include <mercury/common/errors.h>

//RegisterKeywords(
//{ "lazy_watch", "whether failure notifications can be receive without active pinging" },
//{ "eager_cutoff", "what message size in bytes to switch from eager to rendezvous" },
//{ "use_put_protocol", "whether to use a put or get protocol for pt2pt sends" },
//{ "algorithm", "the specific algorithm to use for a given collecitve" },
//{ "comm_sync_stats", "whether to track synchronization stats for communication" },
//{ "smp_single_copy_size", "the minimum size of message for single-copy protocol" },
//{ "max_eager_msg_size", "the maximum size for using eager pt2pt protocol" },
//{ "max_vshort_msg_size", "the maximum size for mailbox protocol" },
//{ "post_rdma_delay", "the time it takes to post an RDMA operation" },
//{ "post_header_delay", "the time it takes to send an eager message" },
//{ "poll_delay", "the time it takes to poll for an incoming message" },
//{ "rdma_pin_latency", "the latency for each RDMA pin information" },
//{ "rdma_page_delay", "the per-page delay for RDMA pinning" },
//);

#include <sst/core/event.h>
#include <iris/sumi/sim_transport.h>
#include <iris/sumi/message.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/launch/app_launcher.h>
#include <mercury/components/node.h>
//#include <sstmac/common/runtime.h>
//#include <sstmac/common/stats/stat_spyplot.h>

using SST::Hg::TimeDelta;

namespace SST::Iris::sumi {

//const int options::initial_context = -2;

class SumiServer :
  public SST::Hg::Service
{

 public:
 Output output;

  SumiServer(SimTransport* tport)
    : Service(tport->serverLibname(),
       SST::Hg::SoftwareId(-1, -1), //belongs to no application
       tport->parent()->os())
  {
  }

  void registerProc(int rank, SimTransport* proc){
    int app_id = proc->sid().app_;
    SimTransport*& slot = procs_[app_id][rank];
    if (slot){
      sst_hg_abort_printf("SumiServer: already registered rank %d for app %d on node %d",
                        rank, app_id, os_->addr());
    }
    slot = proc;

    auto iter = pending_.begin();
    auto end = pending_.end();
    while (iter != end){
      auto tmp = iter++;
      Message* msg = *tmp;
      if (msg->targetRank() == rank && msg->aid() == proc->sid().app_){
        pending_.erase(tmp);
        proc->incomingMessage(msg);
      }
    }
  }

  bool unregisterProc(int rank, Transport* proc){
    int app_id = proc->sid().app_;
    auto iter = procs_.find(app_id);
    auto& subMap = iter->second;
    subMap.erase(rank);
    if (subMap.empty()){
      procs_.erase(iter);
    }
    return procs_.empty();
  }

  void incomingEvent(SST::Event* ev) override {
    SST::Hg::Service::incomingEvent(ev);
  }

  void incomingRequest(SST::Hg::Request *req) override {
    Message* smsg = safe_cast(Message, req);
    output.output("SumiServer %d: incoming %s", os_->addr(), smsg->toString().c_str());
    SimTransport* tport = procs_[smsg->aid()][smsg->targetRank()];
    if (!tport){
      output.output("SumiServer %d: message pending to app %d, target %d",
        os_->addr(), smsg->aid(), smsg->targetRank());
      pending_.push_back(smsg);
    } else {
      tport->incomingMessage(smsg);
    }
  }

  const std::map<int,SimTransport*>& getProcs(int aid) const {
    auto iter = procs_.find(aid);
    if (iter == procs_.end()){
      sst_hg_abort_printf("SumiServer got bad app id %d", aid);
    }
    return iter->second;
  }

 private:
  std::map<int, std::map<int, SimTransport*>> procs_;
  std::list<Message*> pending_;

};


Transport* Transport::get()
{
  return SST::Hg::OperatingSystem::currentThread()->getApi<sumi::SimTransport>("sumi");
}

Transport::~Transport()
{
//  if (engine_) delete engine_;
}

SST::Hg::TimeDelta
Transport::activeDelay(SST::Hg::Timestamp start)
{
  SST::Hg::Timestamp wait_start = std::max(start, last_collection_);
  SST::Hg::Timestamp _now = now();
  last_collection_ = _now;
  if (_now > wait_start){
    return _now - wait_start;
  }
  return SST::Hg::TimeDelta();
}

void
Transport::logMessageDelay(Message * /*msg*/, uint64_t /*bytes*/, int /*stage*/,
                           SST::Hg::TimeDelta /*sync_delay*/,
                           SST::Hg::TimeDelta /*active_delay*/,
                           SST::Hg::TimeDelta /*time since quiesce*/)
{
}

//void
//Transport::startCollectiveMessageLog()
//{
//  last_collection_ = now();
//}

SimTransport::SimTransport(SST::Params& params, SST::Hg::App* parent, SST::Component* comp) :
  //the name of the transport itself should be mapped to a unique name
  Transport("sumi", parent->sid(), parent->os()->addr()),
  API(params, parent, comp),
  //the server is what takes on the specified libname
  completion_queues_(1),
//  spy_bytes_(nullptr),
  default_progress_queue_(parent->os()),
  nic_ioctl_(parent->os()->nicDataIoctl()),
  qos_analysis_(nullptr),
  pragma_block_set_(false),
  pragma_timeout_(-1),
  os_(parent->os())
{
  completion_queues_[0] = std::bind(&DefaultProgressQueue::incoming,
                                    &default_progress_queue_, 0, std::placeholders::_1);
  null_completion_notify_ = std::bind(&SimTransport::drop, this, std::placeholders::_1);
  //rank_ = sid().task_;
  rank_ = os_->addr();
  auto* server_lib = parent->os()->lib(server_libname_);
  SumiServer* server;
  // only do one server per app per node
  if (server_lib == nullptr) {
    server = new SumiServer(this);
    server->start();
  } else {
    server = safe_cast(SumiServer, server_lib);
  }

  post_rdma_delay_ = TimeDelta(params.find<SST::UnitAlgebra>("post_rdma_delay", "0s").getValue().toDouble());
  post_header_delay_ = TimeDelta(params.find<SST::UnitAlgebra>("post_header_delay", "0s").getValue().toDouble());
  poll_delay_ = TimeDelta(params.find<SST::UnitAlgebra>("poll_delay", "0s").getValue().toDouble());

  rdma_pin_latency_ = TimeDelta(params.find<SST::UnitAlgebra>("rdma_pin_latency", "0s").getValue().toDouble());
  rdma_page_delay_ = TimeDelta(params.find<SST::UnitAlgebra>("rdma_page_delay", "0s").getValue().toDouble());
  pin_delay_ = rdma_pin_latency_.ticks() || rdma_page_delay_.ticks();
  page_size_ = params.find<SST::UnitAlgebra>("rdma_page_size", "4096").getRoundedValue();

  output.output("%d", sid().app_);
  nproc_ = os_->nranks();

  auto qos_params = params.get_scoped_params("qos");
  auto qos_name = qos_params.find<std::string>("name", "null");
  qos_analysis_ = SST::Hg::create<QoSAnalysis>("macro", qos_name, qos_params);

  server->registerProc(rank_, this);

  if (!engine_) engine_ = new CollectiveEngine(params, this);

  smp_optimize_ = params.find<bool>("smp_optimize", false);
}

void
SimTransport::allocateCq(int id, std::function<void(Message*)>&& f)
{
  completion_queues_[id] = std::move(f);
  auto iter = held_.find(id);
  if (iter != held_.end()){
    auto& list = iter->second;
    for (Message* m : list){
      f(m);
    }
    held_.erase(iter);
  }
}

void
SimTransport::init()
{
  if (smp_optimize_){
   engine_->barrier(-1, Message::default_cq);
   engine_->blockUntilNext(Message::default_cq);

    SumiServer* server = safe_cast(SumiServer, api_parent_app_->os()->lib(server_libname_));
    auto& map = server->getProcs(sid().app_);
    if (map.size() > 1){ //enable smp optimizations
      for (auto& pair : map){
        smp_neighbors_.insert(pair.first);
      }
    }
  }
}

void
SimTransport::finish()
{
  //this should really loop through and kill off all the pings
  //so none of them execute
}

SimTransport::~SimTransport()
{
  SumiServer* server = safe_cast(SumiServer, api_parent_app_->os()->lib(server_libname_));
  bool del = server->unregisterProc(rank_, this);
  if (del) delete server;

  if (engine_) delete engine_;

  //if (spy_bytes_) delete spy_bytes_;
  //if (spy_num_messages_) delete spy_num_messages_;
}

void
SimTransport::pinRdma(uint64_t bytes)
{
  int num_pages = bytes / page_size_;
  if (bytes % page_size_) ++num_pages;
  SST::Hg::TimeDelta pin_delay = rdma_pin_latency_ + num_pages*rdma_page_delay_;
  compute(pin_delay);
}

void
SimTransport::memcopy(void* dst, void* src, uint64_t bytes)
{
  if (isNonNullBuffer(dst) && isNonNullBuffer(src)){
    ::memcpy(dst, src, bytes);
  }
  //FIXME
  //api_parent_app_->computeBlockMemcpy(bytes);
}

void
SimTransport::memcopyDelay(uint64_t bytes)
{
  //FIXME
  //api_parent_app_->computeBlockMemcpy(bytes);
}

void
SimTransport::incomingEvent(Event * /*ev*/)
{
  sst_hg_abort_printf("sumi_transport::incoming_event: should not directly handle events");
}

int*
SimTransport::nidlist() const
{
  //just cast an int* - it's fine
  //the types are the same size and the bits can be
  //interpreted correctly
  //return (int*) rank_mapper_->rankToNode().data();
  return nullptr;
}

void
SimTransport::compute(SST::Hg::TimeDelta t)
{
  //FIXME
  //api_parent_app_->compute(t);
}


void
SimTransport::send(Message* m)
{
//  int qos = qos_analysis_->selectQoS(m);
//  m->setQoS(qos);
//  if (!m->started()){
//    m->setTimeStarted(api_parent_app_->now());
//  }

//  if (spy_bytes_){
//    switch(m->sstmac::hw::NetworkMessage::type()){
//    case sstmac::hw::NetworkMessage::smsg_send:
//    case sstmac::hw::NetworkMessage::posted_send:
//    case sstmac::hw::NetworkMessage::rdma_get_request:
//    case sstmac::hw::NetworkMessage::rdma_put_payload:
//      spy_bytes_->addData(m->recver(), m->payloadBytes());
//      break;
//    default:
//      break;
//    }
//  }

  switch(m->SST::Hg::NetworkMessage::type()){
    case SST::Hg::NetworkMessage::smsg_send:
      if (m->recver() == rank_){
        //deliver to self
        output.output("Rank %d SUMI sending self message", rank_);
        if (m->needsRecvAck()){
          completion_queues_[m->recvCQ()](m);
        }
        if (m->needsSendAck()){
          auto* ack = m->cloneInjectionAck();
          completion_queues_[m->sendCQ()](static_cast<Message*>(ack));
        }
      } else {
        if (post_header_delay_.ticks()) {
          //api_parent_app_->compute(post_header_delay_);
        }
        nic_ioctl_(m);
      }
      break;
    case SST::Hg::NetworkMessage::posted_send:
      if (post_header_delay_.ticks()) {
        //api_parent_app_->compute(post_header_delay_);
      }
      nic_ioctl_(m);
      break;
    case SST::Hg::NetworkMessage::rdma_get_request:
    case SST::Hg::NetworkMessage::rdma_put_payload:
      if (post_rdma_delay_.ticks()) {
        //api_parent_app_->compute(post_rdma_delay_);
      }
      nic_ioctl_(m);
      break;
    default:
      sst_hg_abort_printf("attempting to initiate send with invalid type %d",
                        m->type())
  }
}

void
SimTransport::smsgSendResponse(Message* m, uint64_t size, void* buffer, int local_cq, int remote_cq, int qos)
{
  //reverse both hardware and software info
  m->SST::Hg::NetworkMessage::reverse();
  m->reverse();
  m->setupSmsg(buffer, size);
  m->setSendCq(local_cq);
  m->setRecvCQ(remote_cq);
  m->setQoS(qos);
  m->SST::Hg::NetworkMessage::setType(Message::smsg_send);
  send(m);
}

void
SimTransport::rdmaGetRequestResponse(Message* m, uint64_t size,
                                     void* local_buffer, void* remote_buffer,
                                     int local_cq, int remote_cq, int qos)
{
  //do not reverse send/recver - this is hardware reverse, not software reverse
  m->SST::Hg::NetworkMessage::reverse();
  m->setupRdmaGet(local_buffer, remote_buffer, size);
  m->setSendCq(remote_cq);
  m->setRecvCQ(local_cq);
  m->setQoS(qos);
  m->SST::Hg::NetworkMessage::setType(Message::rdma_get_request);
  send(m);
}

void
SimTransport::rdmaGetResponse(Message* m, uint64_t size, int local_cq, int remote_cq, int qos)
{
  smsgSendResponse(m, size, nullptr, local_cq, remote_cq, qos);
}

void
SimTransport::rdmaPutResponse(Message* m, uint64_t payload_bytes,
                 void* loc_buffer, void* remote_buffer, int local_cq, int remote_cq, int qos)
{
  m->reverse();
  m->SST::Hg::NetworkMessage::reverse();
  m->setupRdmaPut(loc_buffer, remote_buffer, payload_bytes);
  m->setSendCq(local_cq);
  m->setRecvCQ(remote_cq);
  m->setQoS(qos);
  m->SST::Hg::NetworkMessage::setType(Message::rdma_put_payload);
  send(m);
}

uint64_t
SimTransport::allocateFlowId()
{
  return api_parent_app_->os()->allocateUniqueId();
}

void
SimTransport::incomingMessage(Message *msg)
{
//#if SSTMAC_COMM_DELAY_STATS
//  if (msg){
//    msg->setTimeArrived(api_parent_app_->now());
//  }
//#endif
  msg->writeSyncValue();
  int cq = msg->isNicAck() ? msg->sendCQ() : msg->recvCQ();
  if (cq != Message::no_ack){
    if (cq >= completion_queues_.size()){
      output.output("No CQ yet for %s", msg->toString().c_str());
      held_[cq].push_back(msg);
    } else {
      output.output("CQ %d handle %s", cq, msg->toString().c_str());
      completion_queues_[cq](msg);
    }
  } else {
    output.output("Dropping message without CQ: %s", msg->toString().c_str());
    null_completion_notify_(msg);
    delete msg;
  }
}

SST::Hg::Timestamp
SimTransport::now() const
{
  return api_parent_app_->now();
}

void*
SimTransport::allocateWorkspace(uint64_t size, void* parent)
{
  if (isNonNullBuffer(parent)){
    return ::malloc(size);
  } else {
    return sst_hg_nullptr;
  }
}

void
SimTransport::freeWorkspace(void *buf, uint64_t /*size*/)
{
  if (isNonNullBuffer(buf)){
    ::free(buf);
  }
}

CollectiveEngine::CollectiveEngine(SST::Params& params, Transport *tport) :
  tport_(tport),
  global_domain_(nullptr),
  eager_cutoff_(512),
  use_put_protocol_(false),
  system_collective_tag_(-1) //negative tags reserved for special system work
{
  global_domain_ = new GlobalCommunicator(tport);
  eager_cutoff_ = params.find<int>("eager_cutoff", 512);
  use_put_protocol_ = params.find<bool>("use_put_protocol", false);
  alltoall_type_ = params.find<std::string>("alltoall", "bruck");
  allgather_type_ = params.find<std::string>("allgather", "bruck");

  int default_qos = params.find<int>("default_qos", 0);
  rdma_get_qos_ = params.find<int>("collective_rdma_get_qos", default_qos);
  rdma_header_qos_ = params.find<int>("collective_rdma_header_qos", default_qos);
  ack_qos_ = params.find<int>("collective_ack_qos", default_qos);
  smsg_qos_ = params.find<int>("collective_smsg_qos", default_qos);
}

CollectiveEngine::~CollectiveEngine()
{
  if (global_domain_) delete global_domain_;
}

void
CollectiveEngine::notifyCollectiveDone(int rank, Collective::type_t ty, int tag)
{
  Collective* coll = collectives_[ty][tag];
  if (!coll){
    sst_hg_throw_printf(SST::Hg::ValueError,
      "transport::notify_collective_done: invalid collective of type %s, tag %d",
       Collective::tostr(ty), tag);
  }
  finishCollective(coll, rank, ty, tag);
}

void
CollectiveEngine::initSmp(const std::set<int>& /*neighbors*/)
{
}

void
CollectiveEngine::deadlockCheck()
{
  collective_map::iterator it, end = collectives_.end();
  for (it=collectives_.begin(); it != end; ++it){
    tag_to_collective_map& next = it->second;
    tag_to_collective_map::iterator cit, cend = next.end();
    for (cit=next.begin(); cit != cend; ++cit){
      Collective* coll = cit->second;
      if (!coll->complete()){
        coll->deadlockCheck();
      }
    }
  }
}

CollectiveDoneMessage*
CollectiveEngine::skipCollective(Collective::type_t ty,
  int cq_id, Communicator* comm,
  void* dst, void *src,
  int nelems, int type_size,
  int tag)
{
  if (!comm) comm = global_domain_;
  if (comm->nproc() == 1){
    tport_->memcopy(dst, src, nelems*type_size);
    return new CollectiveDoneMessage(tag, ty, comm, cq_id);
  }
  return nullptr;
}

CollectiveDoneMessage*
CollectiveEngine::allreduce(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                            int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::allreduce, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) {
    return msg;
  }

  if (!comm) comm = global_domain_;

  Collective* coll = nullptr;
  if (comm->smpComm()){
    //tags are restricted to 28 bits - the front 4 bits are mine for various internal operations
    int intra_reduce_tag = 1<<28 | tag;
    auto* intra_reduce = new WilkeHalvingAllreduce(this, dst, src, nelems,
                                   type_size, intra_reduce_tag, fxn, cq_id, comm->smpComm());

    int root = comm->smpComm()->commToGlobalRank(0);
    Collective* prev;
    if (comm->myCommRank() == root){
      if (!comm->ownerComm()){
        sst_hg_abort_printf("Bad owner comm configuration - rank 0 in SMP comm should 'own' node");
      }
      //I am the owner!
      int inter_reduce_tag = 2<<28 | tag;
      auto* inter_reduce = new WilkeHalvingAllreduce(this, dst, dst, nelems,
                                     type_size, inter_reduce_tag, fxn, cq_id, comm->ownerComm());


      intra_reduce->setSubsequent(inter_reduce);
      prev = inter_reduce;
    } else {
      prev = intra_reduce;
    }
    auto* intra_bcast = new BinaryTreeBcastCollective(this, root, dst, nelems, type_size, tag,
                                                      cq_id, comm->smpComm());
    prev->setSubsequent(intra_bcast);
    //this should report back as done on the original communicator!
    coll = new DoNothingCollective(this, tag, cq_id, comm);
    intra_bcast->setSubsequent(coll);
  } else {
    coll = new WilkeHalvingAllreduce(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  }

  return startCollective(coll);
}

sumi::CollectiveDoneMessage*
CollectiveEngine::reduceScatter(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                                  int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::reduce_scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new HalvingReduceScatter(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}

sumi::CollectiveDoneMessage*
CollectiveEngine::scan(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                        int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::scan, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new SimultaneousBtreeScan(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}


CollectiveDoneMessage*
CollectiveEngine::reduce(int root, void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                          int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::reduce, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new WilkeHalvingReduce(this, root, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::bcast(int root, void *buf, int nelems, int type_size, int tag,
                         int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::bcast, cq_id, comm, buf, buf, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BinaryTreeBcastCollective(this, root, buf, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::gatherv(int root, void *dst, void *src,
                   int sendcnt, int *recv_counts,
                   int type_size, int tag, int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::gatherv, cq_id, comm, dst, src, sendcnt, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeGatherv(this, root, dst, src, sendcnt, recv_counts, type_size, tag, cq_id, comm);
  SST::Hg::abort("gatherv");
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::gather(int root, void *dst, void *src, int nelems, int type_size, int tag,
                          int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::gather, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeGather(this, root, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::scatter(int root, void *dst, void *src, int nelems, int type_size, int tag,
                           int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeScatter(this, root, dst, src, nelems, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::scatterv(int root, void *dst, void *src, int* send_counts, int recvcnt, int type_size, int tag,
                            int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::scatterv, cq_id, comm, dst, src, recvcnt, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BtreeScatterv(this, root, dst, src, send_counts, recvcnt, type_size, tag, cq_id, comm);
  SST::Hg::abort("scatterv");
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::alltoall(void *dst, void *src, int nelems, int type_size, int tag,
                            int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::alltoall, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) {
    return msg;
  }

  if (!comm) comm = global_domain_;

  if (comm->smpComm() && comm->smpBalanced()){
    int smpSize = comm->smpComm()->nproc();
    void* intraDst = dst ? new char[nelems*type_size*smpSize] : nullptr;
    int intra_tag = 1<<28 | tag;

    BtreeGather* intra = new BtreeGather(this, 0, intraDst, src, smpSize*nelems,
                                         type_size, intra_tag, cq_id, comm->smpComm());
    DagCollective* prev;
    if (comm->ownerComm()){
      int inter_tag = 2<<28 | tag;
      AllToAllCollective* inter;
      if (alltoall_type_ == "bruck") {
        inter = (AllToAllCollective*) new BruckAlltoallCollective(this, dst, intraDst,
                                                             smpSize*nelems, type_size, inter_tag, cq_id, comm->ownerComm());
      }
      else if (alltoall_type_ == "direct") {
        inter = (AllToAllCollective*) new DirectAlltoallCollective(this, dst, intraDst, smpSize*nelems,
                                                              type_size, inter_tag, cq_id, comm->ownerComm());
      }
      else {
        sst_hg_abort_printf("unrecognized alltoall type");
      }
      intra->setSubsequent(inter);
      prev = inter;
    } else {
      std::cerr << "prev = intra = " << intra << std::endl;
      prev = intra;
    }
    int bcast_tag = 3<<28 | tag;
    auto* bcast = new BinaryTreeBcastCollective(this, 0, dst, comm->nproc()*nelems,
                                                type_size, bcast_tag, cq_id, comm->smpComm());
    prev->setSubsequent(bcast);
    auto* final = new DoNothingCollective(this, tag, cq_id, comm);
    bcast->setSubsequent(final);
    return startCollective(intra);
  } else {
    AllToAllCollective* coll;
    if (alltoall_type_ == "bruck") {
      coll = (AllToAllCollective*) new BruckAlltoallCollective(this, dst, src, nelems, type_size, tag, cq_id, comm);
    }
    else if (alltoall_type_ == "direct") {
      coll = (AllToAllCollective*) new DirectAlltoallCollective(this, dst, src, nelems,
                                                            type_size, tag, cq_id, comm);
    }
    else {
      sst_hg_abort_printf("unrecognized alltoall type");
    }
    return startCollective(coll);
  }
  return nullptr;
}

CollectiveDoneMessage*
CollectiveEngine::alltoallv(void *dst, void *src, int* send_counts, int* recv_counts, int type_size, int tag,
                             int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::alltoallv, cq_id, comm, dst, src, send_counts[0], type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new DirectAlltoallvCollective(this, dst, src, send_counts, recv_counts, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::allgather(void *dst, void *src, int nelems, int type_size, int tag,
                             int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::allgather, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;

  if (comm->smpComm() && comm->smpBalanced()){
    int smpSize = comm->smpComm()->nproc();
    void* intraDst = dst ? new char[nelems*type_size*smpSize] : nullptr;

    int intra_tag = 1<<28 | tag;

    AllgatherCollective* intra;
    if (allgather_type_ == "bruck") {
      intra = (AllgatherCollective*) new BruckAllgatherCollective(this, intraDst, src, nelems, type_size,
                                                                intra_tag, cq_id, comm->smpComm());
    }
    else if (allgather_type_ == "ring") {
      intra = (AllgatherCollective*) new RingAllgatherCollective(this, intraDst, src, nelems, type_size,
                                                                intra_tag, cq_id, comm->smpComm());
    }
    else {
      sst_hg_abort_printf("unrecognized allgather type");
    }

    DagCollective* prev;
    if (comm->ownerComm()){
      int inter_tag = 2<<28 | tag;
      AllgatherCollective* inter;
      if (allgather_type_ == "bruck") {
        intra = (AllgatherCollective*) new BruckAllgatherCollective(this, dst, intraDst, smpSize*nelems, type_size,
                                                                  inter_tag, cq_id, comm->ownerComm());
      }
      else if (allgather_type_ == "ring") {
        intra = (AllgatherCollective*) new RingAllgatherCollective(this, dst, intraDst, smpSize*nelems, type_size,
                                                                 inter_tag, cq_id, comm->ownerComm());
      }
      else {
        sst_hg_abort_printf("unrecognized allgather type");
      }
      intra->setSubsequent(inter);
      prev = inter;
    } else {
      prev = intra;
    }
    int bcast_tag = 3<<28 | tag;
    auto* bcast = new BinaryTreeBcastCollective(this, 0, dst, comm->nproc()*nelems,
                                                type_size, bcast_tag, cq_id, comm->smpComm());
    prev->setSubsequent(bcast);
    auto* final = new DoNothingCollective(this, tag, cq_id, comm);
    bcast->setSubsequent(final);
    return startCollective(intra);
  }
  else {
    AllgatherCollective* coll;
    if (allgather_type_ == "bruck") {
      coll = (AllgatherCollective*) new BruckAllgatherCollective(this, dst, src, nelems, type_size, tag, cq_id, comm);
    }
    else if (allgather_type_ == "ring") {
      coll = (AllgatherCollective*) new RingAllgatherCollective(this, dst, src, nelems, type_size, tag, cq_id, comm);
    }
    else {
      sst_hg_abort_printf("unrecognized allgather type");
    }
    return startCollective(coll);
  }
  return nullptr;
}

CollectiveDoneMessage*
CollectiveEngine::allgatherv(void *dst, void *src, int* recv_counts, int type_size, int tag,
                              int cq_id, Communicator* comm)
{
  //if the allgatherv is skipped, we have a single recv count
  int nelems = *recv_counts;
  auto* msg = skipCollective(Collective::allgatherv, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckAllgathervCollective(this, dst, src, recv_counts, type_size, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::barrier(int tag, int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::barrier, cq_id, comm, 0, 0, 0, 0, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  DagCollective* coll = new BruckBarrierCollective(this, nullptr, nullptr, tag, cq_id, comm);
  return startCollective(coll);
}

CollectiveDoneMessage*
CollectiveEngine::deliverPending(Collective* coll, int tag, Collective::type_t ty)
{
  std::list<CollectiveWorkMessage*> pending = pending_collective_msgs_[ty][tag];
  pending_collective_msgs_[ty].erase(tag);
  CollectiveDoneMessage* dmsg = nullptr;
  for (auto* msg : pending){
    dmsg = coll->recv(msg);
  }
  return dmsg;
}

void
CollectiveEngine::validateCollective(Collective::type_t ty, int tag)
{
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);
  if (it == collectives_[ty].end()){
    return; // all good
  }

  Collective* coll = it->second;
  if (!coll){
   sst_hg_throw_printf(SST::Hg::IllformedError,
    "sumi_api::validate_collective: lingering null collective of type %s with tag %d",
    Collective::tostr(ty), tag);
  }

  if (coll->persistent() && coll->complete()){
    return; // all good
  }

  sst_hg_throw_printf(SST::Hg::IllformedError,
    "sumi_api::validate_collective: cannot overwrite collective of type %s with tag %d",
    Collective::tostr(ty), tag);
}

CollectiveDoneMessage*
CollectiveEngine::startCollective(Collective* coll)
{
  if (coll->type() == Collective::donothing){
    todel_.push_back(coll);
    return new CollectiveDoneMessage(coll->tag(), coll->type(), coll->comm(), coll->cqId());
  }

  coll->initActors();
  int tag = coll->tag();
  Collective::type_t ty = coll->type();
  Collective*& map_entry = collectives_[ty][tag];
  Collective* active = nullptr;
  CollectiveDoneMessage* dmsg=nullptr;
  if (map_entry){
    active = map_entry;
    coll->start();
    dmsg = active->addActors(coll);
    delete coll;
  } else {
    map_entry = active = coll;
    coll->start();
    dmsg = deliverPending(coll, tag, ty);
  }

  while (dmsg && active->hasSubsequent()){
    delete dmsg;
    active = active->popSubsequent();
    dmsg = startCollective(active);
  }

  return dmsg;
}

void
CollectiveEngine::finishCollective(Collective* coll, int rank, Collective::type_t ty, int tag)
{
  bool deliver_cq_msg; bool delete_collective;
  coll->actorDone(rank, deliver_cq_msg, delete_collective);
//  debug_printf(sprockit::dbg::sumi,
//    "Rank %d finishing collective of type %s tag %d - deliver=%d",
//    tport_->rank(), Collective::tostr(ty), tag, deliver_cq_msg);

  if (!deliver_cq_msg)
    return;

  coll->complete();
  if (delete_collective && !coll->persistent()){ //otherwise collective must exist FOREVER
    collectives_[ty].erase(tag);
    todel_.push_back(coll);
  }

  pending_collective_msgs_[ty].erase(tag);
//  debug_printf(sprockit::dbg::sumi,
//    "Rank %d finished collective of type %s tag %d",
//    tport_->rank(), Collective::tostr(ty), tag);
}

void
CollectiveEngine::waitBarrier(int tag)
{
  if (tport_->nproc() == 1) return;
  barrier(tag, Message::default_cq);
  blockUntilNext(Message::default_cq);
}

void
CollectiveEngine::cleanUp()
{
  for (Collective* coll : todel_){
    delete coll;
  }
  todel_.clear();
}

CollectiveDoneMessage*
CollectiveEngine::incoming(Message* msg)
{
  cleanUp();

  CollectiveWorkMessage* cmsg = dynamic_cast<CollectiveWorkMessage*>(msg);
  if (cmsg->sendCQ() == -1 && cmsg->recvCQ() == -1){
    sst_hg_abort_printf("both CQs are invalid for %s", msg->toString().c_str())
  }
  int tag = cmsg->tag();
  Collective::type_t ty = cmsg->type();
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);

  if (it == collectives_[ty].end()){
//    debug_printf(sprockit::dbg::sumi_collective,
//      "Rank %d, queuing %p %s from %d on tag %d for type %s",
//      tport_->rank(), msg,
//      Message::tostr(msg->classType()),
//      msg->sender(),
//      tag, Collective::tostr(ty));
      //message for collective we haven't started yet
      pending_collective_msgs_[ty][tag].push_back(cmsg);
      return nullptr;
  } 

  Collective* coll = it->second;
  auto* dmsg = coll->recv(cmsg);
  while (dmsg && coll->hasSubsequent()){
    delete dmsg;
    coll = coll->popSubsequent();
    dmsg = startCollective(coll);
  }
  return dmsg;
}

CollectiveDoneMessage*
CollectiveEngine::blockUntilNext(int cq_id)
{
  CollectiveDoneMessage* dmsg = nullptr;
  while (dmsg == nullptr){
//    debug_printf(sprockit::dbg::sumi_collective,
//      "Rank %d, blocking collective until next message arrives on CQ %d", tport_->rank(), cq_id);
    auto* msg = tport_->blockingPoll(cq_id);
//    debug_printf(sprockit::dbg::sumi_collective,
//      "Rank %d, unblocking collective on CQ %d", tport_->rank(), cq_id);
    dmsg = incoming(msg);
  }
//  debug_printf(sprockit::dbg::sumi_collective,
//    "Rank %d, exiting collective progress on CQ %d", tport_->rank(), cq_id);
  return dmsg;
}

class NullQoSAnalysis : public QoSAnalysis
{
 public:
  SST_ELI_REGISTER_DERIVED(
    QoSAnalysis,
    NullQoSAnalysis,
    "macro",
    "null",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "perform a QoS analyis based on pattern")

  NullQoSAnalysis(SST::Params& params) :
    QoSAnalysis(params)
  {

  }

  int selectQoS(Message *m) override {
    return m->qos();
  }

  void logDelay(SST::Hg::TimeDelta /*delay*/, Message * /*m*/) override {

  }

};

class PatternQoSAnalysis : public QoSAnalysis
{
 public:
  SST_ELI_REGISTER_DERIVED(
    QoSAnalysis,
    PatternQoSAnalysis,
    "macro",
    "pattern",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "perform a QoS analyis based on pattern")

  PatternQoSAnalysis(SST::Params& params) :
    QoSAnalysis(params)
  {
    rtLatency_ = TimeDelta(params.find<SST::UnitAlgebra>("rt_latency").getValue().toDouble());
    eagerLatency_ = TimeDelta(params.find<SST::UnitAlgebra>("eager_latency").getValue().toDouble());
    rdmaLatency_ = TimeDelta(params.find<SST::UnitAlgebra>("rdma_latency").getValue().toDouble());
    byteDelay_ = TimeDelta(params.find<SST::UnitAlgebra>("bandwidth").getValue().inverse().toDouble());
    rdmaCutoff_ = params.find<SST::UnitAlgebra>("rdma_cutoff").getRoundedValue();
  }

  int selectQoS(Message * /*m*/) override {
    return 0;
  }

  void logDelay(SST::Hg::TimeDelta delay, Message *m) override {
    SST::Hg::TimeDelta acceptable_delay = allowedDelay_ + m->byteLength() * byteDelay_;
    if (m->byteLength() > rdmaCutoff_){
      acceptable_delay += rdmaLatency_ + 2*eagerLatency_ + 3*rtLatency_;
    } else {
      acceptable_delay += eagerLatency_ + rtLatency_;
    }

    printf("Message %12lu: %2u->%2u %8llu %10.4e %10.4e\n",
       m->hash(), m->sender(), m->recver(), static_cast<unsigned long long>(m->byteLength()),
       delay.sec(), acceptable_delay.sec());

  }

 private:
  uint32_t rdmaCutoff_;
  SST::Hg::TimeDelta eagerLatency_;
  SST::Hg::TimeDelta rdmaLatency_;
  SST::Hg::TimeDelta byteDelay_;
  SST::Hg::TimeDelta rtLatency_;
  SST::Hg::TimeDelta allowedDelay_;
};

extern "C" void sst_hg_blocking_call(int condition, double timeout, const char* api_name)
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  auto* api = t->getApi<sumi::SimTransport>(api_name);
  api->setPragmaBlocking(condition, timeout);
}


} // end namespace SST::Iris::sumi
