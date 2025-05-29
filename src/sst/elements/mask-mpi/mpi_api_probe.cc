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
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>

//#define start_probe_call(fxn,comm,src,tag) \
//  StartMPICall(fxn); \
//  mpi_api_debug(sprockit::dbg::mpi, "%s(%s,%s,%s)", \
//    #fxn, srcStr(source).c_str(), tagStr(tag).c_str(), commStr(comm).c_str());

#define start_probe_call(fxn,comm,src,tag)

namespace SST::MASKMPI {

int
MpiApi::probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  start_probe_call(MPI_Probe,comm,source,tag);

  MpiComm* commPtr = getComm(comm);

  MpiRequest* req = MpiRequest::construct(MpiRequest::Probe);
  queue_->probe(req, commPtr, source, tag);
  queue_->progressLoop(req);

  if (status != MPI_STATUS_IGNORE){
    *status = req->status();
  }


  delete req;

  return MPI_SUCCESS;
}

int
MpiApi::iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
  start_probe_call(MPI_Iprobe,comm,source,tag);

  MpiComm* commPtr = getComm(comm);
  bool found = queue_->iprobe(commPtr, source, tag, status);
  if (found){
    *flag = 1;
//    mpi_api_debug(sprockit::dbg::mpi, "MPI_Iprobe(%s,%s,%s)",
//      srcStr(source).c_str(), tagStr(tag).c_str(), commStr(comm).c_str());
  } else {
    if (iprobe_delay_us_){
      queue_->forwardProgress(1e-6*iprobe_delay_us_);
      found = queue_->iprobe(commPtr, source, tag, status);
    }

    if (found) *flag = 1;
    else       *flag = 0;
  }

  return MPI_SUCCESS;
}


}
