// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


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
using namespace SST::Hermes::MP;

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
  bool completedFunction(int val);
  bool completedRecvFunction(int val);
  bool completedWaitFunction(int val);
  bool completedAllreduceFunction(int val);
  bool completedInitFunction(int val);
  bool completedSendFunction(int val);
  bool completedFinalizeFunction(int val);
  bool completedIrecvFunction(int val);
  bool completedBarrierFunction(int val);

  void enqueueNextEvent();

  ////////////////////////////////////////////////////////

  typedef Arg_Functor<ZodiacSiriusTraceReader, int, bool> DerivedFunctor;

  Output zOut;
  OS* os;
  MP::Interface* msgapi;
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

};

}
}

#endif /* _ZODIAC_TRACE_READER_H */
