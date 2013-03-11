
#ifndef _prospero_H
#define _prospero_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <cstring>
#include <string>

#include <stdio.h>
#include <stdint.h>

class prospero : public SST::Component {
public:

  prospero(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup()  { return 0; }
  int Finish() 
  { 
	if(output_level > 0) 
		std::cout << "TRACE:  Closing trace input..." << std::endl;

	if(output_level > 0) 
		std::cout << "TRACE:  Close of trace input complete." << std::endl;
	return 0; 
  }

private:
  prospero();  // for serialization only
  prospero(const prospero&); // do not implement
  void operator=(const prospero&); // do not implement

  virtual bool tick( SST::Cycle_t );

  FILE* trace_input;
  uint64_t max_trace_count;
  int output_level;

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

#endif /* _prospero_H */
