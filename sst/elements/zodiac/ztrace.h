
#ifndef _ZODIAC_TRACE_READER_H
#define _ZODIAC_TRACE_READER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/elements/hermes/msgapi.h>

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacTraceReader : public SST::Component {
public:

  ZodiacTraceReader(SST::ComponentId_t id, SST::Params& params);
  void setup() { }
  void finish() { }

private:
  ZodiacTraceReader();  // for serialization only
  ZodiacTraceReader(const ZodiacTraceReader&); // do not implement
  void operator=(const ZodiacTraceReader&); // do not implement

  void handleEvent( SST::Event *ev );
  virtual bool clockTic( SST::Cycle_t );

  MessageInterface* msgapi;

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
