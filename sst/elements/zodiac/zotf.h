
#ifndef _ZODIAC_OTF_TRACE_READER_H
#define _ZODIAC_OTF_TRACE_READER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "stdio.h"
#include "stdlib.h"
#include <sst/elements/hermes/msgapi.h>

#include "otfreader.h"

using namespace std;
using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacOTFTraceReader : public SST::Component {
public:

  ZodiacOTFTraceReader(SST::ComponentId_t id, SST::Component::Params_t& params);
  void setup() { }
  void finish() { 
  }

private:
  ~ZodiacOTFTraceReader();
  ZodiacOTFTraceReader();  // for serialization only
  ZodiacOTFTraceReader(const ZodiacOTFTraceReader&); // do not implement
  void operator=(const ZodiacOTFTraceReader&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );

  ////////////////////////////////////////////////////////

  MessageInterface* msgapi;
  OTFReader* reader;
  std::queue<ZodiacEvent>* eventQ;

  ////////////////////////////////////////////////////////

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()

};

}
}

#endif /* _ZODIAC_TRACE_READER_H */
