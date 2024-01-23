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

#include <mpi_api.h>
#include <mpi_queue/mpi_queue.h>
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>
#include <cassert>

#undef StartMPICall
#define StartMPICall(fxn)
#undef FinishMPICall
#define FinishMPICall(fxn)

namespace SST::MASKMPI {

int
MpiApi::wait(MPI_Request *request, MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
  MPI_Request request_cpy = *request;
#endif

  StartMPICall(MPI_Wait);
  //mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request, "MPI_Wait(...)");

  int tag, source;
  int rc = doWait(request, status, tag, source);
  FinishMPICall(MPI_Wait);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_) {
    dumpi::OTF2_Writer::mpi_status_t stat;
    stat.tag = tag;
    stat.source = source;
    OTF2Writer_->writer().mpi_wait(start_clock, traceClock(), request_cpy, &stat);
  }
#endif
  return rc;
}

int
MpiApi::doWait(MPI_Request *request, MPI_Status *status, int& tag, int& source)
{
  MPI_Request req = *request;
  if (req == MPI_REQUEST_NULL){
    return MPI_SUCCESS;
  }

//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request,
//    "   MPI_Wait_nonnull(%d)", req);

  MpiRequest* reqPtr = getRequest(req);
  if (!reqPtr->isComplete()){
   queue_->progressLoop(reqPtr);
  }

  tag = reqPtr->status().MPI_TAG;
  source = reqPtr->status().MPI_SOURCE;

  finalizeWaitRequest(reqPtr, request, status);

  return MPI_SUCCESS;
}

void
MpiApi::finalizeWaitRequest(MpiRequest* reqPtr, MPI_Request* req, MPI_Status* status)
{
  if (status != MPI_STATUS_IGNORE){
    *status = reqPtr->status();
  }
  if (!reqPtr->isPersistent()){
    req_map_.erase(*req);
    *req = MPI_REQUEST_NULL;
    delete reqPtr;
  }
}

int
MpiApi::waitall(int count, MPI_Request array_of_requests[],
                 MPI_Status array_of_statuses[])
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
  std::vector<MPI_Request> req_cpy(array_of_requests, array_of_requests + count);
  std::vector<dumpi::OTF2_Writer::mpi_status_t> statuses(count);
#endif

  StartMPICall(MPI_Waitall);
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request,
//    "MPI_Waitall(%d,...)", count);
  bool ignore_status = array_of_statuses == MPI_STATUSES_IGNORE;
  std::vector<MPI_Request> req_vec;
  for (int i=0; i < count; ++i){
    MPI_Status* status = ignore_status ? MPI_STATUS_IGNORE : &array_of_statuses[i];
    int tag, source;
    doWait(&array_of_requests[i], status, tag, source);
#ifdef SST_HG_OTF2_ENABLED
    if (OTF2Writer_) {
      statuses[i].tag = tag;
      statuses[i].source = source;
    }
#endif
  }
  FinishMPICall(MPI_Waitall);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_) {
    OTF2Writer_->writer().mpi_waitall(start_clock, traceClock(), count,
                                       req_cpy.data(), statuses.data());
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::waitany(int count, MPI_Request array_of_requests[], int *indx,
                 MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif


  StartMPICall(MPI_Waitany);
  //mpi_api_debug(sprockit::dbg::mpi, "MPI_Waitany(...)");
  *indx = MPI_UNDEFINED;
  std::vector<MpiRequest*> reqPtrs(count);
  int numNonnull = 0;
  bool call_completed = false;
  //use a null ptr internally to indicate ignore
  for (int i=0; i < count; ++i){
    MPI_Request req = array_of_requests[i];
    if (req != MPI_REQUEST_NULL){
      MpiRequest* reqPtr = getRequest(req);
      if (reqPtr->isComplete()){
#ifdef SST_HG_OTF2_ENABLED
        if (OTF2Writer_){
          dumpi::OTF2_Writer::mpi_status_t stat;
          stat.tag = reqPtr->status().MPI_TAG;
          stat.source = reqPtr->status().MPI_SOURCE;
          OTF2Writer_->writer().mpi_waitany(start_clock, traceClock(), req, &stat);
        }
#endif
        *indx = i;
        finalizeWaitRequest(reqPtr, &array_of_requests[i], status);
        FinishMPICall(MPI_Waitany);

        call_completed = true;
        break;
        //return MPI_SUCCESS;
      }
      reqPtrs[numNonnull++] = reqPtr;
    }
  }

  if (!call_completed && numNonnull == 0){
    sst_hg_abort_printf("MPI_Waitany: passed in all null requests, undefined behavior");
    call_completed = true;
  }

  if(!call_completed) {
    //none of them are already done
    reqPtrs.resize(numNonnull);
    queue_->startProgressLoop(reqPtrs);

    numNonnull = 0;
    for (int i=0; i < count; ++i){
      MPI_Request req = array_of_requests[i];
      if (req != MPI_REQUEST_NULL){
        MpiRequest* reqPtr = reqPtrs[numNonnull++];
        if (reqPtr->isComplete()){
#ifdef SST_HG_OTF2_ENABLED
          if (OTF2Writer_){
            dumpi::OTF2_Writer::mpi_status_t stat;
            stat.tag = reqPtr->status().MPI_TAG;
            stat.source = reqPtr->status().MPI_SOURCE;
            OTF2Writer_->writer().mpi_waitany(start_clock, traceClock(), req, &stat);
          }
#endif
          *indx = i;
          finalizeWaitRequest(reqPtr, &array_of_requests[i], status);
          FinishMPICall(MPI_Waitany);
          call_completed = true;
          break;
        }
      }
    }
  }

  if(!call_completed){
    sst_hg_throw_printf(SST::Hg::ValueError,
                    "MPI_Waitany finished, but had no completed requests");
  }

  return MPI_SUCCESS;
}

int
MpiApi::waitsome(int incount, MPI_Request array_of_requests[],
                  int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
#ifdef SST_HG_OTF2_ENABLED
  // Cache the vector before it is destroyed
  std::vector<MPI_Request> req_vect(array_of_requests, array_of_requests + incount);
  std::vector<dumpi::OTF2_Writer::mpi_status_t> statuses(incount);
  auto start_clock = traceClock();
#endif

  StartMPICall(MPI_Waitsome);
  bool ignore_status = array_of_statuses == MPI_STATUSES_IGNORE;
  //mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request, "MPI_Waitsome(...)");
  int numComplete = 0;
  int numIncomplete = 0;
  std::vector<MpiRequest*> reqPtrs(incount);
  for (int i=0; i < incount; ++i){
    MPI_Request req = array_of_requests[i];
    if (req != MPI_REQUEST_NULL){
      MpiRequest* reqPtr = getRequest(req);
      if (reqPtr->isComplete()){
#ifdef SST_HG_OTF2_ENABLED
        req_vect[i] = req;
        statuses[i].tag = reqPtr->status().MPI_TAG;
        statuses[i].source = reqPtr->status().MPI_SOURCE;
#endif
        array_of_indices[numComplete++] = i;
        finalizeWaitRequest(reqPtr, &array_of_requests[i],
           ignore_status ? MPI_STATUS_IGNORE : &array_of_statuses[i]);
      } else {
        reqPtrs[numIncomplete++] = reqPtr;
      }
    }
  }

  if (numComplete > 0){
    *outcount = numComplete;
  } else {
    reqPtrs.resize(numIncomplete);

    queue_->startProgressLoop(reqPtrs);

    for (int i=0; i < incount; ++i){
      MPI_Request req = array_of_requests[i];
      if (req != MPI_REQUEST_NULL){
        MpiRequest* reqPtr = getRequest(req);
        if (reqPtr->isComplete()){
#ifdef SST_HG_OTF2_ENABLED
          req_vect[i] = req;
          statuses[i].tag = reqPtr->status().MPI_TAG;
          statuses[i].source = reqPtr->status().MPI_SOURCE;
#endif
          array_of_indices[numComplete++] = i;
          finalizeWaitRequest(reqPtr, &array_of_requests[i],
             ignore_status ? MPI_STATUS_IGNORE : &array_of_statuses[i]);
        }
      }
    }
    *outcount = numComplete;
    FinishMPICall(MPI_Waitsome);
  }

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_waitsome(start_clock, traceClock(),
                                        req_vect.data(), *outcount, array_of_indices,
                                        statuses.data());
  }
#endif

  return MPI_SUCCESS;
}

}
