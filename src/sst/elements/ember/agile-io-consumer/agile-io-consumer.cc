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

int agileIOconsumer::memory_bitmask = 0;

agileIOconsumer::agileIOconsumer(SST::ComponentId_t id, Params& prms) : EmberMessagePassingGenerator(id, prms, "Null")
{
    rank_ = EmberMessagePassingGenerator::rank();

  if (rank_ == 0) {
    std::cout << "Running agileIOconsumer motif\n";
  }

  prms.find_array<int>("arg.IONodes", ionodes);

  count = ionodes.size();

  long buffer_size = combined_read_size / count;

  iteration = -1;
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

  if (iteration == -1) {
    // Handle memory allocation
    memSetBacked();
    if (rank_ == 1) {
      // Rank 1 is Blue
      enQ_memAlloc(evQ, &sendBuf, sizeof(Ember::PacketHeader));
      recvBuf = new Hermes::MemAddr[count];
      for (int i = 0; i < count; i++) {
        enQ_memAlloc(evQ, &recvBuf[i], sizeof(Ember::PacketHeader));
      }
      memory_bitmask |= (1 << rank_);
    }
    if (kind == Green) {
      enQ_memAlloc(evQ, &sendBuf, sizeof(Ember::PacketHeader));
      memory_bitmask |= (1 << rank_);
    }
  }

  if (memory_bitmask != magicNumber) {
    return false;
  }

  iteration++;

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
    int count = ionodes.size();
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
    for (int i = 0; i < ionodes.size(); i++) {
      // send( Queue& q, Addr payload, uint32_t count, PayloadDataType dtype,
      // RankID dest, uint32_t tag, Communicator group)
      char buffer[100];
      std::stringstream sstream;
      sstream << "file_request: " << total_request_size << " " << rank_;
      std::string payload_string = sstream.str();
      auto len = payload_string.copy(buffer, payload_string.length());
      buffer[len] = '\0';
      enQ_send(evQ, buffer, len, CHAR, ionodes[i], 0, GroupWorld);
      enQ_recv(evQ, recvBuf[i], total_request_size / ionodes.size(), CHAR, MPI_ANY_SOURCE, 0, GroupWorld);
    }

    return true;
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
  std::array<char, 100> incoming_payload;

  std::string str;
  long count;
  uint64_t target;

  enQ_recv(evQ, incoming_payload.data(), 100, CHAR, MPI_ANY_SOURCE, 0, GroupWorld);
  std::stringstream foo(incoming_payload.data());
  foo >> str >> count >> target;
  enQ_send(evQ, sendBuf, count, CHAR, target, 0, GroupWorld);

  return false;
}
