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
#include <mpi_comm/mpi_comm_cart.h>
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/common/stl_string.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>

//#define StartCommCall(fxn,comm) \
//  StartMPICall(fxn); \
//  mpi_api_debug(sprockit::dbg::mpi, "%s(%s) start", #fxn, commStr(comm).c_str())

#define StartCommCall(fxn,comm)
#undef FinishMPICall
#define FinishMPICall(fxn)

namespace SST::MASKMPI {

int
MpiApi::commDup(MPI_Comm input, MPI_Comm *output)
{
  checkInit();

#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  StartCommCall(MPI_Comm_dup,input);
  MpiComm* inputPtr = getComm(input);
  MpiComm* outputPtr = comm_factory_->comm_dup(inputPtr);
  addCommPtr(outputPtr, output);
  //mpi_api_debug(sprockit::dbg::mpi, "MPI_Comm_dup(%s,*%s) finish",
  //              commStr(input).c_str(), commStr(*output).c_str());
  FinishMPICall(MPI_Comm_dup);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->addComm(outputPtr, input);
    OTF2Writer_->writer().mpi_comm_dup(start_clock, traceClock(),
                                        input, *output);
  }
#endif
  return MPI_SUCCESS;
}

int
MpiApi::commCreateGroup(MPI_Comm comm, MPI_Group group, int  /*tag*/, MPI_Comm *newcomm)
{
  checkInit();
  StartCommCall(MPI_Comm_create_group,comm);
  MpiComm* inputPtr = getComm(comm);
  MpiGroup* groupPtr = getGroup(group);
  MpiComm* outputPtr = comm_factory_->commCreateGroup(inputPtr, groupPtr);
  addCommPtr(outputPtr, newcomm);
//  mpi_api_debug(sprockit::dbg::mpi, "MPI_Comm_create_group(%s,*%s) finish",
//                commStr(comm).c_str(), commStr(*newcomm).c_str());
  FinishMPICall(MPI_Comm_create_group);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    sprockit::abort("OTF2 trace MPI_Comm_create_group unimplemented");
  }
#endif
  return MPI_SUCCESS;
}

int
MpiApi::commSize(MPI_Comm comm, int *size)
{
  StartCommCall(MPI_Comm_size,comm);
  *size = getComm(comm)->size();
  endLibraryCall();
  return MPI_SUCCESS;
}

int
MpiApi::commCreate(MPI_Comm input, MPI_Group group, MPI_Comm *output)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  StartCommCall(MPI_Comm_create,input);
  MpiComm* inputPtr = getComm(input);
  MpiGroup* groupPtr = getGroup(group);
  MpiComm* outputPtr = comm_factory_->commCreate(inputPtr, groupPtr);
  addCommPtr(outputPtr, output);
//  mpi_api_debug(sprockit::dbg::mpi, "MPI_Comm_create(%s,%d,*%s)",
//                commStr(input).c_str(), group, commStr(*output).c_str());
  FinishMPICall(MPI_Comm_create);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->addComm(outputPtr, input);
    OTF2Writer_->writer().mpi_comm_create(start_clock, traceClock(),
                                           input, group, *output);
  }
#endif
  return MPI_SUCCESS;
}

void
MpiApi::commCreateWithId(MPI_Comm input, MPI_Group group, MPI_Comm new_comm)
{
//  mpi_api_debug(sprockit::dbg::mpi, "MPI_Comm_create_with_id(%s,%d,%d)",
//                commStr(input).c_str(), group, new_comm);
  MpiGroup* groupPtr = getGroup(group);
  MpiComm* inputPtr = getComm(input);
  int new_rank = groupPtr->rankOfTask(inputPtr->myTask());
  if (new_rank != -1){ //this is actually part of the group
    MpiComm* newCommPtr = new MpiComm(new_comm, new_rank, groupPtr, sid().app_);
    addCommPtr(newCommPtr, &new_comm);
#ifdef SST_HG_OTF2_ENABLED
    if (OTF2Writer_){
      if (newCommPtr->rank() == 0){
        OTF2Writer_->addComm(newCommPtr, input);
      }
    }
#endif
  }
}

int
MpiApi::commGroup(MPI_Comm comm, MPI_Group* grp)
{
  MpiComm* commPtr = getComm(comm);
  *grp = commPtr->group()->id();
  return MPI_SUCCESS;
}

int
MpiApi::cartCreate(MPI_Comm comm_old, int ndims, const int dims[],
                    const int periods[], int reorder, MPI_Comm *comm_cart)
{
  StartCommCall(MPI_Cart_create,comm_old);
  MpiComm* incommPtr = getComm(comm_old);
  MpiComm* outcommPtr = comm_factory_->createCart(incommPtr, ndims, dims, periods, reorder);
  addCommPtr(outcommPtr, comm_cart);
  FinishMPICall(MPI_Cart_create);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    sprockit::abort("OTF2 trace MPI_Cart_create unimplemented");
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::cartGet(MPI_Comm comm, int maxdims, int dims[], int periods[],
                 int coords[])
{
  MpiComm* incommPtr = getComm(comm);
  MpiCommCart* c = safe_cast(MpiCommCart, incommPtr,
    "mpi_api::cart_get: mpi comm did not cast to mpi_comm_cart");

  for (int i = 0; i < maxdims; i++) {
    dims[i] = c->dim(i);
    periods[i] = c->period(i);
  }

  c->set_coords(c->MpiComm::rank(), coords);

  return MPI_SUCCESS;
}

int
MpiApi::cartdimGet(MPI_Comm comm, int *ndims)
{
  MpiComm* incommPtr = getComm(comm);
  MpiCommCart* c = safe_cast(MpiCommCart, incommPtr,
    "mpi_api::cartdim_get: mpi comm did not cast to mpi_comm_cart");
  *ndims = c->ndims();

  return MPI_SUCCESS;
}

int
MpiApi::cartRank(MPI_Comm comm, const int coords[], int *rank)
{
  MpiComm* incommPtr = getComm(comm);
  MpiCommCart* c = safe_cast(MpiCommCart, incommPtr,
    "mpi_api::cart_rank: mpi comm did not cast to mpi_comm_cart");
  *rank = c->rank(coords);

  return MPI_SUCCESS;
}

int
MpiApi::cartShift(MPI_Comm comm, int direction, int disp, int *rank_source,
                  int *rank_dest)
{
  MpiComm* incommPtr = getComm(comm);
  MpiCommCart* c = safe_cast(MpiCommCart, incommPtr,
    "mpi_api::cart_shift: mpi comm did not cast to mpi_comm_cart");
  *rank_source = c->shift(direction, -1 * disp);
  *rank_dest = c->shift(direction, disp);

  return MPI_SUCCESS;
}

int
MpiApi::cartCoords(MPI_Comm comm, int rank, int  /*maxdims*/, int coords[])
{
  //mpi_api_debug(sprockit::dbg::mpi, "MPI_Cart_coords(...)");
  MpiComm* incommPtr = getComm(comm);
  MpiCommCart* c = safe_cast(MpiCommCart, incommPtr,
    "mpi_api::cart_coords: mpi comm did not cast to mpi_comm_cart");
  c->set_coords(rank, coords);

  return MPI_SUCCESS;
}


int
MpiApi::commSplit(MPI_Comm incomm, int color, int key, MPI_Comm *outcomm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  StartCommCall(MPI_Comm_split,incomm);
  MpiComm* incommPtr = getComm(incomm);
  MpiComm* outcommPtr = comm_factory_->commSplit(incommPtr, color, key);

  addCommPtr(outcommPtr, outcomm);
//  mpi_api_debug(sprockit::dbg::mpi,
//      "MPI_Comm_split(%s,%d,%d,*%s) exit",
//      commStr(incomm).c_str(), color, key, commStr(*outcomm).c_str());

  //but also assign an id to the underlying group
  if (outcommPtr->id() != MPI_COMM_NULL){
    outcommPtr->group()->setId(group_counter_++);
    if (smp_optimize_){
      outcommPtr->createSmpCommunicator(smp_neighbors_, engine(), Iris::sumi::Message::default_cq);
    }
  }

  FinishMPICall(MPI_Comm_split);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->addComm(outcommPtr, incomm);
    OTF2Writer_->writer().mpi_comm_split(start_clock, traceClock(), incomm, color, key, *outcomm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::commFree(MPI_Comm* input)
{
  StartCommCall(MPI_Comm_free,*input);
#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    //If tracing OTF2, I am not allowed to delete any communicators
    //TODO this needs to pass in the comm id
    //OTF2Writer_.generic_call(start_clock, traceClock(), "MPI_Comm_free");
  } else
#endif
  {
    MpiComm* inputPtr = getComm(*input);
    comm_map_.erase(*input);
    if (inputPtr->deleteGroup()){
      grp_map_.erase(inputPtr->group()->id());
    }
    delete inputPtr;
  }
  *input = MPI_COMM_NULL;
  FinishMPICall(MPI_Comm_free);

  return MPI_SUCCESS;
}

int
MpiApi::commGetAttr(MPI_Comm, int  /*comm_keyval*/, void*  /*attribute_val*/, int *flag)
{
  /**
  if (comm_keyval == MPI_TAG_UB){
    *attribute_val = std::numeric_limits<int>::max();
    *flag = 1;
  } else {
    *flag = 0;
  }
  */
  *flag = 0;

  return MPI_SUCCESS;
}

}
