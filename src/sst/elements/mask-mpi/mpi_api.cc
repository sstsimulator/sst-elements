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

#include <sstream>
#include <time.h>
#include <climits>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <ctime>

//#include <mercury/common/runtime.h>

#include <mpi_queue/mpi_queue.h>

#include <mpi_api.h>
#include <mpi_status.h>
#include <mpi_request.h>

#include <mercury/components/node.h>

//#include <sstmac/software/process/backtrace.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/components/operating_system.h>
//#include <sstmac/software/process/ftq_scope.h>
//#include <mercury/operating_system/launch/job_launcher.h>


#include <mpi_protocol/mpi_protocol.h>
#include <mpi_comm/mpi_comm_factory.h>
#include <mpi_types.h>

#include <unusedvariablemacro.h>

#ifdef SST_HG_OTF2_ENABLED 
#include <sumi-mpi/otf2_output_stat.h>
#endif

#include <mercury/common/errors.h>
#//include <sprockit/statics.h>
//#include <sprockit/output.h>
#include <mercury/common/util.h>
#include <sst/core/params.h>
//#include <sstmac/keyword_registration.h>

#ifdef SST_HG_OTF2_ENABLED
#include <sumi-mpi/otf2_output_stat.h>
#endif

//MakeDebugSlot(mpi_sync)
//MakeDebugSlot(mpi_finalize)
//RegisterKeywords(
//{ "iprobe_delay", "the delay incurred each time MPI_Iprobe is called" },
//{ "dump_comm_times", "dump communication time statistics" },
//{ "otf2_dir_basename", "Enables OTF2 and combines this parameter with a timestamp to name the archive"}
//);

//sprockit::StaticNamespaceRegister mpi_ns_reg("mpi");
//sprockit::StaticNamespaceRegister queue_ns_reg("queue");

namespace SST::MASKMPI {

//static sprockit::NeedDeletestatics<MpiApi> del_statics;
//sstmac::FTQTag MpiApi::mpi_tag("MPI", 1);

MpiApi* mask_mpi()
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  return t->getLibrary<MpiApi>("MpiApi");
}

//
// Build a new mpiapi.
//
MpiApi::MpiApi(SST::Params& params, SST::Hg::App* app) :
  SST::Iris::sumi::SimTransport(params, app),
  queue_(nullptr),
  next_type_id_(0),
  next_op_id_(first_custom_op_id),
  status_(is_fresh),
  crossed_comm_world_barrier_(false),
  worldcomm_(nullptr),
  selfcomm_(nullptr),
  group_counter_(MPI_GROUP_SELF+1),
#ifdef SST_HG_OTF2_ENABLED
  OTF2Writer_(nullptr),
#endif
  req_counter_(0),
  generate_ids_(true)
{
  verbose_ = params.find<unsigned int>("verbose", 0);
  out_ = std::unique_ptr<SST::Output>(
    new SST::Output("MpiApi:", verbose_, 0, Output::STDOUT));

  comm_factory_ = new MpiCommFactory(app->sid(), this, verbose_);

  if (!engine_) engine_ = new Iris::sumi::CollectiveEngine(params, this);

  queue_ = new MpiQueue(params, app->sid().task_, this, engine_);

  double probe_delay_s = params.find<SST::UnitAlgebra>("iprobe_delay", "0s").getValue().toDouble();
  iprobe_delay_us_ = probe_delay_s * 1e6;

  double test_delay_s = params.find<SST::UnitAlgebra>("test_delay", "1us").getValue().toDouble();
  test_delay_us_ = test_delay_s * 1e6;

#ifdef SST_HG_OTF2_ENABLED
#if !SST_HG_INTEGRATED_SST_CORE
  auto subname = sprockit::sprintf("App%d-Rank%d", app->sid().app_, app->sid().task_);
  auto* stat = comp->registerStatistic<void>(params, "otf2", subname);
  //this will either be a null stat or an otf2 stat
  //the rest of the code will do null checks on the variable before dumping traces
  OTF2Writer_ = dynamic_cast<OTF2Writer*>(stat);
#endif
#endif
}

uint64_t
MpiApi::traceClock() const
{
  return now().time.ticks();
}

void
MpiApi::deleteStatics()
{
}

MpiApi::~MpiApi()
{
  //MUST DELETE HERE
  //cannot delete in finalize
  //this is weird with context switching
  //an unblock finishes finalize... so finalize is called while the DES thread is still inside the queue
  //the queue outlives mpi_api::finalize!
  if (queue_) delete queue_;

  //these are often not cleaned up correctly by app
  for (auto& pair : grp_map_){
    MpiGroup* grp = pair.second;
    delete grp;
  }

  //these are often not cleaned up correctly by app
  //do not delete this one
  comm_map_.erase(MPI_COMM_NULL);
  for (auto& pair : comm_map_){
    MpiComm* comm = pair.second;
    delete comm;
  }

  // This causes a "pointer being freed was not allocated" error
  // //people can be sloppy cleaning up requests
  // //clean up for them
  // for (auto& pair : req_map_){
  //   MpiRequest* req = pair.second;
  //   delete req;
  // }

  if (qos_analysis_) delete qos_analysis_;
}

int
MpiApi::abort(MPI_Comm  /*comm*/, int errcode)
{

  sst_hg_throw_printf(SST::Hg::ValueError,
    "MPI rank %d exited with code %d", rank_, errcode);
  return MPI_SUCCESS;
}

int
MpiApi::commRank(MPI_Comm comm, int *rank)
{
  *rank = getComm(comm)->rank();
  return MPI_SUCCESS;
}

int
MpiApi::init(int*  /*argc*/, char***  /*argv*/)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

out_->debug(CALL_INFO, 1, 0,
  "rank %d init\n", rank_);

  if (status_ == is_initialized){
    SST::Hg::abort("MPI_Init cannot be called twice");
  }

  //StartMPICall(MPI_Init);

  Iris::sumi::SimTransport::init();

  comm_factory_->init(rank_, nproc_);

  worldcomm_ = comm_factory_->world();
  if (smp_optimize_){
    out_->debug(CALL_INFO, 1, 0,
      "rank %d creating smp communicator\n", rank_);
    worldcomm_->createSmpCommunicator(smp_neighbors_, engine(), Iris::sumi::Message::default_cq);
  }

  selfcomm_ = comm_factory_->self();
  comm_map_[MPI_COMM_WORLD] = worldcomm_;
  comm_map_[MPI_COMM_SELF] = selfcomm_;
  grp_map_[MPI_GROUP_WORLD] = worldcomm_->group();
  grp_map_[MPI_GROUP_SELF] = selfcomm_->group();

  //mpi_api_debug(sprockit::dbg::mpi, "MPI_Init()");

  /** Make sure all the default types are known */
  commitBuiltinTypes();

  status_ = is_initialized;

  queue_->init();

  crossed_comm_world_barrier_ = true;
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->init(start_clock, traceClock(), rank_, nproc_);
  }
#endif

  //FinishMPICall(MPI_Init);

  return MPI_SUCCESS;
}

void
MpiApi::checkInit()
{
  if (status_ != is_initialized){
    sst_hg_abort_printf("MPI Rank %d calling functions before calling MPI_Init", rank_);
  }
}

//
// Finalize MPI.
//
int
MpiApi::finalize()
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  //StartMPICall(MPI_Finalize);

  auto op = startBarrier("MPI_Finalize", MPI_COMM_WORLD);
  waitCollective(std::move(op));

  //mpi_api_debug(sprockit::dbg::mpi, "MPI_Finalize()");

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().generic_call(start_clock, traceClock(), "MPI_Finalize");
    OTF2Writer_->assignGlobalCommIds(this);
  }
#endif

  status_ = is_finalized;

  int rank = commWorld()->rank();
  if (rank == 0) {
//    debug_printf(sprockit::dbg::mpi_finalize,
//      "MPI_Finalize passed on app %d", sid().app_);
  }

  engine_->cleanUp();
  SimTransport::finish();

  //endAPICall();

  return MPI_SUCCESS;
}

//
// Get current time.
//
double
MpiApi::wtime()
{
  //StartMPICall(MPI_Wtime);
  return now().sec();
}

int
MpiApi::getCount(const MPI_Status *status, MPI_Datatype  /*datatype*/, int *count)
{
  *count = status->count;
  return MPI_SUCCESS;
}

const char*
MpiApi::opStr(MPI_Op op)
{
#define op_case(x) case x: return #x;
 switch(op)
 {
 op_case(MPI_MAX);
 op_case(MPI_MIN);
 op_case(MPI_SUM);
 op_case(MPI_PROD);
 op_case(MPI_LAND);
 op_case(MPI_BAND);
 op_case(MPI_LOR);
 op_case(MPI_BOR);
 op_case(MPI_LXOR);
 op_case(MPI_BXOR);
 op_case(MPI_MAXLOC);
 op_case(MPI_MINLOC);
 op_case(MPI_REPLACE);
 default:
  return "CUSTOM";
 }
}

std::string
MpiApi::typeStr(MPI_Datatype mid)
{
  MpiType* ty = typeFromId(mid);
  switch(ty->type())
  {
    case MpiType::PRIM:
      return SST::Hg::sprintf("%s=%d", ty->label.c_str(), mid);
    case MpiType::PAIR:
      return SST::Hg::sprintf("PAIR=%d", mid);
    case MpiType::VEC:
      return SST::Hg::sprintf("VEC=%d", mid);
    case MpiType::IND:
      return SST::Hg::sprintf("IND=%d", mid);
    case MpiType::NONE:
      return SST::Hg::sprintf("NONE=%d", mid);
    default:
      return "UNKNOWN TYPE";
  }
}

std::string
MpiApi::commStr(MPI_Comm comm)
{
  if (comm == commWorld()->id()){
    return "MPI_COMM_WORLD";
  } else if (comm == commSelf()->id()){
    return "MPI_COMM_SELF";
  } else if (comm == MpiComm::comm_null->id()){
    return "MPI_COMM_NULL";
  } else {
    return SST::Hg::sprintf("COMM=%d", int(comm));
  }
}

std::string
MpiApi::commStr(MpiComm* comm)
{
  if (comm == commWorld()){
    return "MPI_COMM_WORLD";
  } else if (comm == commSelf()){
    return "MPI_COMM_SELF";
  } else if (comm == MpiComm::comm_null){
    return "MPI_COMM_NULL";
  } else {
    return SST::Hg::sprintf("COMM=%d", int(comm->id()));
  }
}

std::string
MpiApi::tagStr(int tag)
{
  if (tag==MPI_ANY_TAG){
    return "int_ANY";
  } else {
    return SST::Hg::sprintf("%d", int(tag));
  }
}

std::string
MpiApi::srcStr(int id)
{
  if (id == MPI_ANY_SOURCE){
    return "MPI_SOURCE_ANY";
  } else {
    return SST::Hg::sprintf("%d", int(id));
  }
}

std::string
MpiApi::srcStr(MpiComm* comm, int id)
{
  if (id == MPI_ANY_SOURCE){
    return "MPI_SOURCE_ANY";
  } else {
    return SST::Hg::sprintf("%d:%d", int(id), int(comm->peerTask(id)));
  }
}

MpiComm*
MpiApi::getComm(MPI_Comm comm)
{
  auto it = comm_map_.find(comm);
  if (it == comm_map_.end()) {
    if (comm == MPI_COMM_WORLD){
      std::cerr << "Could not find MPI_COMM_WORLD! "
            << "Are you sure you called MPI_Init" << std::endl;
    }
    sst_hg_throw_printf(SST::Hg::HgError,
        "could not find mpi communicator %d for rank %d",
        comm, int(rank_));
  }
  return it->second;
}

MpiGroup*
MpiApi::getGroup(MPI_Group grp)
{
  auto it = grp_map_.find(grp);
  if (it == grp_map_.end()) {
    sst_hg_abort_printf("could not find mpi group %d for rank %d",
        grp, int(rank_));
  }
  return it->second;
}

void
MpiApi::addKeyval(int key, keyval* keyval)
{
  keyvals_[key] = keyval;
}

keyval*
MpiApi::getKeyval(int key)
{
  checkKey(key);
  return keyvals_[key];
}

MpiRequest*
MpiApi::getRequest(MPI_Request req)
{
  if (req == MPI_REQUEST_NULL){
    return nullptr;
  }

  auto it = req_map_.find(req);
  if (it == req_map_.end()) {
    sst_hg_throw_printf(SST::Hg::HgError,
        "could not find mpi request %d for rank %d",
        req, int(rank_));
  }
  return it->second;
}

void
MpiApi::addCommPtr(MpiComm* ptr, MPI_Comm* comm)
{
  if (generate_ids_){
    *comm = ptr->id();
  } else {
    ptr->setId(*comm);
  }
  comm_map_[*comm] = ptr;
}

void
MpiApi::eraseCommPtr(MPI_Comm comm)
{
  if (comm != MPI_COMM_WORLD && comm != MPI_COMM_SELF && comm != MPI_COMM_NULL) {
    comm_ptr_map::iterator it = comm_map_.find(comm);
    if (it == comm_map_.end()) {
      sst_hg_throw_printf(SST::Hg::HgError,
        "could not find mpi communicator %d for rank %d",
        comm, int(rank_));
    }
    comm_map_.erase(it);
  }
}

void
MpiApi::addGroupPtr(MPI_Group grp, MpiGroup* ptr)
{
  grp_map_[grp] = ptr;
}

void
MpiApi::addGroupPtr(MpiGroup* ptr, MPI_Group* grp)
{
  if (generate_ids_) *grp = group_counter_++;
  grp_map_[*grp] = ptr;
  ptr->setId(*grp);
}

void
MpiApi::eraseGroupPtr(MPI_Group grp)
{
  if (grp != MPI_GROUP_EMPTY
      && grp != MPI_GROUP_WORLD
      && grp != MPI_GROUP_SELF) {
    group_ptr_map::iterator it = grp_map_.find(grp);
    if (it == grp_map_.end()) {
      sst_hg_throw_printf(SST::Hg::HgError,
        "could not find mpi group %d for rank %d",
        grp, int(rank_));
    }
    grp_map_.erase(it);
  }
}

void
MpiApi::addRequestPtr(MpiRequest* ptr, MPI_Request* req)
{
  if (generate_ids_) *req = req_counter_++;
  req_map_[*req] = ptr;
}

void
MpiApi::eraseRequestPtr(MPI_Request req)
{
  req_ptr_map::iterator it = req_map_.find(req);
  if (it == req_map_.end()) {
    sst_hg_throw_printf(SST::Hg::HgError,
        "could not find mpi request %d for rank %d",
        req, int(rank_));
  }
  delete it->second;
  req_map_.erase(it);
}

void
MpiApi::checkKey(int key)
{
  if (keyvals_.find(key) == keyvals_.end()) {
    sst_hg_abort_printf("mpi_api::check_key: could not find keyval %d in key_map", key);
  }
}


int
MpiApi::errorString(int  /*errorcode*/, char *str, int *resultlen)
{
  static const char* errorstr = "mpi error";
  *resultlen = ::strlen(errorstr);
  ::strcpy(str, errorstr);
  return MPI_SUCCESS;
}

//CallGraphCreateTag(idle);
//CallGraphCreateTag(active);

void
MpiApi::logMessageDelay(  Iris::sumi::Message *msg,
    uint64_t bytes,  int stage,
    SST::Hg::TimeDelta sync_delay,  SST::Hg::TimeDelta
    active_delay,  SST::Hg::TimeDelta time_since_quiesce)
{
}

void
MpiApi::finishCurrentMpiCall()
{
}

#define enumcase(x) case x: return #x

const char*
MPI_Call::ID_str(MPI_function func)
{
  switch(func){
  case Call_ID_MPI_Send: return "MPI_Send";
  case Call_ID_MPI_Recv: return "MPI_Recv";
  case Call_ID_MPI_Get_count: return "MPI_Get_count";
  case Call_ID_MPI_Bsend: return "MPI_Bsend";
  case Call_ID_MPI_Ssend: return "MPI_Ssend";
  case Call_ID_MPI_Rsend: return "MPI_Rsend";
  case Call_ID_MPI_Buffer_attach: return "MPI_Buffer_attach";
  case Call_ID_MPI_Buffer_detach: return "MPI_Buffer_detach";
  case Call_ID_MPI_Isend: return "MPI_Isend";
  case Call_ID_MPI_Ibsend: return "MPI_Ibsend";
  case Call_ID_MPI_Issend: return "MPI_Issend";
  case Call_ID_MPI_Irsend: return "MPI_Irsend";
  case Call_ID_MPI_Irecv: return "MPI_Irecv";
  case Call_ID_MPI_Wait: return "MPI_Wait";
  case Call_ID_MPI_Test: return "MPI_Test";
  case Call_ID_MPI_Request_free: return "MPI_Request_free";
  case Call_ID_MPI_Waitany: return "MPI_Waitany";
  case Call_ID_MPI_Testany: return "MPI_Testany";
  case Call_ID_MPI_Waitall: return "MPI_Waitall";
  case Call_ID_MPI_Testall: return "MPI_Testall";
  case Call_ID_MPI_Waitsome: return "MPI_Waitsome";
  case Call_ID_MPI_Testsome: return "MPI_Testsome";
  case Call_ID_MPI_Iprobe: return "MPI_Iprobe";
  case Call_ID_MPI_Probe: return "MPI_Probe";
  case Call_ID_MPI_Cancel: return "MPI_Cancel";
  case Call_ID_MPI_Test_cancelled: return "MPI_Test_cancelled";
  case Call_ID_MPI_Send_init: return "MPI_Send_init";
  case Call_ID_MPI_Bsend_init: return "MPI_Bsend_init";
  case Call_ID_MPI_Ssend_init: return "MPI_Ssend_init";
  case Call_ID_MPI_Rsend_init: return "MPI_Rsend_init";
  case Call_ID_MPI_Recv_init: return "MPI_Recv_init";
  case Call_ID_MPI_Start: return "MPI_Start";
  case Call_ID_MPI_Startall: return "MPI_Startall";
  case Call_ID_MPI_Sendrecv: return "MPI_Sendrecv";
  case Call_ID_MPI_Sendrecv_replace: return "MPI_Sendrecv_replace";
  case Call_ID_MPI_Type_contiguous: return "MPI_Type_contiguous";
  case Call_ID_MPI_Type_vector: return "MPI_Type_vector";
  case Call_ID_MPI_Type_hvector: return "MPI_Type_hvector";
  case Call_ID_MPI_Type_indexed: return "MPI_Type_indexed";
  case Call_ID_MPI_Type_hindexed: return "MPI_Type_hindexed";
  case Call_ID_MPI_Type_struct: return "MPI_Type_struct";
  case Call_ID_MPI_Address: return "MPI_Address";
  case Call_ID_MPI_Type_extent: return "MPI_Type_extent";
  case Call_ID_MPI_Type_size: return "MPI_Type_size";
  case Call_ID_MPI_Type_lb: return "MPI_Type_lb";
  case Call_ID_MPI_Type_ub: return "MPI_Type_ub";
  case Call_ID_MPI_Type_commit: return "MPI_Type_commit";
  case Call_ID_MPI_Type_free: return "MPI_Type_free";
  case Call_ID_MPI_Get_elements: return "MPI_Get_elements";
  case Call_ID_MPI_Pack: return "MPI_Pack";
  case Call_ID_MPI_Unpack: return "MPI_Unpack";
  case Call_ID_MPI_Pack_size: return "MPI_Pack_size";
  case Call_ID_MPI_Barrier: return "MPI_Barrier";
  case Call_ID_MPI_Bcast: return "MPI_Bcast";
  case Call_ID_MPI_Gather: return "MPI_Gather";
  case Call_ID_MPI_Gatherv: return "MPI_Gatherv";
  case Call_ID_MPI_Scatter: return "MPI_Scatter";
  case Call_ID_MPI_Scatterv: return "MPI_Scatterv";
  case Call_ID_MPI_Allgather: return "MPI_Allgather";
  case Call_ID_MPI_Allgatherv: return "MPI_Allgatherv";
  case Call_ID_MPI_Alltoall: return "MPI_Alltoall";
  case Call_ID_MPI_Alltoallv: return "MPI_Alltoallv";
  case Call_ID_MPI_Reduce: return "MPI_Reduce";
  case Call_ID_MPI_Op_create: return "MPI_Op_create";
  case Call_ID_MPI_Op_free: return "MPI_Op_free";
  case Call_ID_MPI_Allreduce: return "MPI_Allreduce";
  case Call_ID_MPI_Reduce_scatter: return "MPI_Reduce_scatter";
  case Call_ID_MPI_Scan: return "MPI_Scan";
  case Call_ID_MPI_Ibarrier: return "MPI_Ibarrier";
  case Call_ID_MPI_Ibcast: return "MPI_Ibcast";
  case Call_ID_MPI_Igather: return "MPI_Igather";
  case Call_ID_MPI_Igatherv: return "MPI_Igatherv";
  case Call_ID_MPI_Iscatter: return "MPI_Iscatter";
  case Call_ID_MPI_Iscatterv: return "MPI_Iscatterv";
  case Call_ID_MPI_Iallgather: return "MPI_Iallgather";
  case Call_ID_MPI_Iallgatherv: return "MPI_Iallgatherv";
  case Call_ID_MPI_Ialltoall: return "MPI_Ialltoall";
  case Call_ID_MPI_Ialltoallv: return "MPI_Ialltoallv";
  case Call_ID_MPI_Ireduce: return "MPI_Ireduce";
  case Call_ID_MPI_Iallreduce: return "MPI_Iallreduce";
  case Call_ID_MPI_Ireduce_scatter: return "MPI_Ireduce_scatter";
  case Call_ID_MPI_Iscan: return "MPI_Iscan";
  case Call_ID_MPI_Reduce_scatter_block: return "MPI_Reduce_scatter_block";
  case Call_ID_MPI_Ireduce_scatter_block: return "MPI_Ireduce_scatter_block";
  case Call_ID_MPI_Group_size: return "MPI_Group_size";
  case Call_ID_MPI_Group_rank: return "MPI_Group_rank";
  case Call_ID_MPI_Group_translate_ranks: return "MPI_Group_translate_ranks";
  case Call_ID_MPI_Group_compare: return "MPI_Group_compare";
  case Call_ID_MPI_Comm_group: return "MPI_Comm_group";
  case Call_ID_MPI_Group_union: return "MPI_Group_union";
  case Call_ID_MPI_Group_intersection: return "MPI_Group_intersection";
  case Call_ID_MPI_Group_difference: return "MPI_Group_difference";
  case Call_ID_MPI_Group_incl: return "MPI_Group_incl";
  case Call_ID_MPI_Group_excl: return "MPI_Group_excl";
  case Call_ID_MPI_Group_range_incl: return "MPI_Group_range_incl";
  case Call_ID_MPI_Group_range_excl: return "MPI_Group_range_excl";
  case Call_ID_MPI_Group_free: return "MPI_Group_free";
  case Call_ID_MPI_Comm_size: return "MPI_Comm_size";
  case Call_ID_MPI_Comm_rank: return "MPI_Comm_rank";
  case Call_ID_MPI_Comm_compare: return "MPI_Comm_compare";
  case Call_ID_MPI_Comm_dup: return "MPI_Comm_dup";
  case Call_ID_MPI_Comm_create: return "MPI_Comm_create";
  case Call_ID_MPI_Comm_create_group: return "MPI_Comm_create_group";
  case Call_ID_MPI_Comm_split: return "MPI_Comm_split";
  case Call_ID_MPI_Comm_free: return "MPI_Comm_free";
  case Call_ID_MPI_Comm_test_inter: return "MPI_Comm_test_inter";
  case Call_ID_MPI_Comm_remote_size: return "MPI_Comm_remote_size";
  case Call_ID_MPI_Comm_remote_group: return "MPI_Comm_remote_group";
  case Call_ID_MPI_Intercomm_create: return "MPI_Intercomm_create";
  case Call_ID_MPI_Intercomm_merge: return "MPI_Intercomm_merge";
  case Call_ID_MPI_Keyval_create: return "MPI_Keyval_create";
  case Call_ID_MPI_Keyval_free: return "MPI_Keyval_free";
  case Call_ID_MPI_Attr_put: return "MPI_Attr_put";
  case Call_ID_MPI_Attr_get: return "MPI_Attr_get";
  case Call_ID_MPI_Attr_delete: return "MPI_Attr_delete";
  case Call_ID_MPI_Topo_test: return "MPI_Topo_test";
  case Call_ID_MPI_Cart_create: return "MPI_Cart_create";
  case Call_ID_MPI_Dims_create: return "MPI_Dims_create";
  case Call_ID_MPI_Graph_create: return "MPI_Graph_create";
  case Call_ID_MPI_Graphdims_get: return "MPI_Graphdims_get";
  case Call_ID_MPI_Graph_get: return "MPI_Graph_get";
  case Call_ID_MPI_Cartdim_get: return "MPI_Cartdim_get";
  case Call_ID_MPI_Cart_get: return "MPI_Cart_get";
  case Call_ID_MPI_Cart_rank: return "MPI_Cart_rank";
  case Call_ID_MPI_Cart_coords: return "MPI_Cart_coords";
  case Call_ID_MPI_Graph_neighbors_count: return "MPI_Graph_neighbors_count";
  case Call_ID_MPI_Graph_neighbors: return "MPI_Graph_neighbors";
  case Call_ID_MPI_Cart_shift: return "MPI_Cart_shift";
  case Call_ID_MPI_Cart_sub: return "MPI_Cart_sub";
  case Call_ID_MPI_Cart_map: return "MPI_Cart_map";
  case Call_ID_MPI_Graph_map: return "MPI_Graph_map";
  case Call_ID_MPI_Get_processor_name: return "MPI_Get_processor_name";
  case Call_ID_MPI_Get_version: return "MPI_Get_version";
  case Call_ID_MPI_Errhandler_create: return "MPI_Errhandler_create";
  case Call_ID_MPI_Errhandler_set: return "MPI_Errhandler_set";
  case Call_ID_MPI_Errhandler_get: return "MPI_Errhandler_get";
  case Call_ID_MPI_Errhandler_free: return "MPI_Errhandler_free";
  case Call_ID_MPI_Error_string: return "MPI_Error_string";
  case Call_ID_MPI_Error_class: return "MPI_Error_class";
  case Call_ID_MPI_Wtime: return "MPI_Wtime";
  case Call_ID_MPI_Wtick: return "MPI_Wtick";
  case Call_ID_MPI_Init: return "MPI_Init";
  case Call_ID_MPI_Finalize: return "MPI_Finalize";
  case Call_ID_MPI_Initialized: return "MPI_Initialized";
  case Call_ID_MPI_Abort: return "MPI_Abort";
  case Call_ID_MPI_Pcontrol: return "MPI_Pcontrol";
  case Call_ID_MPI_Close_port: return "MPI_Close_port";
  case Call_ID_MPI_Comm_accept: return "MPI_Comm_accept";
  case Call_ID_MPI_Comm_connect: return "MPI_Comm_connect";
  case Call_ID_MPI_Comm_disconnect: return "MPI_Comm_disconnect";
  case Call_ID_MPI_Comm_get_parent: return "MPI_Comm_get_parent";
  case Call_ID_MPI_Comm_join: return "MPI_Comm_join";
  case Call_ID_MPI_Comm_spawn: return "MPI_Comm_spawn";
  case Call_ID_MPI_Comm_spawn_multiple: return "MPI_Comm_spawn_multiple";
  case Call_ID_MPI_Lookup_name: return "MPI_Lookup_name";
  case Call_ID_MPI_Open_port: return "MPI_Open_port";
  case Call_ID_MPI_Publish_name: return "MPI_Publish_name";
  case Call_ID_MPI_Unpublish_name: return "MPI_Unpublish_name";
  case Call_ID_MPI_Accumulate: return "MPI_Accumulate";
  case Call_ID_MPI_Get: return "MPI_Get";
  case Call_ID_MPI_Put: return "MPI_Put";
  case Call_ID_MPI_Win_complete: return "MPI_Win_complete";
  case Call_ID_MPI_Win_create: return "MPI_Win_create";
  case Call_ID_MPI_Win_fence: return "MPI_Win_fence";
  case Call_ID_MPI_Win_free: return "MPI_Win_free";
  case Call_ID_MPI_Win_get_group: return "MPI_Win_get_group";
  case Call_ID_MPI_Win_lock: return "MPI_Win_lock";
  case Call_ID_MPI_Win_post: return "MPI_Win_post";
  case Call_ID_MPI_Win_start: return "MPI_Win_start";
  case Call_ID_MPI_Win_test: return "MPI_Win_test";
  case Call_ID_MPI_Win_unlock: return "MPI_Win_unlock";
  case Call_ID_MPI_Win_wait: return "MPI_Win_wait";
  case Call_ID_MPI_Alltoallw: return "MPI_Alltoallw";
  case Call_ID_MPI_Exscan: return "MPI_Exscan";
  case Call_ID_MPI_Add_error_class: return "MPI_Add_error_class";
  case Call_ID_MPI_Add_error_code: return "MPI_Add_error_code";
  case Call_ID_MPI_Add_error_string: return "MPI_Add_error_string";
  case Call_ID_MPI_Comm_call_errhandler: return "MPI_Comm_call_errhandler";
  case Call_ID_MPI_Comm_create_keyval: return "MPI_Comm_create_keyval";
  case Call_ID_MPI_Comm_delete_attr: return "MPI_Comm_delete_attr";
  case Call_ID_MPI_Comm_free_keyval: return "MPI_Comm_free_keyval";
  case Call_ID_MPI_Comm_get_attr: return "MPI_Comm_get_attr";
  case Call_ID_MPI_Comm_get_name: return "MPI_Comm_get_name";
  case Call_ID_MPI_Comm_set_attr: return "MPI_Comm_set_attr";
  case Call_ID_MPI_Comm_set_name: return "MPI_Comm_set_name";
  case Call_ID_MPI_File_call_errhandler: return "MPI_File_call_errhandler";
  case Call_ID_MPI_Grequest_complete: return "MPI_Grequest_complete";
  case Call_ID_MPI_Grequest_start: return "MPI_Grequest_start";
  case Call_ID_MPI_Init_thread: return "MPI_Init_thread";
  case Call_ID_MPI_Is_thread_main: return "MPI_Is_thread_main";
  case Call_ID_MPI_Query_thread: return "MPI_Query_thread";
  case Call_ID_MPI_Status_set_cancelled: return "MPI_Status_set_cancelled";
  case Call_ID_MPI_Status_set_elements: return "MPI_Status_set_elements";
  case Call_ID_MPI_Type_create_keyval: return "MPI_Type_create_keyval";
  case Call_ID_MPI_Type_delete_attr: return "MPI_Type_delete_attr";
  case Call_ID_MPI_Type_dup: return "MPI_Type_dup";
  case Call_ID_MPI_Type_free_keyval: return "MPI_Type_free_keyval";
  case Call_ID_MPI_Type_get_attr: return "MPI_Type_get_attr";
  case Call_ID_MPI_Type_get_contents: return "MPI_Type_get_contents";
  case Call_ID_MPI_Type_get_envelope: return "MPI_Type_get_envelope";
  case Call_ID_MPI_Type_get_name: return "MPI_Type_get_name";
  case Call_ID_MPI_Type_set_attr: return "MPI_Type_set_attr";
  case Call_ID_MPI_Type_set_name: return "MPI_Type_set_name";
  case Call_ID_MPI_Type_match_size: return "MPI_Type_match_size";
  case Call_ID_MPI_Win_call_errhandler: return "MPI_Win_call_errhandler";
  case Call_ID_MPI_Win_create_keyval: return "MPI_Win_create_keyval";
  case Call_ID_MPI_Win_delete_attr: return "MPI_Win_delete_attr";
  case Call_ID_MPI_Win_free_keyval: return "MPI_Win_free_keyval";
  case Call_ID_MPI_Win_get_attr: return "MPI_Win_get_attr";
  case Call_ID_MPI_Win_get_name: return "MPI_Win_get_name";
  case Call_ID_MPI_Win_set_attr: return "MPI_Win_set_attr";
  case Call_ID_MPI_Win_set_name: return "MPI_Win_set_name";
  case Call_ID_MPI_Alloc_mem: return "MPI_Alloc_mem";
  case Call_ID_MPI_Comm_create_errhandler: return "MPI_Comm_create_errhandler";
  case Call_ID_MPI_Comm_get_errhandler: return "MPI_Comm_get_errhandler";
  case Call_ID_MPI_Comm_set_errhandler: return "MPI_Comm_set_errhandler";
  case Call_ID_MPI_File_create_errhandler: return "MPI_File_create_errhandler";
  case Call_ID_MPI_File_get_errhandler: return "MPI_File_get_errhandler";
  case Call_ID_MPI_File_set_errhandler: return "MPI_File_set_errhandler";
  case Call_ID_MPI_Finalized: return "MPI_Finalized";
  case Call_ID_MPI_Free_mem: return "MPI_Free_mem";
  case Call_ID_MPI_Get_address: return "MPI_Get_address";
  case Call_ID_MPI_Info_create: return "MPI_Info_create";
  case Call_ID_MPI_Info_delete: return "MPI_Info_delete";
  case Call_ID_MPI_Info_dup: return "MPI_Info_dup";
  case Call_ID_MPI_Info_free: return "MPI_Info_free";
  case Call_ID_MPI_Info_get: return "MPI_Info_get";
  case Call_ID_MPI_Info_get_nkeys: return "MPI_Info_get_nkeys";
  case Call_ID_MPI_Info_get_nthkey: return "MPI_Info_get_nthkey";
  case Call_ID_MPI_Info_get_valuelen: return "MPI_Info_get_valuelen";
  case Call_ID_MPI_Info_set: return "MPI_Info_set";
  case Call_ID_MPI_Pack_external: return "MPI_Pack_external";
  case Call_ID_MPI_Pack_external_size: return "MPI_Pack_external_size";
  case Call_ID_MPI_Request_get_status: return "MPI_Request_get_status";
  case Call_ID_MPI_Type_create_darray: return "MPI_Type_create_darray";
  case Call_ID_MPI_Type_create_hindexed: return "MPI_Type_create_hindexed";
  case Call_ID_MPI_Type_create_hvector: return "MPI_Type_create_hvector";
  case Call_ID_MPI_Type_create_indexed_block: return "MPI_Type_create_indexed_block";
  case Call_ID_MPI_Type_create_resized: return "MPI_Type_create_resized";
  case Call_ID_MPI_Type_create_struct: return "MPI_Type_create_struct";
  case Call_ID_MPI_Type_create_subarray: return "MPI_Type_create_subarray";
  case Call_ID_MPI_Type_get_extent: return "MPI_Type_get_extent";
  case Call_ID_MPI_Type_get_true_extent: return "MPI_Type_get_true_extent";
  case Call_ID_MPI_Unpack_external: return "MPI_Unpack_external";
  case Call_ID_MPI_Win_create_errhandler: return "MPI_Win_create_errhandler";
  case Call_ID_MPI_Win_get_errhandler: return "MPI_Win_get_errhandler";
  case Call_ID_MPI_Win_set_errhandler: return "MPI_Win_set_errhandler";
  default: 
   sst_hg_abort_printf("Bad MPI Call ID %d\n", func);
   return "Unknown or missing MPI function";
  }
}

}

extern "C" void sst_gdb_print_rank(){
  auto api = SST::MASKMPI::mask_mpi();
}
