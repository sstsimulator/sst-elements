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

#include <sst/core/params.h>
#include <mercury/common/factory.h>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/libraries/library.h>
#include <mercury/operating_system/libraries/api.h>
#include <mercury/operating_system/process/software_id.h>
#include <iris/sumi/message_fwd.h>
#include <iris/sumi/sim_transport.h>
#include <iris/sumi/message.h>
#include <iris/sumi/collective_message.h>
#include <mask_mpi_macro.h>

#include <mpi_types.h>
#include <mpi_integers.h>
#include <mpi_comm/mpi_comm.h>
#include <mpi_types/mpi_type_fwd.h>
#include <mpi_request.h>
#include <mpi_status.h>
#include <mpi_call.h>
#include <mpi_comm/mpi_comm_factory.h>
#include <mpi_debug.h>
#include <mpi_queue/mpi_queue_fwd.h>
#include <mpi_delay_stats.h>

#include <unordered_map>

//#include <sstmac/common/stats/ftq_tag.h>
//#include <sstmac/common/stats/stat_spyplot_fwd.h>
//#include <sstmac/software/process/backtrace.h>
//#include <sumi-mpi/otf2_output_stat_fwd.h>

#pragma once

namespace SST::MASKMPI {

class MpiApi : public SST::Iris::sumi::SimTransport
{
//  friend class OTF2Writer;
 public:
  SST_ELI_REGISTER_DERIVED(
    SST::Hg::API,
    MpiApi,
    "hg",
    "MpiApi",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the MPI transport API")

  MpiApi(SST::Params& params, SST::Hg::App* app, SST::Component* comp);

  static void deleteStatics();

 public:
  ~MpiApi() override;

  MpiQueue* queue() {
    return queue_;
  }

  /**
   * @brief crossed_comm_world_barrier
   *        Useful for statistics based on globally synchronous timings.
   * @return Whether a global collective has been crossed
   *
   */
  bool crossedCommWorldBarrier() const {
    return crossed_comm_world_barrier_;
  }

  MpiComm* commWorld() const {
    return worldcomm_;
  }

  MpiComm* commSelf() const {
    return selfcomm_;
  }

  int commGetAttr(MPI_Comm, int comm_keyval, void* attribute_val, int* flag);

  int commRank(MPI_Comm comm, int* rank);

  int commSize(MPI_Comm comm, int* size);

  int typeSize(MPI_Datatype type, int* size);

  int initialized(int* flag){
    *flag = (status_ == is_initialized);
    return MPI_SUCCESS;
  }

  int finalized(int* flag){
    *flag = (status_ == is_finalized);
    return MPI_SUCCESS;
  }

  int bufferAttach(void*  /*buffer*/, int  /*size*/){
    return MPI_SUCCESS;
  }

  int bufferDetach(void*  /*buffer*/, int*  /*size*/){
    return MPI_SUCCESS;
  }

  int initThread(int* argc, char*** argv, int required, int* provided){
    init(argc, argv);
    *provided = required;
    return MPI_SUCCESS;
  }

#pragma GCC diagnostic ignored "-Woverloaded-virtual"
  int init(int* argc, char*** argv);

  int finalize();

  double wtime();

  void setGenerateIds(bool flag){
    generate_ids_ = flag;
  }

  bool generateIds() const {
    return generate_ids_;
  }

  int abort(MPI_Comm comm, int errcode);

  int errhandlerSet(MPI_Comm  /*comm*/, MPI_Errhandler  /*handler*/){
    return MPI_SUCCESS;
  }

  int errhandlerCreate(MPI_Handler_function * /*function*/, MPI_Errhandler * /*errhandler*/){
    return MPI_SUCCESS;
  }

  int errorClass(int  /*errorcode*/, int* errorclass){
    *errorclass = 0;
    return MPI_SUCCESS;
  }

  int errorString(int errorcode, char* str, int* resultlen);

  int commSplit(MPI_Comm incomm, int color, int key, MPI_Comm* outcomm);

  int commDup(MPI_Comm input, MPI_Comm* output);

  int commCreate(MPI_Comm input, MPI_Group group, MPI_Comm* output);

  int commGroup(MPI_Comm comm, MPI_Group* grp);

  int commCreateGroup(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm);

  void commCreateWithId(MPI_Comm input, MPI_Group group, MPI_Comm new_comm);

  /**
   * @param group
   * @param num_members
   * @param members
   * @return Whether the current rank is in the group
   */
  bool groupCreateWithId(MPI_Group group, int num_members, const int* members);

  int cartCreate(MPI_Comm comm_old, int ndims, const int dims[],
              const int periods[], int reorder, MPI_Comm *comm_cart);

  int cartGet(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);

  int cartdimGet(MPI_Comm comm, int *ndims);

  int cartRank(MPI_Comm comm, const int coords[], int *rank);

  int cartShift(MPI_Comm comm, int direction, int disp,
                 int *rank_source, int *rank_dest);

  int cartCoords(MPI_Comm comm, int rank, int maxdims, int coords[]);

  int commFree(MPI_Comm* input);

  int commSetErrhandler(MPI_Comm  /*comm*/, MPI_Errhandler  /*errhandler*/){
    return MPI_SUCCESS;
  }

  int groupIncl(MPI_Group oldgrp,
             int num_ranks,
             const int* ranks,
             MPI_Group* newgrp);

  int groupRangeIncl(MPI_Group oldgrp, int n, int ranges[][3], MPI_Group* newgrp);

  int groupFree(MPI_Group* grp);

  int groupTranslateRanks(MPI_Group grp1, int n, const int* ranks1, MPI_Group grp2, int* ranks2);

  int sendrecv(const void* sendbuf, int sendcount,
        MPI_Datatype sendtype, int dest, int sendtag,
        void* recvbuf, int recvcount,
        MPI_Datatype recvtype, int source, int recvtag,
        MPI_Comm comm, MPI_Status* status);

  int send(const void *buf, int count,
           MPI_Datatype datatype, int dest, int tag,
           MPI_Comm comm);

  int sendInit(const void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm, MPI_Request *request);

  int isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
            MPI_Comm comm, MPI_Request *request);

  int recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
           MPI_Comm comm, MPI_Status *status);

  int irecv(void *buf, int count, MPI_Datatype datatype, int source,
            int tag, MPI_Comm comm, MPI_Request *request);

  int recvInit(void *buf, int count, MPI_Datatype datatype,
      int source, int tag, MPI_Comm comm, MPI_Request *request);

  int request_free(MPI_Request* req);

  int start(MPI_Request* req);

  int startall(int count, MPI_Request* req);

  int wait(MPI_Request *request, MPI_Status *status);

  int waitall(int count, MPI_Request requests[], MPI_Status statuses[]);

  int waitany(int count, MPI_Request requests[], int *indx, MPI_Status *status);

  int waitsome(int incount, MPI_Request requests[],
           int *outcount, int indices[], MPI_Status statuses[]);

  int test(MPI_Request *request, int *flag, MPI_Status *status);

  int testall(int count, MPI_Request requests[], int *flag, MPI_Status statuses[]);

  int testany(int count, MPI_Request requests[], int *indx, int *flag, MPI_Status *status);

  int testsome(int incount, MPI_Request requests[], int *outcount,
               int indices[], MPI_Status statuses[]);

  int probe(int source, int tag, MPI_Comm comm, MPI_Status *status);

  int iprobe(int source, int tag, MPI_Comm comm, int* flag, MPI_Status *status);

 int barrier(MPI_Comm comm);

  int bcast(int count, MPI_Datatype datatype, int root, MPI_Comm comm);

  int bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

  int scatter(int sendcount, MPI_Datatype sendtype,
          int recvcount, MPI_Datatype recvtype, int root,
          MPI_Comm comm);

  int scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
           void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
           MPI_Comm comm);

  int scatterv(const int *sendcounts,
           MPI_Datatype sendtype, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm);

  int scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
           MPI_Datatype sendtype, void *recvbuf, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm);

  int gather(int sendcount, MPI_Datatype sendtype,
         int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm);

  int gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
         void *recvbuf, int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm);

  int gatherv(int sendcount, MPI_Datatype sendtype,
         const int *recvcounts,
         MPI_Datatype recvtype, int root, MPI_Comm comm);

  int gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
          void *recvbuf, const int *recvcounts, const int *displs,
          MPI_Datatype recvtype, int root, MPI_Comm comm);

  int allgather(int count, MPI_Datatype type, MPI_Comm comm);

  int allgather(int sendcount, MPI_Datatype sendtype,
            int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm);

  int allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm);

  int allgatherv(int sendcount, MPI_Datatype sendtype,
             const int *recvcounts,
             MPI_Datatype recvtype, MPI_Comm comm);

  int allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
             void *recvbuf, const int *recvcounts, const int *displs,
             MPI_Datatype recvtype, MPI_Comm comm);

  int alltoall(int sendcount, MPI_Datatype sendtype,
           int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm);

  int alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm);

  int alltoallv(const int *sendcounts,
            MPI_Datatype sendtype,
            const int *recvcounts,
            MPI_Datatype recvtype,
            MPI_Comm comm);

  int alltoallv(const void *sendbuf, const int *sendcounts,
            const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
            const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
            MPI_Comm comm);

  int reduce(int count, MPI_Datatype type, MPI_Op op, int root, MPI_Comm comm);

  int reduce(const void* src, void* dst,
         int count, MPI_Datatype type, MPI_Op op, int root,
         MPI_Comm comm);

  int allreduce(int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm);

  int allreduce(const void* src, void* dst,
            int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm);

  int scan(int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm);

  int scan(const void* src, void* dst,
            int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm);

  int reduceScatter(int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm);

  int reduceScatter(const void* src, void* dst,
                 const int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm);

  int reduceScatterBlock(int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm);

  int reduceScatterBlock(const void* src, void* dst,
                 int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm);

  int ibarrier(MPI_Comm comm, MPI_Request* req);

  int ibcast(int count, MPI_Datatype datatype, int root,
        MPI_Comm comm, MPI_Request* req);

  int ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm, MPI_Request* req);

  int iscatter(int sendcount, MPI_Datatype sendtype,
          int recvcount, MPI_Datatype recvtype, int root,
          MPI_Comm comm, MPI_Request* req);

  int iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
           void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
           MPI_Comm comm, MPI_Request* req);

  int iscatterv(const int *sendcounts,
           MPI_Datatype sendtype, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm, MPI_Request* req);

  int iscatterv(const void *sendbuf, const int *sendcounts, const int *displs,
           MPI_Datatype sendtype, void *recvbuf, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm, MPI_Request* req);

  int igather(int sendcount, MPI_Datatype sendtype,
         int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm, MPI_Request* req);

  int igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
         void *recvbuf, int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm, MPI_Request* req);

  int igatherv(int sendcount, MPI_Datatype sendtype,
         const int *recvcounts,
         MPI_Datatype recvtype, int root,
         MPI_Comm comm, MPI_Request* req);

  int igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
          void *recvbuf, const int *recvcounts, const int *displs,
          MPI_Datatype recvtype, int root,
          MPI_Comm comm, MPI_Request* req);

  int iallgather(int sendcount, MPI_Datatype sendtype,
            int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req);

  int iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req);

  int iallgatherv(int sendcount, MPI_Datatype sendtype,
             const int *recvcounts,
             MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* req);

  int iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
             void *recvbuf, const int *recvcounts, const int *displs,
             MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* req);

  int ialltoall(int sendcount, MPI_Datatype sendtype,
            int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req);

  int ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req);

  int ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                 const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                 const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                 MPI_Request *request);

  int ialltoallv(const int *sendcounts, MPI_Datatype sendtype,
                const int *recvcounts, MPI_Datatype recvtype,
                MPI_Comm comm, MPI_Request* req);

  int ialltoallv(const void *sendbuf, const int *sendcounts,
            const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
            const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req);

  int ireduce(int count, MPI_Datatype type, MPI_Op op, int root,
            MPI_Comm comm, MPI_Request* req);

  int ireduce(const void* src, void* dst,
         int count, MPI_Datatype type, MPI_Op op, int root,
         MPI_Comm comm, MPI_Request* req);

  int iallreduce(int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm, MPI_Request* req);

  int iallreduce(const void* src, void* dst,
            int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm, MPI_Request* req);

  int iscan(int count, MPI_Datatype type, MPI_Op op,
        MPI_Comm comm, MPI_Request* req);

  int iscan(const void* src, void* dst,
        int count, MPI_Datatype type, MPI_Op op,
        MPI_Comm comm, MPI_Request* req);

  int ireduceScatter(int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req);

  int ireduceScatter(const void* src, void* dst,
                 const int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req);

  int ireduceScatterBlock(int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req);

  int ireduceScatterBlock(const void* src, void* dst,
                 int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req);


  int typeGetName(MPI_Datatype type, char* type_name, int* resultlen);

  int typeSetName(MPI_Datatype type, const char* type_name);

  int typeExtent(MPI_Datatype type, MPI_Aint* extent);

  int packSize(int incount, MPI_Datatype datatype,
         MPI_Comm comm, int *size);

  int winFlush(int rank, MPI_Win win);

  int winFlushLocal(int rank, MPI_Win win);

  int winCreate(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                 MPI_Comm comm, MPI_Win *win);

  int winFree(MPI_Win *win);

  int winLock(int lock_type, int rank, int assert, MPI_Win win);

  int winUnlock(int rank, MPI_Win win);

  int get(void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win);

  int put(const void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win);

 public:
  int opCreate(MPI_User_function* user_fn, int commute, MPI_Op* op);

  int opFree(MPI_Op* op);

  int getCount(const MPI_Status* status, MPI_Datatype datatype, int* count);

  int typeDup(MPI_Datatype intype, MPI_Datatype* outtype);

  int typeSetName(MPI_Datatype id, const std::string &name);

  int typeIndexed(int count, const int _blocklens_[],
               const int* _indices_,
               MPI_Datatype intype, MPI_Datatype* outtype);

  int typeHindexed(int count, const int _blocklens_[],
              const MPI_Aint* _indices_,
               MPI_Datatype intype, MPI_Datatype* outtype);


  int typeContiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type);

  int typeVector(int count, int blocklength, int stride,
              MPI_Datatype old_type,
              MPI_Datatype* new_type);

  int typeHvector(int count, int blocklength, MPI_Aint stride,
              MPI_Datatype old_type,
              MPI_Datatype* new_type);

  int typeCreateStruct(const int count, const int* blocklens,
              const MPI_Aint* displs,
              const MPI_Datatype* old_types,
              MPI_Datatype* newtype);

  int typeCreateStruct(const int count, const int* blocklens,
              const int* displs,
              const MPI_Datatype* old_types,
              MPI_Datatype* newtype);

  int typeCommit(MPI_Datatype* type);

  int typeFree(MPI_Datatype* type);

  MpiType* typeFromId(MPI_Datatype id);

  void allocateTypeId(MpiType* type);

  std::string commStr(MpiComm* comm);

  std::string commStr(MPI_Comm comm);

  std::string tagStr(int tag);

  std::string srcStr(int id);

  std::string srcStr(MpiComm* comm, int id);

  std::string typeStr(MPI_Datatype mid);

  const char* opStr(MPI_Op op);

  MpiComm* getComm(MPI_Comm comm);

  MpiGroup* getGroup(MPI_Group grp);

  MpiRequest* getRequest(MPI_Request req);

  void addCommPtr(MpiComm* ptr, MPI_Comm* comm);

  void eraseCommPtr(MPI_Comm comm);

  void addGroupPtr(MpiGroup* ptr, MPI_Group* grp);

  void addGroupPtr(MPI_Group grp, MpiGroup* ptr);

  void eraseGroupPtr(MPI_Group grp);

  void addRequestPtr(MpiRequest* ptr, MPI_Request* req);

  void eraseRequestPtr(MPI_Request req);

  void checkKey(int key);

  void addKeyval(int key, keyval* keyval);

  keyval *getKeyval(int key);

  void finishCollective(CollectiveOpBase* op);

 private:
  int doWait(MPI_Request *request, MPI_Status *status, int& tag, int& source);

  void finalizeWaitRequest(MpiRequest* reqPtr, MPI_Request* request, MPI_Status* status);

  int doTypeHvector(int count, int blocklength, MPI_Aint stride,
              MpiType* old_type,
              MPI_Datatype* new_type);

  int doTypeHindexed(int count, const int _blocklens_[],
       const MPI_Aint* _indices_,
       MpiType* old_type, MPI_Datatype* outtype);

  void startMpiCollective(
      SST::Iris::sumi::Collective::type_t ty,
      const void* sendbuf, void* recvbuf,
      MPI_Datatype sendtype, MPI_Datatype recvtype,
      CollectiveOpBase* op);


  void* allocateTempPackBuffer(int count, MpiType* type);

  void freeTempPackBuffer(void* srcbuf);

  /**
   * @brief waitCollective The function takes ownership of the op.
   *  This should only be called with a single pending collective.
   *  If there are multiple pending collectives,
   *  this can lead to an error condition
   * @param op Moved collective operation
   */
  void waitCollective(CollectiveOpBase::ptr&& op);

  void waitCollectives(std::vector<CollectiveOpBase::ptr>&& op);

  void freeRequests(int nreqs, MPI_Request* reqs, int* inds);

  void commitBuiltinTypes();

  void commitBuiltinType(MpiType* type, MPI_Datatype id);

  std::string typeLabel(MPI_Datatype tid);

  SST::Iris::sumi::CollectiveDoneMessage*  startAllgather(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startAlltoall(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startAllreduce(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startBarrier(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startBcast(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startGather(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startReduce(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startReduceScatter(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startReduceScatterBlock(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startScan(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startScatter(CollectiveOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startAllgatherv(CollectivevOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startAlltoallv(CollectivevOp* op);

  SST::Iris::sumi::CollectiveDoneMessage*  startGatherv(CollectivevOp* op);

  SST::Iris::sumi::CollectiveDoneMessage* startScatterv(CollectivevOp* op);

  void finishCollectiveOp(CollectiveOpBase* op_);

  void finishVcollectiveOp(CollectiveOpBase* op_);

  /* Collective operations */
  CollectiveOpBase::ptr startBarrier(const char* name, MPI_Comm comm);

  CollectiveOpBase::ptr startBcast(const char* name, MPI_Comm comm, int count, MPI_Datatype datatype,
                                  int root, void *buffer);

  CollectiveOpBase::ptr
  startScatter(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
           int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startScatterv(const char* name, MPI_Comm comm, const int *sendcounts, MPI_Datatype sendtype, int root,
                 const int *displs, int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startGather(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
               int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startGatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
          const int *recvcounts, const int *displs, MPI_Datatype recvtype,
          const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startAllgather(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
            int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startAllgatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                   const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startAlltoall(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                 int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf);

  CollectiveOpBase::ptr
  startAlltoallv(const char* name, MPI_Comm comm, const int *sendcounts, MPI_Datatype sendtype, const int *sdispls,
            const int *recvcounts, MPI_Datatype recvtype, const int *rdispls,
            const void *sendbuf,  void *recvbuf);

  CollectiveOpBase::ptr
  startReduce(const char* name, MPI_Comm comm, int count, MPI_Datatype type, int root,
               MPI_Op op, const void* src, void* dst);

  CollectiveOpBase::ptr
  startAllreduce(const char* name, MPI_Comm comm, int count, MPI_Datatype type,
               MPI_Op op, const void* src, void* dst);

  CollectiveOpBase::ptr
  startAllreduce(MpiComm* commPtr, int count, MPI_Datatype type,
               MPI_Op op, const void* src, void* dst);

  CollectiveOpBase::ptr
  startReduceScatter(const char* name, MPI_Comm comm, const int* recvcounts, MPI_Datatype type,
                       MPI_Op op, const void* src, void* dst);

  CollectiveOpBase::ptr
  startReduceScatterBlock(const char* name, MPI_Comm comm, int count, MPI_Datatype type,
                             MPI_Op op, const void* src, void* dst);

  CollectiveOpBase::ptr
  startScan(const char* name, MPI_Comm comm, int count, MPI_Datatype type,
             MPI_Op op, const void* src, void* dst);

  void doStart(MPI_Request req);

  MpiRequest* addImmediateCollective(CollectiveOpBase::ptr&& op);

  void addImmediateCollective(CollectiveOpBase::ptr&& op, MPI_Request* req);

  void finishCurrentMpiCall();

  void setNewMpiCall(MPI_function func){
    current_call_.ID = func;
    current_call_.start = last_collection_ = now();
    //update this to at least the beginning of this function
  }

  bool test(MPI_Request *request, MPI_Status *status, int& tag, int& source);

  int typeSize(MPI_Datatype type){
    int ret;
    typeSize(type, &ret);
    return ret;
  }

  SST::Iris::sumi::reduce_fxn getCollectiveFunction(CollectiveOpBase* op);

  void checkInit();

  MpiRequest* doIsend(const void *buf, int count, MPI_Datatype datatype, int dest,
                        int tag, MPI_Comm comm);
  int doRecv(void *buf, int count, MPI_Datatype datatype, int source,
            int tag, MPI_Comm comm, MPI_Status* status);

  int doIsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
            MPI_Comm comm, MPI_Request *request, bool print);

 private:
  friend class MpiCommFactory;

  MpiQueue* queue_;

  MPI_Datatype next_type_id_;

  static const MPI_Op first_custom_op_id = 1000;
  MPI_Op next_op_id_;

//  static sstmac::FTQTag mpi_tag;

  MpiCommFactory comm_factory_;

  int iprobe_delay_us_;
  int test_delay_us_;

  enum {
    is_fresh, is_initialized, is_finalizing, is_finalized
  } status_;

  bool crossed_comm_world_barrier_;

  MpiComm* worldcomm_;
  MpiComm* selfcomm_;

  std::map<MPI_Datatype, MpiType*> known_types_;
  std::map<MPI_Datatype, MpiType::ptr> allocatedTypes_;

  typedef std::unordered_map<MPI_Op, MPI_User_function*> op_map;
  op_map custom_ops_;

  typedef std::unordered_map<MPI_Comm, MpiComm*> comm_ptr_map;
  comm_ptr_map comm_map_;
  typedef std::unordered_map<MPI_Group, MpiGroup*> group_ptr_map;
  group_ptr_map grp_map_;
  MPI_Group group_counter_;

  typedef std::unordered_map<MPI_Request, MpiRequest*> req_ptr_map;
  req_ptr_map req_map_;
  MPI_Request req_counter_;

  SST::Statistics::MultiStatistic<int, //sender
                                  int, //recver
                                  int, //type
                                  int, //stage
                                  uint64_t, //byte length
                                  uint64_t, //flow ID
                                  double, //send sync delay
                                  double, //recv sync delay
                                  double, //injection delay
                                  double, //contention delay
                                  double, //min delay in absence of contention
                                  double, //active sync delay experienced by app
                                  double, //active delay experienced by app
                                  double, //time since last quiescence
                                  double  //global clock time
                                 >* delays_;

  std::unordered_map<int, keyval*> keyvals_;

  bool generate_ids_;

  uint64_t traceClock() const;

#ifdef SST_HG_OTF2_ENABLED
  OTF2Writer* OTF2Writer_;
#endif

 public:
  void logMessageDelay(SST::Iris::sumi::Message *msg, uint64_t bytes, int stage,
                       SST::Hg::TimeDelta sync_delay, SST::Hg::TimeDelta active_delay,
                       SST::Hg::TimeDelta time_since_quiesce) override;

 private:
  MPI_Call current_call_;

};

MpiApi* mask_mpi();

}

#define _StartMPICall_(fxn) \
  CallGraphAppend(fxn); \
  sstmac::sw::FTQScope scope(activeThread(), mpi_tag); \
  startAPICall()


#define StartMPICall(fxn) \
  _StartMPICall_(fxn); \
  setNewMpiCall(Call_ID_##fxn)
#define FinishMPICall(fxn) \
  mpi_api_debug(sprockit::dbg::mpi, #fxn " finished"); \
  finishCurrentMpiCall(); \
  endAPICall()

#define mpi_api_debug(flags, ...) \
  mpi_debug(commWorld()->rank(), flags, __VA_ARGS__)

#define mpi_api_cond_debug(flags, cond, ...) \
  mpi_cond_debug(commWorld()->rank(), flags, cond, __VA_ARGS__)
