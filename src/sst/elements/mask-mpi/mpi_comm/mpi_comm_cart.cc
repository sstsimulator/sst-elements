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

#include <mpi_comm/mpi_comm_cart.h>
#include <mercury/common/errors.h>

#include <memory>

namespace SST::MASKMPI {

MpiCommCart::MpiCommCart(
  MPI_Comm id,
  int rank, MpiGroup* peers,
  AppId aid, int ndims,
  const int *dims, const int *periods, int reorder) :
  MpiComm(id, rank, peers, aid, TOPO_CART),
  ndims_(ndims), 
  // Wint-in-bool-context on gcc9.2, but I don't know why just ignore for now
  reorder_(reorder) 
{
  for (int i = 0; i < ndims; i++) {
    dims_.push_back(dims[i]);
    periods_.push_back(periods[i]);
  }
}

void
MpiCommCart::set_coords(int rank, int* coords)
{
  int prev = 1;
  for (int i = 0; i < dims_.size(); i++) {
    int co = (rank / prev) % dims_[i];

    coords[i] = co;
    prev = prev * dims_[i];
  }
}

int
MpiCommCart::rank(const int* coords)
{
  int rank = 0;
  int prev = 1;
  for (int i = 0; i < dims_.size(); i++) {
    rank += coords[i] * prev;
    prev *= dims_[i];
  }
  return rank;
}

int
MpiCommCart::shift(int dir, int dis)
{

  if (dir >= (int) dims_.size()) {
    sst_hg_throw_printf(SST::Hg::HgError,
                     "mpicomm_cart::shift: dir %d is too big for dims %d",
                     dir, dims_.size());
  }
  auto coords = std::make_unique<int[]>(dims_.size());
  set_coords(rank_, coords.get());
  coords[dir] += dis;

  if (coords[dir] >= dims_[dir]) {
    if (periods_[dir]) {
      coords[dir] = coords[dir] % dims_[dir];
      return rank(coords.get());
    } else {
      return MpiComm::proc_null;
    }
  } else if (coords[dir] < 0) {
    if (periods_[dir]) {
      coords[dir] = (dims_[dir] + coords[dir]) % dims_[dir];
      return rank(coords.get());
    } else {
      return MpiComm::proc_null;
    }
  } else {
    return rank(coords.get());
  }

}

}
