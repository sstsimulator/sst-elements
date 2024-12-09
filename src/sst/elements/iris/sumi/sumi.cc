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

#include <mercury/operating_system/process/app.h>
#include <mercury/components/operating_system.h>
#include <mercury/common/util.h>
#include <iris/sumi/transport.h>
#include <iris/sumi/sim_transport.h>

using namespace SST::Hg;

//this redirection macro foobars things here
#ifdef sleep
#if sleep == ssthg_sleep
#undef sleep
#endif
#endif

namespace SST::Iris::sumi {

static SimTransport* current_transport()
{
  Thread* t = Thread::current();
  return t->getLibrary<SimTransport>("sumi");
}

static CollectiveEngine* current_engine()
{
  auto* tport = current_transport();
  return tport->engine();
}

Transport* sumi_api()
{
  return current_transport();
}

CollectiveEngine* sumi_engine()
{
  return current_engine();
}

void comm_init()
{
  auto* tport = current_transport();
  tport->init();
}

void comm_kill_process()
{
  SST::Hg::abort("unimplemented: comm kill process");
}

void comm_killNode()
{
  //SST::Hg::OperatingSystem::currentOs()->killNode();
  throw TerminateException();
}

void comm_finalize()
{
  current_transport()->finish();
}

CollectiveDoneMessage*
comm_allreduce(void *dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
               int cq_id, Communicator* comm)
{
  return current_engine()->allreduce(dst, src, nelems, type_size, tag, fxn, cq_id, comm);
}

CollectiveDoneMessage* comm_scan(void *dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
               int cq_id, Communicator* comm)
{
  return current_engine()->scan(dst, src, nelems, type_size, tag, fxn, cq_id, comm);
}

CollectiveDoneMessage*
comm_reduce(int root, void *dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn,
            int cq_id, Communicator* comm)
{
  return current_engine()->reduce(root, dst, src, nelems, type_size, tag, fxn, cq_id, comm);
}

CollectiveDoneMessage* comm_alltoall(void *dst, void *src, int nelems, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->alltoall(dst, src, nelems, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_allgather(void *dst, void *src, int nelems, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->allgather(dst, src, nelems, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_allgatherv(void *dst, void *src, int* recv_counts, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->allgatherv(dst, src, recv_counts, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_gather(int root, void *dst, void *src, int nelems, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->gather(root, dst, src, nelems, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_scatter(int root, void *dst, void *src, int nelems, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->scatter(root, dst, src, nelems, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_bcast(int root, void *buffer, int nelems, int type_size, int tag, int cq_id, Communicator* comm)
{
  return current_engine()->bcast(root, buffer, nelems, type_size, tag, cq_id, comm);
}

CollectiveDoneMessage* comm_barrier(int tag, int cq_id, Communicator* comm)
{
  return current_engine()->barrier(tag, cq_id, comm);
}

int comm_rank()
{
  return current_transport()->rank();
}

int comm_nproc()
{
  return current_transport()->nproc();
}

Message* comm_poll()
{
  return current_transport()->blockingPoll(Message::default_cq);
}

double wall_time()
{
  return OperatingSystem::currentOs()->now().sec();
}

}
