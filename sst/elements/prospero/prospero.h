
#ifndef _prospero_H
#define _prospero_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdint.h>

using namespace std;

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

  prospero(SST::ComponentId_t id, SST::Component::Params_t& params);

  int Setup()  { return 0; }
  int Finish() {
	if(output_level > 0) 
		std::cout << "TRACE:  Closing trace input..." << std::endl;

	fclose(trace_input);

	if(output_level > 0) 
		std::cout << "TRACE:  Close of trace input complete." << std::endl;
	return 0; 
  }

  read_trace_return readNextRequest(memory_request* req);

private:
  prospero();  // for serialization only
  prospero(const prospero&); // do not implement
  void operator=(const prospero&); // do not implement

  virtual bool tick( SST::Cycle_t );

  FILE* trace_input;
  memory_request next_request;
  uint64_t max_trace_count;
  uint64_t tick_count;
  uint64_t max_tick_count;
  int output_level;
  int pending_request_limit;

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

#endif /* _prospero_H */
