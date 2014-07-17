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


#ifndef _H_EMBER_ENGINE
#define _H_EMBER_ENGINE

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>
#include <sst/core/output.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/stats/histo/histo.h>
#include <sst/core/rng/gaussian.h>
#include <sst/core/rng/constant.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sst/elements/hermes/msgapi.h>
#include <queue>
#include <vector>
#include <string>

#include "embergen.h"
#include "emberevent.h"
#include "emberinitev.h"
#include "emberfinalizeev.h"
#include "embersendev.h"
#include "emberrecvev.h"
#include "emberirecvev.h"
#include "emberisendev.h"
#include "emberwaitev.h"
#include "emberwaitallev.h"
#include "emberstartev.h"
#include "emberstopev.h"
#include "embercomputeev.h"
#include "emberbarrierev.h"
#include "emberallredev.h"
#include "emberredev.h"
#include "embergettimeev.h"

using namespace SST::Statistics;
using namespace SST::Hermes;

namespace SST {
namespace Ember {

enum EmberDataMode {
	NOBACKING,
	BACKZEROS
};

const uint32_t EMBER_SPYPLOT_NONE = 0;
const uint32_t EMBER_SPYPLOT_SEND_COUNT = 1;
const uint32_t EMBER_SPYPLOT_SEND_BYTES = 2;

class EmberEngine : public SST::Component {
public:
	EmberEngine(SST::ComponentId_t id, SST::Params& params);
	~EmberEngine();
	void setup();
	void finish();
	void init(unsigned int phase);

	void refillQueue();
	void checkQueue();
	void handleEvent(SST::Event* ev);

	void processInitEvent(EmberInitEvent* ev);
	void processFinalizeEvent(EmberFinalizeEvent* ev);
	void processSendEvent(EmberSendEvent* ev);
	void processRecvEvent(EmberRecvEvent* ev);
	void processComputeEvent(EmberComputeEvent* ev);
	void processStartEvent(EmberStartEvent* ev);
	void processStopEvent(EmberStopEvent* ev);
	void processWaitEvent(EmberWaitEvent* ev);
	void processWaitallEvent(EmberWaitallEvent* ev);
	void processIRecvEvent(EmberIRecvEvent* ev);
	void processISendEvent(EmberISendEvent* ev);
	void processBarrierEvent(EmberBarrierEvent* ev);
	void processAllreduceEvent(EmberAllreduceEvent* ev);
	void processReduceEvent(EmberReduceEvent* ev);
	void processGetTimeEvent(EmberGetTimeEvent* ev);

	void completedInit(int val);
	void completedFinalize(int val);
	void completedSend(int val);
	void completedRecv(int val);
	void completedIRecv(int val);
	void completedISend(int val);
	void completedWait(int val);
	void completedWaitWithoutDelete(int val);
	void completedWaitall(int val);
	void completedWaitallWithoutDelete(int val);
	void completedBarrier(int val);
	void completedAllreduce(int val);
	void completedReduce(int val);

	void issueNextEvent(uint32_t nanoSecDelay);
	void printHistogram(Histogram<uint32_t, uint32_t>* histo);

	void updateMap(std::map<int32_t, uint32_t>* map, const int32_t rank, const uint32_t value);
	void updateMap(std::map<int32_t, uint64_t>* map, const int32_t rank, const uint64_t value);
	void updateSpyplot(const int32_t rank, const uint64_t bytesSent);
	void generateSpyplotRank(const char* filename);

private:
	SST::Params* engineParams;
	int jobId;
	int thisRank;
	int worldSize;
	uint32_t eventCount;
	char* emptyBuffer;
	uint32_t emptyBufferSize;
	uint32_t generationPhase;
	uint32_t printStats;
	uint32_t motifCount;
	uint32_t currentMotif;
	bool continueProcessing;
	EmberDataMode dataMode;

	std::queue<EmberEvent*> evQueue;
	std::map<int32_t, uint32_t>* spyplotSends;
	std::map<int32_t, uint64_t>* spyplotSendBytes;
	uint32_t spyplotMode;

	EmberGenerator* generator;
	SST::Link* selfEventLink;
	MessageInterface* msgapi;
	Output* output;
	SST::TimeConverter* nanoTimeConverter;
	std::vector<MessageResponse> currentRecv;
	MessageRequest* currentReq;

	typedef Arg_Functor<EmberEngine, int> HermesAPIFunctor;

	HermesAPIFunctor finalizeFunctor;
	HermesAPIFunctor initFunctor;
	HermesAPIFunctor recvFunctor;
	HermesAPIFunctor sendFunctor;
	HermesAPIFunctor waitFunctor;
	HermesAPIFunctor waitNoDelFunctor;
	HermesAPIFunctor waitallFunctor;
	HermesAPIFunctor waitallNoDelFunctor;
	HermesAPIFunctor irecvFunctor;
	HermesAPIFunctor isendFunctor;
	HermesAPIFunctor barrierFunctor;
	HermesAPIFunctor allreduceFunctor;
	HermesAPIFunctor reduceFunctor;

	SSTRandomDistribution* computeNoiseDistrib;

	uint32_t getDataTypeWidth(EmberDataType theType);
	PayloadDataType convertToHermesType(EmberDataType theType);
	ReductionOperation convertToHermesReduceOp(EmberReductionOperation theOp);

	Histogram<uint32_t, uint32_t>* accumulateTime;
	uint64_t nextEventStartTimeNanoSec;

	Histogram<uint32_t, uint32_t>* histoStart;
	Histogram<uint32_t, uint32_t>* histoInit;
	Histogram<uint32_t, uint32_t>* histoFinalize;
	Histogram<uint32_t, uint32_t>* histoRecv;
	Histogram<uint32_t, uint32_t>* histoSend;
	Histogram<uint32_t, uint32_t>* histoCompute;
	Histogram<uint32_t, uint32_t>* histoWait;
	Histogram<uint32_t, uint32_t>* histoIRecv;
	Histogram<uint32_t, uint32_t>* histoISend;
	Histogram<uint32_t, uint32_t>* histoBarrier;
	Histogram<uint32_t, uint32_t>* histoAllreduce;
	Histogram<uint32_t, uint32_t>* histoReduce;

	EmberEngine();			    		// For serialization
	EmberEngine(const EmberEngine&);    // Do not implement
	void operator=(const EmberEngine&); // Do not implement

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

#endif
