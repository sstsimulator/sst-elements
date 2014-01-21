
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
#include <string.h>
#include <sst/elements/hermes/msgapi.h>

#include "siriusreader.h"
#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacSiriusTraceReader : public SST::Component {
public:

  ZodiacSiriusTraceReader(SST::ComponentId_t id, SST::Params& params);
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
  void completedWaitFunction(int val);
  void completedAllreduceFunction(int val);
  void completedInitFunction(int val);
  void completedSendFunction(int val);
  void completedFinalizeFunction(int val);
  void completedIrecvFunction(int val);
  void completedBarrierFunction(int val);

  void enqueueNextEvent();

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

  DerivedFunctor allreduceFunctor;
  DerivedFunctor barrierFunctor;
  DerivedFunctor finalizeFunctor;
  DerivedFunctor initFunctor;
  DerivedFunctor irecvFunctor;
  DerivedFunctor recvFunctor;
  DerivedFunctor retFunctor;
  DerivedFunctor sendFunctor;
  DerivedFunctor waitFunctor;

  std::map<uint64_t, MessageRequest*> reqMap;
  MessageResponse* currentRecv;
  int rank;
  string trace_file;
  int verbosityLevel;

  uint64_t zSendCount;
  uint64_t zRecvCount;
  uint64_t zIRecvCount;
  uint64_t zWaitCount;
  uint64_t zAllreduceCount;

  uint64_t zSendBytes;
  uint64_t zRecvBytes;
  uint64_t zIRecvBytes;
  uint64_t zAllreduceBytes;

  uint64_t nanoCompute;
  uint64_t nanoSend;
  uint64_t nanoRecv;
  uint64_t nanoAllreduce;
  uint64_t nanoBarrier;
  uint64_t nanoInit;
  uint64_t nanoFinalize;
  uint64_t nanoWait;
  uint64_t nanoIRecv;

  uint64_t currentlyProcessingWaitEvent;
  uint64_t nextEventStartTimeNano;
  uint64_t* accumulateTimeInto;
  double scaleCompute;

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
