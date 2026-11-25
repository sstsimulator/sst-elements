/**
Copyright 2009-2026 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2026, NTESS

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

// Self-checking MPI_Allreduce skeleton. Every rank contributes (rank+1) into
// each of N elements; the result in every element must be n(n+1)/2. Uses a
// multi-element buffer so a reduce-scatter's per-rank chunking (and remainder
// for non-power-of-2 rank counts) is exercised. The algorithm the engine runs
// is selected via app1.collective.allreduce / SUMI_ALLREDUCE_ALG, so this app
// checks correctness and, with a selected algorithm, proves the selection.

#define ssthg_app_name allreduce

#include <stdio.h>

#include <mask_mpi.h>
#include <mercury/common/skeleton.h>

#define NELEMS 128

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int* in = new int[NELEMS];
    int* out = new int[NELEMS];
    for (int i = 0; i < NELEMS; ++i) { in[i] = rank + 1; out[i] = -1; }

    MPI_Allreduce(in, out, NELEMS, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    int expected = size * (size + 1) / 2;
    int bad = -1;
    for (int i = 0; i < NELEMS; ++i) {
      if (out[i] != expected) { bad = i; break; }
    }

    if (bad >= 0) {
      printf("FAIL: allreduce rank %d elem %d got %d expected %d\n",
             rank, bad, out[bad], expected);
    } else if (rank == 0) {
      printf("PASS: allreduce (%d ranks, %d elems, SUM=%d)\n",
             size, NELEMS, expected);
    }

    delete[] in;
    delete[] out;
    MPI_Finalize();
    return 0;
}
