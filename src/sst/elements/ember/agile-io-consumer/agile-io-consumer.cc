// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <sst_config.h>
#include "agile-io-consumer.h"
#include "emberevent.h"
#include "event.h"
#include "mpi/embermpigen.h"
#include "params.h"
#include "sst/elements/hermes/hermes.h"
#include "sst/elements/hermes/msgapi.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>
#include <sstream>

#ifndef MPI_ANY_SOURCE
#define MPI_ANY_SOURCE -1
#endif

using namespace SST;
using namespace SST::Ember;

agileIOconsumer::agileIOconsumer(SST::ComponentId_t id, Params& prms) : EmberMessagePassingGenerator(id, prms, "Null")
{
    rank_ = EmberMessagePassingGenerator::rank();

  if (rank_ == 0) {
    std::cout << "Running agileIOconsumer motif\n";
  }

  prms.find_array<int>("arg.IONodes", ionodes);

  count = ionodes.size();

  long buffer_size = combined_read_size / count;
}

void agileIOconsumer::setup()
{
  rank_ = EmberMessagePassingGenerator::rank();

  auto it = find(ionodes.begin(), ionodes.end(), rank_);
  if (it == ionodes.end()) {
    kind = Blue;
    std::cerr << "Blue\n";
  }
  else {
    kind = Green;
    std::cerr << "Green\n";
  }
}

bool
agileIOconsumer::generate(std::queue<EmberEvent*>& evQ)
{
  evQ_ = &evQ;

  if (first) {
    first = false;
    // Handle memory allocation
    memSetBacked();
    if (rank_ == 1) {
      // Rank 1 is Blue
      blue_mesgReq = new MessageRequest[count];
      blue_recvBuf = new Hermes::MemAddr[count];
      blue_sendBuf = new Hermes::MemAddr[count];
      for (int i = 0; i < count; i++) {
        enQ_memAlloc(evQ, &blue_recvBuf[i], sizeof(Ember::PacketHeader));
        enQ_memAlloc(evQ, &blue_sendBuf[i], sizeof(Ember::PacketHeader));
      }
    }
    if (kind == Green) {
      enQ_memAlloc(evQ, &green_sendBuf, sizeof(Ember::PacketHeader));
      enQ_memAlloc(evQ, &green_recvBuf, sizeof(Ember::PacketHeader));
    }

    return false;
  }

  if (rank_ == 1) return blue_request(combined_read_size);
  if (kind == Green) return green_read();
  return false;
}

void
agileIOconsumer::read_request()
{
}

int
agileIOconsumer::write_data()
{
    return 0;
}

int
agileIOconsumer::num_io_nodes()
{
    return 0;
}

// Sent to all the IO nodes
void agileIOconsumer::validate(const long total_request_size) {
    long request = total_request_size / count;
    if (request * count != total_request_size) {
      std::cerr << request << " * " << count << " != " << total_request_size
                << "\n";
      abort();
    }
}

// send each of the IO nodes a request for total_request_size
// it will be up to them to return total_request_size / num_io_nodes
bool
agileIOconsumer::broadcast_and_receive(const long &total_request_size,
                                std::queue<EmberEvent *> &evQ) {
    if (blue_round == 0) {
      blue_round++;
      for (int i = 0; i < count; i++) {
        PacketHeader* send_buffer = (PacketHeader*) blue_sendBuf[i].getBacking();
        send_buffer->dst = ionodes[i];
        send_buffer->src = rank_;
        send_buffer->len = combined_read_size;
        enQ_send(evQ, blue_sendBuf[i], PacketSize, UINT64_T, ionodes[i], Tag, GroupWorld);
      }
      return false;
    }
    else if (blue_round == 1) {
      blue_round++;
      for (int i = 0; i < count; i++) {
        PacketHeader *recv_buffer = (PacketHeader*) blue_recvBuf[i].getBacking();
        enQ_irecv(evQ, blue_recvBuf[i], PacketSize, UINT64_T, AnySrc, Tag, GroupWorld, &blue_mesgReq[i]);
        enQ_wait(evQ, &blue_mesgReq[i]);
      }
      return false;
    }
    else {
      for (int i = 0; i < count; i++) {
        PacketHeader *ph = (PacketHeader*)blue_recvBuf[i].getBacking();
        std::cerr << ph << "\n";
      }
      return true;
    }
}

bool
agileIOconsumer::blue_request(const long total_request_size) {
    std::queue<EmberEvent *> &evQ = *evQ_;

    validate(total_request_size);

    return broadcast_and_receive(total_request_size, evQ);
}

// Each IO node responds with amount of data read, possibly less than requested
bool
agileIOconsumer::green_read()
{
  std::queue<EmberEvent *> &evQ = *evQ_;
  // uint64_t target;

  if (green_read_first) {
    green_read_first = false;
    enQ_irecv(evQ, green_recvBuf, PacketSize, UINT64_T, AnySrc, Tag, GroupWorld, &green_mesgReq);
    enQ_wait(evQ, &green_mesgReq, &green_mesgResp);

    return false;
  }
  else {
    PacketHeader *ph = (PacketHeader*) green_recvBuf.getBacking();
    auto target = ph->src;
    auto request_size = ph->len;
    PacketHeader *ph2 = (PacketHeader*)green_sendBuf.getBacking();
    ph2->dst = target;
    ph2->src = rank_;
    ph2->len = request_size / count;
    enQ_send(evQ, green_sendBuf, PacketSize, UINT64_T, target, Tag, GroupWorld);
    // Now should we send a non-backed buffer of size ph2->len???

    return true;
  }
}
