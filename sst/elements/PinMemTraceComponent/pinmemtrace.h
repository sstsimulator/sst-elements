
#ifndef _PinMemTrace_H
#define _PinMemTrace_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>
#include <string>

#include <stdio.h>
#include <stdint.h>

typedef enum {
	EXECUTE_PIN,
	EXECUTE_ONLY,
	TRACE_FILE
} PIN_MEM_TRACE_TYPE;

class PinMemTrace : public SST::Component {
public:

  PinMemTrace(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup()  { return 0; }
  int Finish() 
  { 
	if(output_level > 0) 
		std::cout << "TRACE:  Closing trace input..." << std::endl;
	switch(trace_type) {
	case TRACE_FILE:
		fclose(trace_input);
		break;

	case EXECUTE_PIN:
	case EXECUTE_ONLY:
		pclose(trace_input);
		break;
	}

	if(output_level > 0) 
		std::cout << "TRACE:  Close of trace input complete." << std::endl;
	return 0; 
  }

private:
  PinMemTrace();  // for serialization only
  PinMemTrace(const PinMemTrace&); // do not implement
  void operator=(const PinMemTrace&); // do not implement

  virtual bool tick( SST::Cycle_t );

  PIN_MEM_TRACE_TYPE trace_type;
  FILE* trace_input;
  uint64_t max_trace_count;
  int output_level;
  std::string pin_path;

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
    // This is going to be a problem, we are trying to recreate
    // a file handle or alternatively a UNIX pipe.
    trace_input = NULL;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _PinMemTrace_H */
