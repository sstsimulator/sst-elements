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

#define ssthg_app_name alltoall

#include <stdio.h>
#include <stdlib.h>

#include <mask_mpi.h>
#include <mercury/common/skeleton.h>

int main(int argc, char* argv[])
{
    MPI_Init(&argc, &argv);

    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const int problem_factor = 128;
    const int problem_size = size * problem_factor;
    const int source_stride = 1000000;
    const int destination_stride = 1000;
    int* values = new int[problem_size];
    for (int destination = 0; destination < size; ++destination) {
      for (int element = 0; element < problem_factor; ++element) {
        int index = destination * problem_factor + element;
        values[index] = rank * source_stride +
                        destination * destination_stride + element;
      }
    }

    int* recv_values = new int[problem_size];
    MPI_Alltoall(values, problem_factor, MPI_INT, recv_values, problem_factor, MPI_INT, MPI_COMM_WORLD);

    int bad_source = -1;
    int bad_element = -1;
    int expected = 0;
    int actual = 0;
    for (int source = 0; source < size && bad_source < 0; ++source) {
      for (int element = 0; element < problem_factor; ++element) {
        int index = source * problem_factor + element;
        expected = source * source_stride +
                   rank * destination_stride + element;
        actual = recv_values[index];
        if (actual != expected) {
          bad_source = source;
          bad_element = element;
          break;
        }
      }
    }

    if (bad_source < 0) {
      printf("Rank %d PASS\n", rank);
    } else {
      printf("Rank %d mismatch:\n"
             "  source=%d\n"
             "  element=%d\n"
             "  expected=%d\n"
             "  actual=%d\n"
             "Rank %d FAIL\n",
             rank, bad_source, bad_element, expected, actual, rank);
    }

    delete[] recv_values;
    delete[] values;

    MPI_Finalize();

    return bad_source < 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
