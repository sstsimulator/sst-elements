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

#include <string>
#include <mpi_request.h>
#include <mpi_comm/mpi_comm.h>

namespace SST::MASKMPI {

CollectiveOpBase::CollectiveOpBase(MpiComm* cm) :
  packed_send(false),
  packed_recv(false),
  tag(cm->nextCollectiveTag()),
  comm(cm),
  complete(false)
{
}

CollectiveOp::CollectiveOp(int scnt, int rcnt, MpiComm *cm) :
    CollectiveOpBase(cm)
{
  sendcnt = scnt;
  recvcnt = rcnt;
}

CollectivevOp::CollectivevOp(int scnt, int* recvcnts, int* rd, MpiComm* cm) :
    CollectiveOpBase(cm),
    recvcounts(recvcnts),
    sendcounts(0), rdisps(rd)
{
  sendcnt = scnt;
  recvcnt = 0;
}

CollectivevOp::CollectivevOp(int* sendcnts, int* sd, int rcnt, MpiComm* cm) :
    CollectiveOpBase(cm),
    sendcounts(sendcnts),
    sdisps(sd)
{
  sendcnt = 0;
  recvcnt = rcnt;
}

CollectivevOp::CollectivevOp(int* sendcnts, int* sd,
                               int* recvcnts, int *rd, MpiComm* cm) :
  CollectiveOpBase(cm),
  recvcounts(recvcnts), sendcounts(sendcnts),
  sdisps(sd), rdisps(rd)
{
  sendcnt = 0;
  recvcnt = 0;
}

CollectiveOp::CollectiveOp(int count, MpiComm* cm) :
  CollectiveOpBase(cm)
{
  sendcnt = count;
  recvcnt = count;
}

MpiRequest::~MpiRequest()
{
  if (persistent_op_) delete persistent_op_;
  //do not delete - deleted elsewhere
  //if (collective_op_) delete collective_op_;
}

void
MpiRequest::complete(MpiMessage* msg)
{
  if (!cancelled_) {
    msg->buildStatus(&stat_);
  }
  complete();
}

std::string
MpiRequest::typeStr() const
{
  if (isPersistent()){
    return "persistent";
  } else if (isCollective()){
    return "collective";
  } else {
    return "regular";
  }
}

}
