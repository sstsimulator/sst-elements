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

#include <mpi_request_fwd.h>
#include <mpi_queue/mpi_queue_fwd.h>
#include <mpi_message.h>
#include <mercury/common/node_address.h>

#pragma once

namespace SST::MASKMPI {

/**
 * A nested type to handle individual mpi receive requests.
 */
class MpiQueueRecvRequest  {
  friend class MpiQueue;
  friend class RendezvousGet;
  friend class Eager1;
  friend class Eager0;

 public:
  MpiQueueRecvRequest(SST::Hg::Timestamp start, MpiRequest* key, MpiQueue* queue,
                     int count, MPI_Datatype type, int source, int tag,
                     MPI_Comm comm, void* buffer);

  ~MpiQueueRecvRequest();

  bool matches(MpiMessage* msg);

  void setSeqnum(int seqnum) {
    seqnum_ = seqnum;
  }

  MpiRequest* req() const {
    return key_;
  }

  SST::Hg::Timestamp start() const {
    return start_;
  }

  bool isCancelled() const;

 private:
  /// The queue to whom we belong.
  MpiQueue* queue_;

  /// The parameters I will be matching on.
  int source_;
  int tag_;
  MPI_Comm comm_;
  int seqnum_;

  /** Where data will end up in its final form */
  void* final_buffer_;
  /** Temporary buffer that initially receives data.
   *  Needed for non-contiguous types */
  char* recv_buffer_;

  /// The parameters I will not be matching on, but are good error checks.
  int count_;
  MpiType* type_;
  MpiRequest* key_;
  SST::Hg::Timestamp start_;
};

}
