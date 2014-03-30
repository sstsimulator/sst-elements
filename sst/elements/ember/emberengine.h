
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
#include "emberwaitev.h"
#include "emberstartev.h"
#include "embercomputeev.h"

using namespace SST::Statistics;
using namespace SST::Hermes;

namespace SST {
namespace Ember {

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
	void processWaitEvent(EmberWaitEvent* ev);
	void processIRecvEvent(EmberIRecvEvent* ev);

	void completedInit(int val);
	void completedFinalize(int val);
	void completedSend(int val);
	void completedRecv(int val);
	void completedIRecv(int val);
	void completedWait(int val);

	void issueNextEvent(uint32_t nanoSecDelay);
	void printHistoBin(uint64_t binStart, uint64_t width, uint64_t* bin);
	

private:  
	int thisRank;
	uint32_t eventCount;
	char* emptyBuffer;
	uint32_t emptyBufferSize;
	uint32_t generationPhase;
	bool printStats;
	bool continueProcessing;

	std::queue<EmberEvent*> evQueue;
	EmberGenerator* generator;
	SST::Link* selfEventLink;
	MessageInterface* msgapi;
	Output* output;
	SST::TimeConverter* nanoTimeConverter;
	MessageResponse currentRecv;
	MessageRequest* currentReq;

	typedef Arg_Functor<EmberEngine, int> HermesAPIFunctor;

	HermesAPIFunctor finalizeFunctor;
	HermesAPIFunctor initFunctor;
	HermesAPIFunctor recvFunctor;
	HermesAPIFunctor sendFunctor;
	HermesAPIFunctor waitFunctor;
	HermesAPIFunctor irecvFunctor;

	Histogram<uint64_t>* accumulateTime;
	uint64_t nextEventStartTimeNanoSec;

	//uint64_t nanoInit;
	//uint64_t nanoFinalize;
	//uint64_t nanoSend;
	//uint64_t nanoRecv;
	//uint64_t nanoCompute;

	Histogram<uint64_t>* histoStart;
	Histogram<uint64_t>* histoInit;
	Histogram<uint64_t>* histoFinalize;
	Histogram<uint64_t>* histoRecv;
	Histogram<uint64_t>* histoSend;
	Histogram<uint64_t>* histoCompute;
	Histogram<uint64_t>* histoWait;
	Histogram<uint64_t>* histoIRecv;

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
