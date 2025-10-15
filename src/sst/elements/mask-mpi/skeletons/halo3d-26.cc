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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mask_mpi.h>
#include <mercury/common/skeleton.h>
//#include <sstmac/replacements/sys/time.h>
//#include <sstmac/replacements/time.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

void get_position(int rank, int pex, int pey, int pez,
                  int* myX, int* myY, int* myZ)
{
  const int plane = rank % (pex * pey);
  *myY = plane / pex;
  *myX = (plane % pex) != 0 ? (plane % pex) : 0;
  *myZ = rank / (pex * pey);
}

int convert_position_to_rank(int pX, int pY, int pZ,
                             int myX, int myY, int myZ)
{
  myX = (myX + pX) % pX;
  myY = (myY + pY) % pY;
  myZ = (myZ + pZ) % pZ;
  return (myZ * (pX * pY)) + (myY * pX) + myX;
}

#define ssthg_app_name halo3d26
int USER_MAIN(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  int world_me = -1;
  int world_size = -1;


  MPI_Comm_rank(MPI_COMM_WORLD, &world_me);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  int size = world_size;
  int me = world_me;

  MPI_Comm halo_comm = MPI_COMM_WORLD;

  int pex = 1;  // pex, pey, and pez should all be overridden in test_halo3d26.py
  int pey = 1;  // otherwise will fail at (pex * pey * pez) != size) below
  int pez = 1;

  int nx = 10;
  int ny = 10;
  int nz = 10;

  int repeats = 100;
  int vars = 1;

  long sleep = 1000;

  int print = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-nx") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -nx without a value.\n");
        }

        exit(-1);
      }

      nx = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-ny") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -ny without a value.\n");
        }

        exit(-1);
      }

      ny = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-nz") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -nz without a value.\n");
        }

        exit(-1);
      }

      nz = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-pex") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -pex without a value.\n");
        }

        exit(-1);
      }

      pex = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-pey") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -pey without a value.\n");
        }

        exit(-1);
      }

      pey = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-pez") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -pez without a value.\n");
        }

        exit(-1);
      }

      pez = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-iterations") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -iterations without a value.\n");
        }

        exit(-1);
      }

      repeats = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-vars") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -vars without a value.\n");
        }

        exit(-1);
      }

      vars = atoi(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-sleep") == 0) {
      if (i == argc) {
        if (me == 0) {
          fprintf(stderr, "Error: specified -sleep without a value.\n");
        }

        exit(-1);
      }

      sleep = atol(argv[i + 1]);
      ++i;
    } else if (strcmp(argv[i], "-print") == 0){
      print = atoi(argv[i + 1]);
      ++i;
    } else {
      if (0 == me) {
        fprintf(stderr, "Unknown option: %s\n", argv[i]);
      }

      exit(-1);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if ((pex * pey * pez) != size) {
    fprintf(stderr, "Error: rank grid does not equal number of ranks.\n");
    fprintf(stderr, "%7d x %7d x %7d != %7d\n", pex, pey, pez, size);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (me == 0 && print) {
    printf("# MPI Nearest Neighbor Communication\n");
    printf("# Info:\n");
    printf("# Processor Grid:         %7d x %7d x %7d\n", pex, pey, pez);
    printf("# Data Grid (per rank):   %7d x %7d x %7d\n", nx, ny, nz);
    printf("# Iterations:             %7d\n", repeats);
    printf("# Variables:              %7d\n", vars);
    printf("# Sleep:                  %7ld\n", sleep);
  }

  int posX, posY, posZ;
  get_position(me, pex, pey, pez, &posX, &posY, &posZ);

  const int xFaceUp =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY, posZ);
  const int xFaceDown =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY, posZ);
  const int yFaceUp =
      convert_position_to_rank(pex, pey, pez, posX, posY + 1, posZ);
  const int yFaceDown =
      convert_position_to_rank(pex, pey, pez, posX, posY - 1, posZ);
  const int zFaceUp =
      convert_position_to_rank(pex, pey, pez, posX, posY, posZ + 1);
  const int zFaceDown =
      convert_position_to_rank(pex, pey, pez, posX, posY, posZ - 1);

  const int vertexA =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY - 1, posZ - 1);
  const int vertexB =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY - 1, posZ + 1);
  const int vertexC =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY + 1, posZ - 1);
  const int vertexD =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY + 1, posZ + 1);
  const int vertexE =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY - 1, posZ - 1);
  const int vertexF =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY - 1, posZ + 1);
  const int vertexG =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY + 1, posZ - 1);
  const int vertexH =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY + 1, posZ + 1);

  const int edgeA =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY - 1, posZ);
  const int edgeB =
      convert_position_to_rank(pex, pey, pez, posX, posY - 1, posZ - 1);
  const int edgeC =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY - 1, posZ);
  const int edgeD =
      convert_position_to_rank(pex, pey, pez, posX, posY - 1, posZ + 1);
  const int edgeE =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY, posZ + 1);
  const int edgeF =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY, posZ + 1);
  const int edgeG =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY, posZ - 1);
  const int edgeH =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY, posZ - 1);
  const int edgeI =
      convert_position_to_rank(pex, pey, pez, posX - 1, posY + 1, posZ);
  const int edgeJ =
      convert_position_to_rank(pex, pey, pez, posX, posY + 1, posZ + 1);
  const int edgeK =
      convert_position_to_rank(pex, pey, pez, posX + 1, posY + 1, posZ);
  const int edgeL =
      convert_position_to_rank(pex, pey, pez, posX, posY + 1, posZ - 1);

  int requestcount = 0;
  MPI_Status* status;
  status = (MPI_Status*)malloc(sizeof(MPI_Status) * 52);

  MPI_Request* requests;
  requests = (MPI_Request*)malloc(sizeof(MPI_Request) * 52);

  double* sendBuffer = nullptr;
  double* recvBuffer = nullptr;

  struct timeval start;
  struct timeval end;

  struct timespec sleepTS;
  sleepTS.tv_sec = 0;
  sleepTS.tv_nsec = sleep;

  struct timespec remainTS;

  //gettimeofday(&start, NULL);

  for (int i = 0; i < repeats; ++i) {
    requestcount = 0;
    struct timeval iter_start;
    struct timeval iter_end;
    //gettimeofday(&iter_start, NULL);

    if (nanosleep(&sleepTS, &remainTS) == EINTR) {
      while (nanosleep(&remainTS, &remainTS) == EINTR)
        ;
    }

    MPI_Irecv(recvBuffer, ny * nz * vars, MPI_DOUBLE, xFaceUp, 1000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * nz * vars, MPI_DOUBLE, xFaceUp, 1000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, ny * nz * vars, MPI_DOUBLE, xFaceDown,
              1000, halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * nz * vars, MPI_DOUBLE, xFaceDown,
              1000, halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * nz * vars, MPI_DOUBLE, yFaceUp, 2000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * nz * vars, MPI_DOUBLE, yFaceUp, 2000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * nz * vars, MPI_DOUBLE, yFaceDown,
              2000, halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * nz * vars, MPI_DOUBLE, yFaceDown,
              2000, halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * ny * vars, MPI_DOUBLE, zFaceUp, 4000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * ny * vars, MPI_DOUBLE, zFaceUp, 4000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * ny * vars, MPI_DOUBLE, zFaceDown,
              4000, halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * ny * vars, MPI_DOUBLE, zFaceDown,
              4000, halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nz * vars, MPI_DOUBLE, edgeA, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nz * vars, MPI_DOUBLE, edgeA, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * vars, MPI_DOUBLE, edgeB, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * vars, MPI_DOUBLE, edgeB, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nz * vars, MPI_DOUBLE, edgeC, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nz * vars, MPI_DOUBLE, edgeC, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * vars, MPI_DOUBLE, edgeD, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * vars, MPI_DOUBLE, edgeD, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, ny * vars, MPI_DOUBLE, edgeE, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * vars, MPI_DOUBLE, edgeE, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, ny * vars, MPI_DOUBLE, edgeF, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * vars, MPI_DOUBLE, edgeF, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, ny * vars, MPI_DOUBLE, edgeG, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * vars, MPI_DOUBLE, edgeG, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, ny * vars, MPI_DOUBLE, edgeH, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, ny * vars, MPI_DOUBLE, edgeH, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nz * vars, MPI_DOUBLE, edgeI, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nz * vars, MPI_DOUBLE, edgeI, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * vars, MPI_DOUBLE, edgeJ, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * vars, MPI_DOUBLE, edgeJ, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nz * vars, MPI_DOUBLE, edgeK, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nz * vars, MPI_DOUBLE, edgeK, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Irecv(recvBuffer, nx * vars, MPI_DOUBLE, edgeL, 8000,
              halo_comm, &requests[requestcount++]);
    MPI_Isend(sendBuffer, nx * vars, MPI_DOUBLE, edgeL, 8000,
              halo_comm, &requests[requestcount++]);

    MPI_Waitall(requestcount, requests, status);
    requestcount = 0;
    //gettimeofday(&iter_end, NULL);
    const double timeTaken = (iter_end.tv_sec-iter_start.tv_sec) + (iter_end.tv_usec-iter_start.tv_usec)*1e-6;
    if (print){
      printf("Rank %d = [%d,%d,%d] iteration %d: %12.8fs\n", me, posX, posY, posZ, i, timeTaken);
    }
  }

  //gettimeofday(&end, NULL);

  MPI_Barrier(MPI_COMM_WORLD);

  if (convert_position_to_rank(pex, pey, pez, pex / 2, pey / 2, pez / 2) ==
      me) {

    if (print){
      printf("# Results from rank: %d\n", me);

      const double timeTaken =
          (((double)end.tv_sec) + ((double)end.tv_usec) * 1.0e-6) -
          (((double)start.tv_sec) + ((double)start.tv_usec) * 1.0e-6);

      printf("Total time = %20.6f\n", timeTaken);
    }
  }

  MPI_Finalize();
  if (me == 0) printf("halo3d-26 executed successfully\n");
  return 0;
}
