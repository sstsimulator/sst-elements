//===------------------------------------------------------------*- c++ -*-===//
//
//                                     shad
//
//      the scalable high-performance algorithms and data structure library
//
//===----------------------------------------------------------------------===//
//
// copyright 2018 battelle memorial institute
//
// licensed under the apache license, version 2.0 (the "license"); you may not
// use this file except in compliance with the license. you may obtain a copy
// of the license at
//
//     http://www.apache.org/licenses/license-2.0
//
// unless required by applicable law or agreed to in writing, software
// distributed under the license is distributed on an "as is" basis, without
// warranties or conditions of any kind, either express or implied. see the
// license for the specific language governing permissions and limitations
// under the license.
//
//===----------------------------------------------------------------------===/

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <ostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sst_config.h>
#include "embertricount.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>

using namespace SST;
using namespace SST::Ember;

EmberTriCountGenerator::EmberTriCountGenerator(SST::ComponentId_t id, Params& prms):
  EmberMessagePassingGenerator(id, prms, "Null" ), params_(prms), debug_(0), generate_loop_index_(0), need_to_wait_(false),
  next_task_(0), old_i_(ULLONG_MAX), old_j_(ULLONG_MAX), num_data_ranks_(0), request_index_(-1), num_data_received_(0), num_end_messages_(0),
  free_data_requests_(false), free_datareq_buffers_(false), send_request_flag_(false), send_request_array_(nullptr), test_sends_(false)
{
  rank_ = EmberMessagePassingGenerator::rank();

  if (rank_ == 0)
    std::cout << "Running Tricount Ember motif\n";

  debug_ = params_.find<int>("arg.debug_level");

  // Set number of tasks = (NT^2 + NT) / 2 ~= 6 * number of localities
  // (from SHAD reference implementation)
  NL_ = EmberMessagePassingGenerator::size() - 1;
  double quad = ( -1.0 + std::sqrt(1.0 + 4.0 * 12.0 * NL_) ) / 2.0;
  NT_ = std::llround(quad);
  num_tasks_ = ( (NT_ * NT_) + NT_ ) / 2;

  do_constant_degree_ =  (bool) params_.find<bool>("arg.use_constant_degree",false);
  if (do_constant_degree_) {
    uint64_t scale = (uint64_t) params_.find<uint64_t>("arg.scale",0);
    if (scale == 0) { printf("scale argument required when using constant degree\n"); abort(); }
    num_vertices_ = pow(2,scale);
    constant_degree_ = (uint64_t) params_.find<uint64_t>("arg.constant_degree", 0);
    if (constant_degree_ == 0) { printf("constant_degree argument required when using constant degree\n"); abort(); }
    num_edges_ = num_vertices_ * constant_degree_;
  }
  else
    init_vertices();
  if (rank_ == 0 && debug_ > 2)
    std::cerr << "num_edges_ " << num_edges_ << std::endl;

  // Compute each first edge held by each rank
  first_edges();

  // Compute start index of each task
  starts();
}

void
EmberTriCountGenerator::init_vertices() {

  // Read in precomputed Vertices (start index of vertex i's edges)
  std::string vertices_filename( (std::string) params_.find<std::string>("arg.vertices_filename") );
  std::ifstream infile;
  infile.open(vertices_filename);
  if (!infile.is_open()) { printf("File open failed\n"); abort(); }
  infile >> num_edges_;
  bool end(infile.eof());
  if (end) { printf("Couldn't read number of edges from file\n"); abort(); }
  uint64_t vertex, first_edge, expected=0;
  while (!end) {
    infile >> vertex >> first_edge;
    end = infile.eof();
    if (!end) {
      if (vertex != expected) { printf("Missing vertex\n"); abort(); }
      Vertices_.push_back(first_edge);
      ++expected;
    }
  }
  num_vertices_ = Vertices_.size();
  // Last vertex never holds a unique edge, Vertices_ entry is equal to "last edge index + 1"
  // Push back that entry one more time so we can compute zero edges by difference
  Vertices_.push_back(first_edge);


  if (rank_ == 0 && debug_ > 2) {
    for (int i=0; i<Vertices_.size(); ++i)
      std::cerr << i << " " << Vertices_[i] << std::endl;
  }
}

void
EmberTriCountGenerator::first_edges() {
  // Compute first edge that each rank holds in distributed array
  // Rank 0 doesn't hold data in this motif (NL_ is num_ranks - 1)
  // This data distributions matches a SHAD distributed array
  uint64_t remainder = num_edges_ % NL_;
  uint64_t pivot = NL_ - remainder;
  uint64_t div = num_edges_ / NL_;
  first_edges_.resize(NL_ + 2);
  first_edges_[0] = 0;
  first_edges_[1] = 0;
  for (int i=2; i <= NL_; ++i) {
    first_edges_[i] = first_edges_[i-1] + div;
    if (i > pivot + 1) ++first_edges_[i];
  }
  // first_edges_[NL_ + 1] is used to compute the number of edges rank NL_ holds
  first_edges_[NL_ + 1] = first_edges_[NL_] + div;
  if (NL_ > pivot) ++first_edges_[NL_ + 1];
}

void
EmberTriCountGenerator::starts() {
  // Algorithm adapted from SHAD reference implementation
  uint64_t task = 1;
  uint64_t incr = (1.5 * num_edges_) / NT_;
  uint64_t edge_target = incr;

  uint64_t vertex=0;
  taskStarts_.push_back(vertex);
  if (rank_ == 0 && debug_) std::cerr << "A task starts at " << vertex << std::endl;
  for (uint64_t vertex = 0; vertex < num_vertices_; ++vertex) {  // for each vertex

    int vertex_start;
    if (do_constant_degree_)
      vertex_start = vertex * constant_degree_;
    else
      vertex_start = Vertices_[vertex];

    if (vertex_start < edge_target) continue;               // ... continue until edge target is reached
    if (vertex > 0) taskStarts_.push_back(vertex);               // ... else store start vertex for this task
    if (rank_ == 0 && debug_) std::cerr << "A task starts at " << vertex << std::endl;

    uint64_t task_size = incr + (vertex_start - edge_target);
    maxTask_ = std::max(task_size, maxTask_);             // max task size

    if (edge_target == num_edges_) break;                 // ... all done

    // incr reduces at 1/4 NT and 3/4 NT
    if      ( task < std::llround(0.25 * NT_) ) incr = (1.5 * num_edges_) / NT_;
    else if ( task < std::llround(0.75 * NT_) ) incr = (1.0 * num_edges_) / NT_;
    else                                       incr = (0.5 * num_edges_) / NT_;

    ++task;
    edge_target = std::min(vertex_start + incr, num_edges_);
  }
  if (rank_ == 0 && debug_) std::cerr << "A (psuedo) task starts at " << num_vertices_ << std::endl;
  taskStarts_.push_back(num_vertices_);
}

bool
EmberTriCountGenerator::generate(std::queue<EmberEvent*>& evQ){
  evQ_ = &evQ;
  if (generate_loop_index_ == 0) {
    memSetBacked();
    enQ_memAlloc(evQ, &task_send_memaddr_, sizeofDataType(UINT64_T) );
    if (rank_ != 0) {
      enQ_memAlloc(evQ, &task_recv_memaddr_, sizeofDataType(UINT64_T) );
      enQ_memAlloc(evQ, &datareq_recv_memaddr_, sizeofDataType(UINT64_T) );
    }
  }

  if (rank_ == 0) return task_server();
  return task_client();
}

bool
EmberTriCountGenerator::task_server() {
  std::queue<EmberEvent*>& evQ = *evQ_;

  if (next_task_ < num_tasks_) {
    if (generate_loop_index_ > 0) {
      if (debug_ > 1) std::cerr << "task_server received from " << taskreq_recv_response_.src << " and is sending task start " << next_task_ << std::endl;
      uint64_t* send_buffer = (uint64_t*) task_send_memaddr_.getBacking();
      *send_buffer = next_task_;
      enQ_send(evQ, task_send_memaddr_, 1, UINT64_T, taskreq_recv_response_.src, TASK_ASSIGN, GroupWorld);
      ++next_task_;
    }
    if (debug_ > 1) std::cerr << "task_server starting taskreq_recv\n";
    enQ_recv(evQ, nullptr, 1, UINT64_T, Hermes::MP::AnySrc, TASK_REQUEST, GroupWorld, &taskreq_recv_response_);
    ++generate_loop_index_;
    return false;
  }

  // If we reach here, all tasks have been assigned
  if (num_end_messages_ < NL_) {
    if (debug_ > 1) std::cerr << "task_server sending TASK_NULL to " << taskreq_recv_response_.src << std::endl;
    enQ_send(evQ, task_send_memaddr_, 1, UINT64_T, taskreq_recv_response_.src, TASK_NULL, GroupWorld);
    ++num_end_messages_;
    if (num_end_messages_ < NL_) {
      if (debug_ > 1) std::cerr << "task_server out of tasks but hasn't informed all clients, starting taskreq_recv\n";
      enQ_recv(evQ, nullptr, 1, UINT64_T, Hermes::MP::AnySrc, TASK_REQUEST, GroupWorld, &taskreq_recv_response_);
      ++generate_loop_index_;
    }
    return false;
  }
  else {
    // Once all the clients have been sent TASK_NULL we know there are no outstanding data requests and we can send TASKS_COMPLETE
    if (debug_ > 1) std::cerr << "task_server sending TASKS_COMPLETE to all clients\n";
    for (int i=1; i <= NL_; ++i) {
      enQ_send(evQ, task_send_memaddr_, 1, UINT64_T, i, TASKS_COMPLETE, GroupWorld);
    }
  }

  return true;
}

bool
EmberTriCountGenerator::task_client() {
  std::queue<EmberEvent*>& evQ = *evQ_;

  if (debug_ > 1) std::cerr << "rank " << rank_ << " beginning client loop " << generate_loop_index_ << std::endl;

  if (send_request_flag_) {
    if (debug_ > 1) std::cerr << "rank " << rank_ << " send request complete" << std::endl;
    test_sends_ = false;
    send_request_flag_ = false;
    std::list<MessageRequest*>::iterator iter = send_requests_.begin();
    for (int i=0; i < send_request_index_; ++i) ++iter;
    delete *iter;
    send_requests_.erase(iter);
    if (send_requests_.size()) {
      if (debug_ > 1) std::cerr << "rank " << rank_ << " testing " << send_requests_.size() << " send requests\n";
      test_send_requests();
      ++generate_loop_index_;
      return false;
    }
  }
  else if (test_sends_) {
    if (debug_ > 1) std::cerr << "rank " << rank_ << " testing send requests" << std::endl;
    test_sends_ = false;
    if (send_requests_.size()) {
      test_send_requests();
      ++generate_loop_index_;
      return false;
    }
  }

  // setup request indexes (num_data_ranks_ may be zero)
  task_index_ = num_data_ranks_ - num_data_received_;
  datareq_index_ = num_data_ranks_ - num_data_received_ + task_recv_active_;

  // Ember oddity -- we can't copy our irecv requests into an array for a waitany
  // on the same generate loop because we don't know if the internal request objects have been constructed yet
  // so we tick/tock between waiting on requests and processing completed requests
  if (need_to_wait_) {
    wait_for_any();
    need_to_wait_ = false;
    ++generate_loop_index_;
    return false;
  }

  if (debug_ > 1) {
    std::cerr << "num_data_ranks_: " << num_data_ranks_ << std::endl;
    std::cerr << "request_index_:  " << request_index_ << std::endl;
  }

  if (generate_loop_index_ == 0) {
    request_task();
    recv_datareq();
    need_to_wait_ = true;
    ++generate_loop_index_;
    return false;
  }

  // If we have a data request then send data
  if (request_index_ == datareq_index_) {
    //datareq_recv_active_ = false;
    int requestor = any_response_.src;
    uint64_t size = datareq_recv_memaddr_.at<uint64_t>(0);
    if (debug_ > 1) std::cerr << "rank " << rank_ << " sending " << size << " to " << requestor << std::endl;
    MessageRequest *send_req = new MessageRequest();
    send_requests_.push_back(send_req);
    enQ_isend( evQ, nullptr, size, UINT64_T, requestor, DATA, GroupWorld, send_req );
    // prepare for next request
    recv_datareq();
  }

  // If we received a task then send out data requests
  else if (request_index_ == task_index_) {
    task_recv_active_ = false;
    data_recvs_complete_.clear();

    // Termination
    // TASK_NULL: no tasks left to assign, but continue to handle data requests
    // TASKS_COMPLETE: all tasks are complete, terminate
    if (any_response_.tag == TASK_NULL) {
      if (debug_) std::cerr << "rank " << rank_ << " received TASK_NULL" << std::endl;
      enQ_irecv( evQ, task_recv_memaddr_, 1, UINT64_T, 0, Hermes::MP::AnyTag, GroupWorld, &task_recv_request_);
      task_recv_active_ = true;
      need_to_wait_ = true;
      return false;
    }
    else if (any_response_.tag == TASKS_COMPLETE) {
      if (debug_) std::cerr << "rank " << rank_ << " received TASKS_COMPLETE -- Bye!\n";
      enQ_cancel( evQ, datareq_recv_request_ );
      wait_send_requests();
      return true;
    }

    uint64_t task = task_recv_memaddr_.at<uint64_t>(0);
    if (debug_) std::cerr << "rank " << rank_ << " received task " << task << std::endl;

    // Pull in data required to count triangles
    // Algorithm adapted from SHAD reference implementation

    // compute task indices
    uint64_t i = NT_ - 1 - (uint64_t) (sqrt(-8 * task + 4 * (NT_ + 1) * NT_ - 7) / 2.0 - 0.5);
    uint64_t j = task + i - ((NT_ + 1) * NT_ / 2) + ((NT_ + 1 - i) * (NT_ - i) / 2);
    if (debug_ > 1) {
      std::cerr << "i: " << i << std::endl;
      std::cerr << "j: " << j << std::endl;
    }

    uint64_t first_i, last_i, first_j, last_j, first_edge, last_edge;
    bool skip = false;
    if (i >= taskStarts_.size() - 1 || j >= taskStarts_.size() - 1 ) {
      skip = true;
      if (debug_) {
        std::cerr << "skipping last vertex task (which never holds unique edges)\n";
      }
    }
    if (i != old_i_ && !skip) {
       old_i_   = i;
       first_i = taskStarts_[i];      // first vertex of partition i
       last_i  = taskStarts_[i + 1];  // first vertex of partition i + 1
       if (do_constant_degree_) {
         first_edge = first_i * constant_degree_;
         last_edge = last_i * constant_degree_;
       }
       else {
         first_edge = Vertices_[first_i];
         last_edge = Vertices_[last_i];
       }
       if (first_edge != last_edge) --last_edge;
       if (debug_ > 2) {
         std::cerr << "first_i     : " << first_i << std::endl;
         std::cerr << "last_i      : " << last_i << std::endl;
         std::cerr << "i first_edge: " << first_edge << std::endl;
         std::cerr << "i last_edge : " << last_edge << std::endl;
       }
       request_edges( first_edge, last_edge );
    }

    if ((j != old_j_) && (j != i) && !skip) {
       old_j_   = j;
       first_j = taskStarts_[j];         // first vertex of partition j
       last_j  = taskStarts_[j + 1];     // first vertex of partition j + 1
       if (do_constant_degree_) {
         first_edge = first_j * constant_degree_;
         last_edge = last_j * constant_degree_;
       }
       else {
         first_edge = Vertices_[first_j];
         last_edge = Vertices_[last_j];
       }
       if (first_edge != last_edge) --last_edge;
       if (debug_ > 2) {
         std::cerr << "first_j     : " << first_j << std::endl;
         std::cerr << "last_j      : " << last_j << std::endl;
         std::cerr << "j first_edge: " << first_edge << std::endl;
         std::cerr << "j last_edge : " << last_edge << std::endl;
       }
       request_edges( first_edge, last_edge );
    }
    if (num_data_ranks_ == 0) { // all data is local, compute
      if (debug_) std::cerr << "all data is local, computing\n";
      //enQ_compute(evQ, 1000); // FIXME: need model of compute times
      request_task();
      // make send requests go away
      test_sends_ = true;
    }
  }

  // test if data is back
  else if (request_index_ < (num_data_ranks_ - num_data_received_)) {
    if (debug_ > 1) std::cerr << "rank " << rank_ << " received data with index " << request_index_ << " from " << any_response_.src << std::endl;
    ++num_data_received_;
    data_recvs_complete_.insert( datareq_index_map_[request_index_] );
    if (num_data_received_ == num_data_ranks_) {
      if (debug_) std::cerr << "rank " << rank_ << " has all remote data, computing current task\n";
      num_data_ranks_ = 0;
      num_data_received_ = 0;
      free_data_requests_ = true;
      //enQ_compute(evQ, 1000); // FIXME: need model of compute times
      request_task();
      // make send requests go away
      test_sends_ = true;
    }
    else {
      wait_for_any();
      need_to_wait_ = false;
      ++generate_loop_index_;
      return false;
    }
  }

  ++generate_loop_index_;
  need_to_wait_ = true;
  return false;
}

void
EmberTriCountGenerator::request_task() {
  std::queue<EmberEvent*>& evQ = *evQ_;
  // send a request for a task
  enQ_send( evQ, nullptr, 1, UINT64_T, 0, TASK_REQUEST, GroupWorld);
  // fire off a recv for the task request
  if (debug_ > 1) std::cerr << "rank " << rank_ << " start a task recv\n";
  enQ_irecv( evQ, task_recv_memaddr_, 1, UINT64_T, 0, Hermes::MP::AnyTag, GroupWorld, &task_recv_request_ );
  task_recv_active_ = true;
}

void
EmberTriCountGenerator::recv_datareq() {
  std::queue<EmberEvent*>& evQ = *evQ_;
  if (debug_ > 1) std::cerr << "rank " << rank_ << " start a datareq recv\n";
  enQ_irecv( evQ, datareq_recv_memaddr_, 1, UINT64_T, Hermes::MP::AnySrc, DATA_REQUEST, GroupWorld, &datareq_recv_request_);
  datareq_recv_active_ = true;
}

void
EmberTriCountGenerator::wait_for_any() {
  std::queue<EmberEvent*>& evQ = *evQ_;
  if (debug_ > 1) std::cerr << "rank " << rank_ <<  " num_data_ranks_ " << num_data_ranks_ << std::endl;
  uint64_t size = num_data_ranks_ - num_data_received_ + task_recv_active_ + datareq_recv_active_;
  if (debug_ > 1) std::cerr << "rank " << rank_ <<  " enqueing waitany with size " << size << std::endl;
  if (generate_loop_index_ > 1) delete all_requests_;
  all_requests_ = new MessageRequest[size];
  uint64_t index=0;
  datareq_index_map_.clear();
  for (uint64_t i=0; i<num_data_ranks_; ++i) {
    if (data_recvs_complete_.find(i) == data_recvs_complete_.end()) {
      int major=0;
      int current=0;
      while (i >= current && major < data_recv_requests_.size()) {
        if (current <= i && i < current + data_recv_requests_sizes_[major]) break;
        current += data_recv_requests_sizes_[major];
        ++major;
      }
      int minor = i - current;
      if (debug_ > 2) std::cerr << "copy data_recv_requests_[" << major << "][" << minor << "] " << data_recv_requests_[major][minor] << " to " << index << std::endl;
      all_requests_[index] = data_recv_requests_[major][minor];
      datareq_index_map_[index] = i;
      ++index;
    }
  }
  if (task_recv_active_) {
    if (debug_ > 2) std::cerr << "copy task recv " << task_recv_request_ << " to " << task_index_ << std::endl;
    all_requests_[index] = task_recv_request_;
    ++index;
  }
  if (datareq_recv_active_) {
    if (debug_ > 2) std::cerr << "copy datareq_recv " << datareq_recv_request_ << " to " << datareq_index_ << std::endl;
    all_requests_[index] = datareq_recv_request_;
  }
  enQ_waitany(evQ, size, all_requests_, &request_index_, &any_response_);
}

void
EmberTriCountGenerator::request_edges( uint64_t first_edge, uint64_t last_edge ) {
  std::queue<EmberEvent*>& evQ = *evQ_;

  // Determine what other ranks have edge data for this vertex
  std::map<uint64_t,uint64_t> rank_to_size;
  edges_to_ranks( first_edge, last_edge, rank_to_size );
  rank_to_size.erase(rank_); // don't request from ourself
  int num_nonlocal = rank_to_size.size();
  if (debug_ > 1) std::cerr << num_nonlocal << " ranks have nonlocal data for this partition\n";
  num_data_ranks_ += num_nonlocal;

  // get the data
  if (num_nonlocal == 0) return;  // I have all the data locally

  else {                          // Else send a data request to each rank
    if(free_data_requests_) {
      if (debug_ > 1) std::cerr << "freeing data requests\n";
      while (!data_recv_requests_.empty()){
        delete data_recv_requests_.back();
        data_recv_requests_.pop_back();
        data_recv_requests_sizes_.pop_back();
      }
      while (!datareq_send_memaddrs_.empty()){
        memFree(datareq_send_memaddrs_.back().getBacking());
        datareq_send_memaddrs_.pop_back();
      }
      while (!datareq_send_requests_.empty()){
        delete datareq_send_requests_.back();
        datareq_send_requests_.pop_back();
      }
      free_data_requests_ = false;
    }
    data_recv_requests_.push_back(new MessageRequest[num_nonlocal]);
    data_recv_requests_sizes_.push_back(num_nonlocal);
    if (debug_ > 1) std::cerr << "pushing back MessageRequest array of size " << num_nonlocal << std::endl;


    int index=0;
    for (auto iter : rank_to_size) {
      uint64_t rank = iter.first;
      uint64_t size = iter.second;
      datareq_send_memaddrs_.push_back( SST::Hermes::MemAddr(memAlloc(sizeofDataType(UINT64_T))) );
      uint64_t* send_buffer = (uint64_t*) datareq_send_memaddrs_.back().getBacking();
      *send_buffer = size;
      if (debug_) std::cerr << "rank " << rank_ << " sending data request to rank " << rank << " for size " << size << std::endl;
      MessageRequest *send_req = new MessageRequest();
      datareq_send_requests_.push_back(send_req);
      enQ_isend( evQ, datareq_send_memaddrs_.back(), 1, UINT64_T, rank, DATA_REQUEST, GroupWorld, send_req);
      enQ_irecv( evQ, nullptr, size, UINT64_T, rank, DATA, GroupWorld, &data_recv_requests_.back()[index]);
      ++index;
    }
  }
}

void
EmberTriCountGenerator::edges_to_ranks(uint64_t first_edge, uint64_t last_edge, std::map<uint64_t,uint64_t>& rank_to_size) {
  for (int rank=1; rank <= NL_; ++rank) {
    uint64_t rank_first = first_edges_[rank];
    uint64_t rank_last = first_edges_[rank + 1] - 1;
    if (debug_ > 2) {
      std::cerr << "rank " << rank << " rank_first " << rank_first << std::endl;
      std::cerr << "rank " << rank << " rank_last  " << rank_last << std::endl;
    }
    if (first_edge > rank_last || last_edge < rank_first) continue;
    int size = rank_last - rank_first + 1;
    if (last_edge < rank_last) {
      size -= rank_last - last_edge;
    }
    if (first_edge > rank_first) {
      size -= first_edge - rank_first;
    }
    rank_to_size[rank] = size;
  }
}

void
EmberTriCountGenerator::test_send_requests() {
   std::queue<EmberEvent*>& evQ = *evQ_;
   int size = send_requests_.size();
   if (size == 0) return;
   if (send_request_array_) delete send_request_array_;
   send_request_array_ = new MessageRequest[size];
   int i=0;
   for(auto req : send_requests_) {
       send_request_array_[i] = *req;
       ++i;
   }
   enQ_testany( evQ, size, send_request_array_, &send_request_index_, &send_request_flag_);
}

void
EmberTriCountGenerator::wait_send_requests() {
   std::queue<EmberEvent*>& evQ = *evQ_;
   int size = send_requests_.size();
   if (size == 0) return;
   if (send_request_array_) delete send_request_array_;
   send_request_array_ = new MessageRequest[size];
   int i=0;
   for(auto req : send_requests_) {
       send_request_array_[i] = *req;
       ++i;
   }
   // these have to be complete to get here
   while (!datareq_send_requests_.empty()){
     delete datareq_send_requests_.back();
     datareq_send_requests_.pop_back();
   }
   enQ_waitall( evQ, size, send_request_array_);
}
