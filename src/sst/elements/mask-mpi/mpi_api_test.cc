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
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>

namespace SST::MASKMPI {

bool
MpiApi::test(MPI_Request *request, MPI_Status *status, int& tag, int& source)
{
//  _StartMPICall_(MPI_Test);
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request, "MPI_Test(...)");

  if (*request == MPI_REQUEST_NULL){
    return true;
  }

  MpiRequest* reqPtr = getRequest(*request);
  if (reqPtr->isComplete()){
    if (status != MPI_STATUS_IGNORE){
      *status = reqPtr->status();
    }
    tag = reqPtr->status().MPI_TAG;
    source = reqPtr->status().MPI_SOURCE;
    eraseRequestPtr(*request);
    *request = MPI_REQUEST_NULL;
    return true;
  } else {
    if (test_delay_us_){
      queue_->forwardProgress(test_delay_us_*1e-6);
    }
    return false;
  }
}

int
MpiApi::test(MPI_Request *request, int *flag, MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  MPI_Request req_cpy = *request;
  auto start_clock = traceClock();
#endif
  int tag, source;
  //_StartMPICall_(MPI_Test);
  if (test(request, status, tag, source)){
    //mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request, "MPI_Test(...)");
    *flag = 1;
  } else {
    *flag = 0;
  }
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    dumpi::OTF2_Writer::mpi_status_t stat;
    stat.tag = tag;
    stat.source = source;
    OTF2Writer_->writer().mpi_test(start_clock, traceClock(), req_cpy, *flag, &stat);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[])
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();

  // Make a copy of requests since they may mutate before OTF2 call
  std::vector<dumpi::mpi_request_t> requests(array_of_requests, array_of_requests + count);
  std::vector<dumpi::OTF2_Writer::mpi_status_t> statuses(count);
#endif

  //_StartMPICall_(MPI_Testall);
  *flag = 1;
  bool ignore_status = array_of_statuses == MPI_STATUSES_IGNORE;
  for (int i=0; i < count; ++i){
    MPI_Status* stat = ignore_status ? MPI_STATUS_IGNORE : &array_of_statuses[i];
    int tag, source;
    if (!test(&array_of_requests[i], stat, tag, source)){
      *flag = 0;
      break;
    }
#ifdef SST_HG_OTF2_ENABLED
    statuses[i].tag = tag;
    statuses[i].source = source;
#endif
  }
  if (*flag){
    //mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request,
    //  "MPI_Testall(%d,...)", count);
  }
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_testall(start_clock, traceClock(),
                          count, requests.data(), *flag, statuses.data());
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
  // Make a copy of requests since they may mutate before OTF2 call
  std::vector<dumpi::mpi_request_t> requests(array_of_requests, array_of_requests + count);
  dumpi::OTF2_Writer::mpi_status_t stat;
#endif

  //startAPICall();
  if (count == 0){
    *flag = 1;
    return MPI_SUCCESS;
  }
  *flag = 0;
  *indx = MPI_UNDEFINED;
  for (int i=0; i < count; ++i){
    int tag, source;
    if (test(&array_of_requests[i], status, tag, source)){
      *flag = 1;
      *indx = i;
#ifdef SST_HG_OTF2_ENABLED
      stat.tag = tag;
      stat.source = source;
#endif
      break;
    }
  }
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_testany(start_clock, traceClock(),
                          requests.data(), *indx, *flag, &stat);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                  int array_of_indices[], MPI_Status array_of_statuses[])
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
  // Make a copy of requests since they may mutate before OTF2 call
  std::vector<dumpi::mpi_request_t> requests(array_of_requests, array_of_requests + incount);
  std::vector<dumpi::OTF2_Writer::mpi_status_t> statuses(incount);
#endif

  //startAPICall();
  int numComplete = 0;
  bool ignore_status = array_of_statuses == MPI_STATUSES_IGNORE;
  for (int i=0; i < incount; ++i){
    MPI_Status* stat = ignore_status ? MPI_STATUS_IGNORE : &array_of_statuses[i];
    int tag, source;
    if (test(&array_of_requests[i], stat, tag, source)){
      array_of_indices[numComplete++] = i;
#ifdef SST_HG_OTF2_ENABLED
      statuses[i].tag = tag;
      statuses[i].source = source;
#endif
    }
  }
  *outcount = numComplete;
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_){
    OTF2Writer_->writer().mpi_testsome(start_clock, traceClock(),
                                        requests.data(), *outcount, array_of_indices,
                                        statuses.data());
  }
#endif

  return MPI_SUCCESS;
}


}
