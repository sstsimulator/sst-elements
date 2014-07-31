// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _prospero_H
#define _prospero_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/simpleMem.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#define PROSPERO_TRACE_TEXT 0
#define PROSPERO_TRACE_BINARY 1
#define PROSPERO_TRACE_COMPRESSED 2

using namespace std;
using namespace SST::Interfaces;

namespace SST {
namespace Prospero {

typedef enum {
	READ_SUCCESS,
	READ_FAILED_EOF
} read_trace_return;

typedef enum {
	READ,
	WRITE
} op_type;

typedef struct {
	uint64_t instruction_count;
	uint64_t memory_address;
	op_type memory_op_type;
	uint32_t size;
} memory_request;

class prospero : public SST::Component {
public:

  prospero(SST::ComponentId_t id, SST::Params& params);

  void setup()  { }
  void finish() {
	if(output_level > 0)
		std::cout << "TRACE:  Closing trace input..." << std::endl;

	if(2 == trace_format) {
#ifdef HAVE_LIBZ
		gzclose( (gzFile) trace_input );
#else
		std::cerr << "Error: trace format is compressed but libz is not available.\n");
		exit(-1);
#endif
	} else {
		fclose(trace_input);
	}

	if(output_level > 0) 
		std::cout << "TRACE:  Close of trace input complete." << std::endl;

	double sim_seconds = (double) getCurrentSimTimeNano() / (double) 1000000000.0;

	std::cout << "---------------------------------------------------------------------------" << std::endl;
	std::cout << "TRACE Statistics:" << std::endl;
	std::cout << "- Cycle/Queue Size Distribution: " << std::endl;
	std::cout << "- Requests in Flight / Cycles Request Queue Spent in State:" << std::endl;
	for(int i = 0; i < (pending_request_limit + 1); i++) {
		std::cout << "- ";

		const int digits = (i > 0) ? (int) log10( (double) i ) : 1;

		for(int j = 0; j < (25 - digits); j++) {
			std::cout << " ";
		}

		std::cout << "[" << i << "] " << queue_count_bins[i] << std::endl;
	}
        std::cout << "- Total pages created:         " << new_page_creates << std::endl;
	std::cout << "- Total page coverage (bytes): " << (new_page_creates * page_size) << std::endl;
	std::cout << "- Total page coverage (MB):    " << ((new_page_creates * page_size) / (1024.0 * 1024.0)) << std::endl;
	std::cout << "- Requests generated:          " << requests_generated << std::endl;
	std::cout << "- Read req generated:          " << read_req_generated << std::endl;
	std::cout << "- Write req generated:         " << write_req_generated << std::endl;
	std::cout << "- Split request count:         " << split_request_count << std::endl;
        std::cout << "- Total bytes read:            " << total_bytes_read << std::endl;
        std::cout << "- Total bytes written:         " << total_bytes_written << std::endl;
        std::cout << "- Simulated Time (ns):         " << getCurrentSimTimeNano() << std::endl;
	std::cout << "- Simulated Time (secs):       " << sim_seconds << std::endl;
        /*
	std::cout << "- Read Bandwidth (B/s):        " << ((double)total_bytes_read / sim_seconds) << std::endl;
        std::cout << "- Write Bandwidth (B/s):       " << ((double)total_bytes_written / sim_seconds) << std::endl;
        std::cout << "- Bandwidth (combined):        " << ((total_bytes_read + total_bytes_written) / sim_seconds) << std::endl;
        std::cout << "- Read Bandwidth (MB/s):       " << (((double)total_bytes_read / (1024 * 1024)) / sim_seconds) << std::endl;
        std::cout << "- Write Bandwidth (MB/s):      " << (((double)total_bytes_written / (1024 * 1024)) / sim_seconds) << std::endl;
        std::cout << "- Bandwidth (combined):        " << (((total_bytes_read + total_bytes_written) / (1024.0 * 1024.0)) / sim_seconds) << std::endl;
	*/

//	printf("- Read Bandwidth (B/s):        %7.4ef\n", ((double)total_bytes_read / sim_seconds));
//        std::cout << "- Write Bandwidth (B/s):       " << ((double)total_bytes_written / sim_seconds) << std::endl;
//        std::cout << "- Bandwidth (combined):        " << ((total_bytes_read + total_bytes_written) / sim_seconds) << std::endl;
        printf("- Read Bandwidth (MB/s):       %10.4f\n", (((double)total_bytes_read / (1024 * 1024)) / sim_seconds));
        printf("- Write Bandwidth (MB/s):      %10.4f\n", (((double)total_bytes_written / (1024 * 1024)) / sim_seconds));
        printf("- Bandwidth (combined):        %10.4f\n", (((total_bytes_read + total_bytes_written) / (1024.0 * 1024.0)) / sim_seconds));

	std::cout << "---------------------------------------------------------------------------" << std::endl;

//	return 0; 
  }

  void handleEvent(SimpleMem::Request* event);
  void createPendingRequest(memory_request req);
  read_trace_return readNextRequest(memory_request* req);

private:
  prospero();  // for serialization only
  prospero(const prospero&); // do not implement
  void operator=(const prospero&); // do not implement

  virtual bool tick( SST::Cycle_t );

  FILE* trace_input;
  memory_request next_request;
  uint32_t currentFile;
  uint32_t maxFile;
  std::string tracePrefix;

  uint32_t trace_format;
  uint64_t total_bytes_read;
  uint64_t total_bytes_written;
  uint64_t max_trace_count;
  uint64_t tick_count;
  uint64_t max_tick_count;
  uint64_t requests_generated;
  uint64_t read_req_generated;
  uint64_t write_req_generated;
  uint64_t* queue_count_bins;
  int output_level;
  int pending_request_limit;
  int page_size;
  int cache_line_size;
  uint64_t next_page_start;
  uint64_t split_request_count;
  map<uint64_t, uint64_t> page_table;
  int new_page_creates;
  uint8_t* zero_buffer;
  map<SimpleMem::Request::id_t, memory_request*> pending_requests;
  bool keep_generating;
  uint64_t phys_limit;
  double timeMultiplier;
  bool performVirtualTranslation;
  uint64_t lineCount;

  SST::Output* output;
  SimpleMem* cache_link;

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

}
}
#endif /* _prospero_H */
