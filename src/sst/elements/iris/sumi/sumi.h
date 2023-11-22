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

#ifndef sumi_msg_api_h
#define sumi_msg_api_h

#include <sumi/options.h>
#include <sumi/message.h>
#include <sumi/comm_functions.h>
#include <sumi/collective_message.h>
#include <sumi/timeout.h>
#include <sumi/communicator.h>
#include <sumi/transport.h>

namespace SST::Iris::sumi {

sumi::Transport* sumi_api();

sumi::CollectiveEngine* sumi_engine();

void comm_init();

void comm_finalize();

int comm_rank();

int comm_nproc();

template <class T, class... Args>
void rdma_get(int remote_proc, uint64_t byte_length, void* local_buffer, void* remote_buffer,
              int local_cq, int remote_cq, Args&&... args){

  sumi_api()->rdmaGet(remote_proc, byte_length, local_buffer, remote_buffer,
                       local_cq, remote_cq, sumi::Message::pt2pt,
                       std::forward<Args>(args)...);
}

template <class T, class... Args>
void rdma_put(int remote_proc, uint64_t byte_length, void* local_buffer, void* remote_buffer,
                  int local_cq, int remote_cq, Args&&... args){

  sumi_api()->rdmaPut(remote_proc, byte_length, local_buffer, remote_buffer,
                       local_cq, remote_cq, sumi::Message::pt2pt,
                       std::forward<Args>(args)...);
}

template <class T, class... Args>
void smsg_send(int remote_proc, uint64_t byte_length, void* buffer,
                   int local_cq, int remote_cq, Args&&... args){

  sumi_api()->smsgSend(remote_proc, byte_length, buffer,
                        local_cq, remote_cq, sumi::Message::pt2pt,
                        std::forward<Args>(args)...);
}

CollectiveDoneMessage* comm_alltoall(void* dst, void* src, int nelems, int type_size, int tag,
                   int cq_id, Communicator* comm = nullptr);

CollectiveDoneMessage* comm_allgather(void* dst, void* src, int nelems, int type_size, int tag,
                   int cq_id, Communicator* comm = nullptr);

CollectiveDoneMessage* comm_allgatherv(void* dst, void* src, int* recv_counts, int type_size, int tag,
                   int cq_id, Communicator* comm = nullptr);

CollectiveDoneMessage* comm_gather(int root, void* dst, void* src, int nelems, int type_size, int tag,
                 int cq_id, Communicator* comm = nullptr);

CollectiveDoneMessage* comm_scatter(int root, void* dst, void* src, int nelems, int type_size, int tag,
                  int cq_id, Communicator* comm = nullptr);

CollectiveDoneMessage* comm_bcast(int root, void* buffer, int nelems, int type_size, int tag,
                int cq_id, Communicator* comm = nullptr);

/**
* The total size of the input/result buffer in bytes is nelems*type_size
* @param dst  Buffer for the result. Can be NULL to ignore payloads.
* @param src  Buffer for the input. Can be NULL to ignore payloads.
* @param nelems The number of elements in the input and result buffer.
* @param type_size The size of the input type, i.e. sizeof(int), sizeof(double)
* @param tag A unique tag identifier for the collective
* @param fxn The function that will actually perform the reduction
* @param fault_aware Whether to execute in a fault-aware fashion to detect failures
* @param context The context (i.e. initial set of failed procs)
*/
CollectiveDoneMessage* comm_allreduce(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                    int cq_id, Communicator* comm = nullptr);

template <typename data_t, template <typename> class Op>
CollectiveDoneMessage* comm_allreduce(void* dst, void* src, int nelems, int tag, int cq_id, Communicator* comm = nullptr){
  typedef ReduceOp<Op, data_t> op_class_type;
  return comm_allreduce(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cq_id, comm);
}

CollectiveDoneMessage* comm_scan(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
               int cq_id, Communicator* comm = nullptr);

template <typename data_t, template <typename> class Op>
CollectiveDoneMessage* comm_scan(void* dst, void* src, int nelems, int tag,int cq_id, Communicator* comm = nullptr){
  typedef ReduceOp<Op, data_t> op_class_type;
  return comm_scan(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cq_id, comm);
}

CollectiveDoneMessage* comm_reduce(int root, void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                 int cq_id, Communicator* comm = nullptr);

template <typename data_t, template <typename> class Op>
CollectiveDoneMessage* comm_reduce(int root, void* dst, void* src, int nelems, int tag,int cq_id, Communicator* comm = nullptr){
  typedef ReduceOp<Op, data_t> op_class_type;
  return comm_reduce(root, dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cq_id, comm);
}

CollectiveDoneMessage* comm_barrier(int tag, int cq_id, Communicator* comm = nullptr);

Message* comm_poll();

void compute(double sec);

void sleep_hires(double sec);

void sleepUntil(double sec);

/**
 * Every node has exactly the same notion of time - universal, global clock.
 * Thus, if rank 0 starts and 10 minuts later rank 1 starts,
 * even though rank 1 has only been running for 30 seconds, the time will still return
 * 10 mins, 30 seconds.
 * @return The current system wall-clock time in seconds.
 *         At application launch, time is zero.
 */
double wall_time();

}
