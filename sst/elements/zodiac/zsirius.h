
#ifndef _ZODIAC_SIRIUS_TRACE_READER_H
#define _ZODIAC_SIRIUS_TRACE_READER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/output.h>

#include <stdio.h>
#include <stdlib.h>
#include <sst/elements/hermes/msgapi.h>

#include "siriusreader.h"
#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacSiriusTraceReader : public SST::Component {
public:

  ZodiacSiriusTraceReader(SST::ComponentId_t id, SST::Component::Params_t& params);
  void setup();
  void finish();
  void init(unsigned int phase);

private:
  ~ZodiacSiriusTraceReader();
  ZodiacSiriusTraceReader();  // for serialization only
  ZodiacSiriusTraceReader(const ZodiacSiriusTraceReader&); // do not implement
  void operator=(const ZodiacSiriusTraceReader&); // do not implement

  void handleEvent( SST::Event *ev );
  void handleSelfEvent(SST::Event *ev);
  virtual bool clockTic( SST::Cycle_t );
  void handleComputeEvent(ZodiacEvent* zEv);
  void handleSendEvent(ZodiacEvent* zEv);
  void handleRecvEvent(ZodiacEvent* zEv);
  void handleIRecvEvent(ZodiacEvent* zEv);
  void handleInitEvent(ZodiacEvent* zEv);
  void handleWaitEvent(ZodiacEvent* zEv);
  void handleFinalizeEvent(ZodiacEvent* zEv);
  void handleAllreduceEvent(ZodiacEvent* zEv);
  void handleBarrierEvent(ZodiacEvent* zEv);
  void completedFunction(int val);
  void completedRecvFunction(int val);

  ////////////////////////////////////////////////////////

  typedef Arg_Functor<ZodiacSiriusTraceReader, int> DerivedFunctor;

  Output zOut;
  MessageInterface* msgapi;
  SiriusReader* trace;
  std::queue<ZodiacEvent*>* eventQ;
  SST::Link* selfLink;
  SST::TimeConverter* tConv;
  char* emptyBuffer;
  uint32_t emptyBufferSize;
  DerivedFunctor retFunctor;
  DerivedFunctor recvFunctor;
  std::map<uint64_t, MessageRequest*> reqMap;
  MessageResponse* currentRecv;
  int rank;
  string trace_file;
  int verbosityLevel;

  uint64_t zSendCount;
  uint64_t zRecvCount;
  uint64_t zIRecvCount;
  uint64_t zWaitCount;

  uint64_t zSendBytes;
  uint64_t zRecvBytes;
  uint64_t zIRecvBytes; 

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
