// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/params.h>
#include <sst/core/event.h>
#include <sst/core/factory.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/allreduce.h>
#include <iris/sumi/ring_allreduce.h>
#include <iris/sumi/collective_registry.h>
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
#include <iris/sumi/sim_transport.h>
#include <iris/sumi/message.h>
#include <mercury/common/stl_string.h>
#include <mercury/common/component.h>
#include <mercury/common/util.h>
#include <mercury/common/request.h>
#include <mercury/common/errors.h>
#include <mercury/common/output.h>
#include <mercury/components/operating_system.h>
#include <mercury/components/node.h>
#include <mercury/operating_system/libraries/library.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/launch/app_launcher.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using SST::Hg::TimeDelta;

namespace SST::Iris::sumi {

//const int options::initial_context = -2;

class SumiServer :
  public SST::Hg::Service
{

 public:
 //const std::unique_ptr<SST::Output> & out_;

//SumiServer(SimTransport* tport, std::unique_ptr<SST::Output> & output)
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
    //if (out_) out_->debug(CALL_INFO, 1, 0, "SumiServer %d: incoming %s\n", os_->addr(), smsg->toString().c_str());
    SimTransport* tport = procs_[smsg->aid()][smsg->targetRank()];
    if (!tport){
      //if  (out_) out_->debug(CALL_INFO, 1, 0, "SumiServer %d: message pending to app %d, target %d\n",
      //  os_->addr(), smsg->aid(), smsg->targetRank());
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
  return SST::Hg::OperatingSystem::currentThread()->getLibrary<sumi::SimTransport>("sumi");
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

SimTransport::SimTransport(SST::Params& params, SST::Hg::App* parent) :
  //the name of the transport itself should be mapped to a unique name
  Transport("sumi", parent->sid(), parent->os()->addr()),
  Library(params, parent),
  //the server is what takes on the specified libname
  completion_queues_(1),
  default_progress_queue_(parent->os()),
  nic_ioctl_(parent->os()->nicDataIoctl()),
  qos_analysis_(nullptr),
  pragma_block_set_(false),
  pragma_timeout_(-1),
  os_(parent->os())
{
  unsigned int verbose = params.find<unsigned int>("verbose", 0);
  out_ = std::unique_ptr<SST::Output>(
      new SST::Output("sumi::SimTransport:", verbose, 0, Output::STDOUT));

  completion_queues_[0] = std::bind(&DefaultProgressQueue::incoming,
                                    &default_progress_queue_, 0, std::placeholders::_1);
  null_completion_notify_ = std::bind(&SimTransport::drop, this, std::placeholders::_1);
  rank_ = parent->get_taskid();
  auto* server_lib = parent->os()->eventLibrary(server_libname_);
  SumiServer* server;
  // only do one server per app per node
  if (server_lib == nullptr) {
    //server = new SumiServer(this, out_);
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
  page_size_ = params.find<SST::UnitAlgebra>("rdma_page_size", "4096 B").getRoundedValue();

  output.output("%d", sid().app_);
  nproc_ = os_->numRanks();

  auto qos_params = params.get_scoped_params("qos");
  auto qos_name = qos_params.find<std::string>("name", "null");
  if (qos_name != "null") {
    auto factory = Factory::getFactory();
    qos_analysis_ = factory->Create<QoSAnalysis>("sumi." + qos_name, qos_params);
  }

  out_->debug(CALL_INFO, 1, 0,
              "registering proc %d\n", rank_);
  server->registerProc(rank_, this);

  if (!engine_) engine_ = new CollectiveEngine(params, this);

  smp_optimize_ = params.find<bool>("smp_optimize", false);

  compute_api_ =  dynamic_cast<SST::Hg::ComputeAPI*>(api_parent_app_->getLibrary("ComputeLibrary"));
}

void
SimTransport::allocateCq(int id, std::function<void(Message*)>&& f)
{
  completion_queues_[id] = std::move(f);
  auto iter = held_.find(id);
  if (iter != held_.end()){
    auto& list = iter->second;
    for (Message* m : list){
      completion_queues_[id](m);
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

    SumiServer* server = safe_cast(SumiServer, api_parent_app_->os()->eventLibrary(server_libname_));
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
  SumiServer* server = safe_cast(SumiServer, api_parent_app_->os()->eventLibrary(server_libname_));
  bool del = server->unregisterProc(rank_, this);
  if (del) delete server;

  if (engine_) delete engine_;
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
  compute_api_->computeBlockMemcpy(bytes);
}

void
SimTransport::memcopyDelay(uint64_t bytes)
{
  compute_api_->computeBlockMemcpy(bytes);
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
  sst_hg_abort_printf("sumi_transport::nidList unimplemented");
  return nullptr;
}

void
SimTransport::compute(SST::Hg::TimeDelta t)
{
  compute_api_->compute(t);
}


void
SimTransport::send(Message* m)
{
  switch(m->SST::Hg::NetworkMessage::type()){
    case SST::Hg::NetworkMessage::smsg_send:
      if (m->recver() == rank_){
        //deliver to self
        out_->debug(CALL_INFO, 1, 0,
          "Rank %d SUMI sending self message\n", rank_);
        if (m->needsRecvAck()){
          completion_queues_[m->recvCQ()](m);
        }
        if (m->needsSendAck()){
          auto* ack = m->cloneInjectionAck();
          completion_queues_[m->sendCQ()](static_cast<Message*>(ack));
        }
      } else {
        out_->debug(CALL_INFO, 1, 0,
          "Rank %d SUMI smsg_send to %d\n", rank_, m->recver());
        if (post_header_delay_.ticks()) {
          compute_api_->compute(post_header_delay_);
        }
        nic_ioctl_(m);
      }
      break;
    case SST::Hg::NetworkMessage::posted_send:
      if (post_header_delay_.ticks()) {
        compute_api_->compute(post_header_delay_);
      }
      nic_ioctl_(m);
      break;
    case SST::Hg::NetworkMessage::rdma_get_request:
    case SST::Hg::NetworkMessage::rdma_put_payload:
      if (post_rdma_delay_.ticks()) {
        compute_api_->compute(post_rdma_delay_);
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
  auto id = api_parent_app_->os()->allocateUniqueId().msg_num;
  return id;
}

void
SimTransport::incomingMessage(Message *msg)
{
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

namespace {
// Registry-selectable operations and their configuration keys.
enum class AlgTarget {
  operation,
  allreduce_intra,
  allreduce_inter,
  allreduce_bcast,
};
struct AlgKeys {
  Collective::type_t type;
  const char* param_key;
  const char* env_key;
  AlgTarget target;
};
constexpr AlgKeys alg_keys[] = {
  { Collective::allgather, "collective.allgather", "SUMI_ALLGATHER_ALG",
    AlgTarget::operation },
  { Collective::allreduce, "collective.allreduce", "SUMI_ALLREDUCE_ALG",
    AlgTarget::operation },
  { Collective::alltoall, "collective.alltoall", "SUMI_ALLTOALL_ALG",
    AlgTarget::operation },
  { Collective::reduce, "collective.reduce", "SUMI_REDUCE_ALG",
    AlgTarget::operation },
  { Collective::bcast, "collective.bcast", "SUMI_BCAST_ALG",
    AlgTarget::operation },
  // reduce_scatter is deliberately absent: no algorithm is registered yet
  // (the built-in HalvingReduceScatter is an abort stub), so advertising a
  // key would offer a param whose every value aborts. Restore the row when
  // a real implementation registers itself.
  { Collective::scan, "collective.scan", "SUMI_SCAN_ALG",
    AlgTarget::operation },
  { Collective::gather, "collective.gather", "SUMI_GATHER_ALG",
    AlgTarget::operation },
  { Collective::scatter, "collective.scatter", "SUMI_SCATTER_ALG",
    AlgTarget::operation },
  { Collective::allreduce, "collective.allreduce.intra",
    "SUMI_ALLREDUCE_INTRA_ALG", AlgTarget::allreduce_intra },
  { Collective::allreduce, "collective.allreduce.inter",
    "SUMI_ALLREDUCE_INTER_ALG", AlgTarget::allreduce_inter },
  { Collective::bcast, "collective.allreduce.broadcast",
    "SUMI_ALLREDUCE_BCAST_ALG", AlgTarget::allreduce_bcast },
};

constexpr char hierarchy_key[] = "collective.allreduce.hierarchy";
constexpr char collective_prefix[] = "collective.";

struct LegacyParam {
  const char* key;
  const char* replacement;
};
constexpr LegacyParam legacy_params[] = {
  { "allgather", "collective.allgather" },
  { "alltoall", "collective.alltoall" },
};

const AlgKeys* algKeysFor(Collective::type_t ty)
{
  for (const auto& k : alg_keys)
    if (k.type == ty && k.target == AlgTarget::operation) return &k;
  return nullptr;
}

const AlgKeys* algKeysForParam(const std::string& param)
{
  for (const auto& k : alg_keys)
    if (param == k.param_key) return &k;
  return nullptr;
}

void rejectLegacyParams(SST::Params& params)
{
  for (const auto& k : legacy_params){
    if (params.contains(k.key)){
      sst_hg_abort_printf("parameter \"%s\" has been replaced by \"%s\"",
                          k.key, k.replacement);
    }
  }
}

void validateCollectiveParams(SST::Params& params)
{
  for (const auto& key : params.getKeys()){
    if (key.compare(0, sizeof(collective_prefix) - 1,
                    collective_prefix) != 0){
      continue;
    }
    if (key == hierarchy_key || algKeysForParam(key)) continue;

    std::string supported = hierarchy_key;
    for (const auto& k : alg_keys){
      supported += ", ";
      supported += k.param_key;
    }
    sst_hg_abort_printf("unknown collective parameter '%s'; supported "
                        "parameters: %s", key.c_str(), supported.c_str());
  }
}

// Resolve parameter, then environment; empty means no override.
std::string resolveAlgName(SST::Params& params, const char* param_key,
                           const char* env_key)
{
  std::string v = params.find<std::string>(param_key, "");
  if (!v.empty()) return v;
  const char* e = std::getenv(env_key);
  if (e && *e) return std::string(e);
  return std::string();
}

CollectiveFactoryArgs makeArgs(CollectiveEngine* engine, void* dst, void* src,
    int root, int nelems, int type_size, int tag, int cq_id,
    reduce_fxn fxn, Communicator* comm)
{
  CollectiveFactoryArgs a;
  a.engine = engine; a.dst = dst; a.src = src; a.root = root;
  a.nelems = nelems; a.type_size = type_size; a.tag = tag;
  a.cq_id = cq_id; a.fxn = fxn; a.comm = comm;
  return a;
}
}

CollectiveEngine::CollectiveEngine(SST::Params& params, Transport *tport) :
  tport_(tport),
  global_domain_(nullptr),
  eager_cutoff_(512),
  use_put_protocol_(false),
  system_collective_tag_(-1), //negative tags reserved for special system work
  allreduce_hierarchy_(AllreduceHierarchy::automatic),
  warned_no_smp_hierarchy_(false)
{
  global_domain_ = new GlobalCommunicator(tport);
  eager_cutoff_ = params.find<int>("eager_cutoff", 512);
  use_put_protocol_ = params.find<bool>("use_put_protocol", false);
  rejectLegacyParams(params);
  validateCollectiveParams(params);

  std::string hierarchy = params.find<std::string>(hierarchy_key, "auto");
  if (hierarchy == "auto"){
    allreduce_hierarchy_ = AllreduceHierarchy::automatic;
  } else if (hierarchy == "hierarchical"){
    allreduce_hierarchy_ = AllreduceHierarchy::hierarchical;
  } else if (hierarchy == "flat"){
    allreduce_hierarchy_ = AllreduceHierarchy::flat;
  } else {
    sst_hg_abort_printf("unknown %s '%s'; expected auto, hierarchical, or flat",
                        hierarchy_key, hierarchy.c_str());
  }

  // Parameters override their matching environment variables.
  for (const auto& k : alg_keys){
    std::string alg = resolveAlgName(params, k.param_key, k.env_key);
    switch (k.target){
      case AlgTarget::operation: alg_names_[k.type] = alg; break;
      case AlgTarget::allreduce_intra: allreduce_intra_alg_ = alg; break;
      case AlgTarget::allreduce_inter: allreduce_inter_alg_ = alg; break;
      case AlgTarget::allreduce_bcast: allreduce_bcast_alg_ = alg; break;
    }
  }

  if (allreduce_hierarchy_ == AllreduceHierarchy::flat &&
      (!allreduce_intra_alg_.empty() || !allreduce_inter_alg_.empty() ||
       !allreduce_bcast_alg_.empty())){
    sst_hg_abort_printf("%s=flat cannot be combined with "
                        "collective.allreduce.intra, collective.allreduce.inter, "
                        "or collective.allreduce.broadcast", hierarchy_key);
  }

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

std::string
CollectiveEngine::engineAlgName(Collective::type_t ty) const
{
  auto it = alg_names_.find(ty);
  return it == alg_names_.end() ? std::string() : it->second;
}

Collective*
CollectiveEngine::makeRegisteredCollective(Collective::type_t ty,
    void* dst, void* src, int root, int nelems, int type_size, int tag,
    int cq_id, reduce_fxn fxn, Communicator* comm,
    const std::string& selected_alg, const char* fallback_alg,
    const char* param_key, const char* env_key)
{
  std::string alg = selected_alg;
  if (alg.empty() && fallback_alg) alg = fallback_alg;
  if (alg.empty()) {
    sst_hg_abort_printf("no algorithm selected for collective '%s'",
                        Collective::tostr(ty));
  }
  auto args = makeArgs(this, dst, src, root, nelems, type_size, tag, cq_id, fxn, comm);
  Collective* c = CollectiveRegistry::make(ty, alg, args);
  if (!c){
    const AlgKeys* keys = algKeysFor(ty);
    sst_hg_abort_printf("unknown %s algorithm '%s' (param %s / env %s)",
                        Collective::tostr(ty), alg.c_str(),
                        param_key ? param_key : (keys ? keys->param_key : "?"),
                        env_key ? env_key : (keys ? keys->env_key : "?"));
  }
  return c;
}

CollectiveDoneMessage*
CollectiveEngine::startCollectiveOp(Collective::type_t ty,
    void* dst, void* src, int root, int nelems, int type_size, int tag,
    int cq_id, reduce_fxn fxn, Communicator* comm,
    const std::function<Collective*()>& makeDefault)
{
  std::string alg = engineAlgName(ty);
  if (!alg.empty()){
    Collective* c = makeRegisteredCollective(ty, dst, src, root, nelems,
        type_size, tag, cq_id, fxn, comm, alg, nullptr);
    return startCollective(c);
  }
  return startCollective(makeDefault());
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

  if (allreduce_hierarchy_ == AllreduceHierarchy::hierarchical &&
      !comm->smpComm() &&
      (comm->smpInitialized() || tport_->smpNeighbors().empty()) &&
      !warned_no_smp_hierarchy_){
    warned_no_smp_hierarchy_ = true;
    if (comm->myCommRank() == 0){
      fprintf(stderr, "warning: no SMP communicator for hierarchical "
                      "all-reduce; using flat\n");
    }
  }

  bool hierarchical = allreduce_hierarchy_ != AllreduceHierarchy::flat &&
                      comm->smpComm();
  if (!hierarchical){
    Collective* flat = makeRegisteredCollective(Collective::allreduce,
        dst, src, 0, nelems, type_size, tag, cq_id, fxn, comm,
        engineAlgName(Collective::allreduce), "recdouble");
    return startCollective(flat);
  }

  int intra_tag = 1<<28 | tag;
  Collective* intra = makeRegisteredCollective(Collective::allreduce,
      dst, src, 0, nelems, type_size, intra_tag, cq_id, fxn,
      comm->smpComm(), allreduce_intra_alg_, "recdouble",
      "collective.allreduce.intra", "SUMI_ALLREDUCE_INTRA_ALG");

  Collective* prev = intra;
  if (comm->ownerComm() && comm->ownerComm()->nproc() > 1){
    int inter_tag = 2<<28 | tag;
    std::string inter_alg = allreduce_inter_alg_.empty()
        ? engineAlgName(Collective::allreduce) : allreduce_inter_alg_;
    Collective* inter = makeRegisteredCollective(Collective::allreduce,
        dst, dst, 0, nelems, type_size, inter_tag, cq_id, fxn,
        comm->ownerComm(), inter_alg, "recdouble",
        allreduce_inter_alg_.empty() ? nullptr : "collective.allreduce.inter",
        allreduce_inter_alg_.empty() ? nullptr : "SUMI_ALLREDUCE_INTER_ALG");
    intra->setSubsequent(inter);
    prev = inter;
  }

  int bcast_tag = 3<<28 | tag;
  std::string bcast_alg = allreduce_bcast_alg_.empty()
      ? engineAlgName(Collective::bcast) : allreduce_bcast_alg_;
  Collective* bcast = makeRegisteredCollective(Collective::bcast,
      dst, dst, 0, nelems, type_size, bcast_tag, cq_id, nullptr,
      comm->smpComm(), bcast_alg, "btree",
      allreduce_bcast_alg_.empty() ? nullptr : "collective.allreduce.broadcast",
      allreduce_bcast_alg_.empty() ? nullptr : "SUMI_ALLREDUCE_BCAST_ALG");
  prev->setSubsequent(bcast);

  auto* final = new DoNothingCollective(this, tag, cq_id, comm);
  bcast->setSubsequent(final);
  return startCollective(intra);
}

sumi::CollectiveDoneMessage*
CollectiveEngine::reduceScatter(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                                  int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::reduce_scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  return startCollectiveOp(Collective::reduce_scatter, dst, src, 0, nelems, type_size, tag, cq_id, fxn, comm,
      [&]{ return new HalvingReduceScatter(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm); });
}

sumi::CollectiveDoneMessage*
CollectiveEngine::scan(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                        int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::scan, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  return startCollectiveOp(Collective::scan, dst, src, 0, nelems, type_size, tag, cq_id, fxn, comm,
      [&]{ return new SimultaneousBtreeScan(this, dst, src, nelems, type_size, tag, fxn, cq_id, comm); });
}


CollectiveDoneMessage*
CollectiveEngine::reduce(int root, void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
                          int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::reduce, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  return startCollectiveOp(Collective::reduce, dst, src, root, nelems, type_size, tag, cq_id, fxn, comm,
      [&]{ return new WilkeHalvingReduce(this, root, dst, src, nelems, type_size, tag, fxn, cq_id, comm); });
}

CollectiveDoneMessage*
CollectiveEngine::bcast(int root, void *buf, int nelems, int type_size, int tag,
                         int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::bcast, cq_id, comm, buf, buf, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  return startCollectiveOp(Collective::bcast, buf, buf, root, nelems, type_size, tag, cq_id, nullptr, comm,
      [&]{ return new BinaryTreeBcastCollective(this, root, buf, nelems, type_size, tag, cq_id, comm); });
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
  return startCollectiveOp(Collective::gather, dst, src, root, nelems, type_size, tag, cq_id, nullptr, comm,
      [&]{ return new BtreeGather(this, root, dst, src, nelems, type_size, tag, cq_id, comm); });
}

CollectiveDoneMessage*
CollectiveEngine::scatter(int root, void *dst, void *src, int nelems, int type_size, int tag,
                           int cq_id, Communicator* comm)
{
  auto* msg = skipCollective(Collective::scatter, cq_id, comm, dst, src, nelems, type_size, tag);
  if (msg) return msg;

  if (!comm) comm = global_domain_;
  return startCollectiveOp(Collective::scatter, dst, src, root, nelems, type_size, tag, cq_id, nullptr, comm,
      [&]{ return new BtreeScatter(this, root, dst, src, nelems, type_size, tag, cq_id, comm); });
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
    Collective* prev;
    if (comm->ownerComm()){
      int inter_tag = 2<<28 | tag;
      Collective* inter = makeRegisteredCollective(Collective::alltoall,
          dst, intraDst, 0, smpSize*nelems, type_size, inter_tag, cq_id,
          nullptr, comm->ownerComm(), engineAlgName(Collective::alltoall),
          "bruck");
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
    Collective* coll = makeRegisteredCollective(Collective::alltoall,
        dst, src, 0, nelems, type_size, tag, cq_id, nullptr, comm,
        engineAlgName(Collective::alltoall), "bruck");
    return startCollective(coll);
  }
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

    Collective* intra = makeRegisteredCollective(Collective::allgather,
        intraDst, src, 0, nelems, type_size, intra_tag, cq_id, nullptr,
        comm->smpComm(), engineAlgName(Collective::allgather), "bruck");

    Collective* prev;
    if (comm->ownerComm()){
      int inter_tag = 2<<28 | tag;
      Collective* inter = makeRegisteredCollective(Collective::allgather,
          dst, intraDst, 0, smpSize*nelems, type_size, inter_tag, cq_id,
          nullptr, comm->ownerComm(), engineAlgName(Collective::allgather),
          "bruck");
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
    Collective* coll = makeRegisteredCollective(Collective::allgather,
        dst, src, 0, nelems, type_size, tag, cq_id, nullptr, comm,
        engineAlgName(Collective::allgather), "bruck");
    return startCollective(coll);
  }
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

extern "C" void sst_hg_blocking_call(int condition, double timeout, const char* lib_name)
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  auto* lib = t->getLibrary<sumi::SimTransport>(lib_name);
  lib->setPragmaBlocking(condition, timeout);
}


} // end namespace SST::Iris::sumi
