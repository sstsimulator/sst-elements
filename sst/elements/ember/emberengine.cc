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


#include "sst_config.h"
#include "sst/core/serialization.h"

#include "emberengine.h"

using namespace std;
using namespace SST::Statistics;
using namespace SST::Ember;

static const char* FINALIZE_HISTO_NAME = "Finalize Time";
static const char* INIT_HISTO_NAME = "Initialization Time";
static const char* RECV_HISTO_NAME = "Recv Time";
static const char* SEND_HISTO_NAME = "Send Time";
static const char* WAIT_HISTO_NAME = "Wait Time";
static const char* IRECV_HISTO_NAME = "IRecv Time";
static const char* ISEND_HISTO_NAME = "ISend Time";
static const char* BARRIER_HISTO_NAME = "Barrier Time";
static const char* ALLREDUCE_HISTO_NAME = "Allreduce Time";
static const char* REDUCE_HISTO_NAME = "Reduce Time";
static const char* COMPUTE_HISTO_NAME = "Compute Time";
static const char* START_HISTO_NAME = "Start Time";
static const char* STOP_HISTO_NAME = "Stop Time";

EmberEngine::EmberEngine(SST::ComponentId_t id, SST::Params& params) :
    Component( id ),
    generationPhase(0),
	finalizeFunctor(HermesAPIFunctor(this, &EmberEngine::completedFinalize)),
	initFunctor(HermesAPIFunctor(this, &EmberEngine::completedInit)),
	recvFunctor(HermesAPIFunctor(this, &EmberEngine::completedRecv)),
	sendFunctor(HermesAPIFunctor(this, &EmberEngine::completedSend)),
	waitFunctor(HermesAPIFunctor(this, &EmberEngine::completedWait)),
	waitNoDelFunctor(HermesAPIFunctor(this, &EmberEngine::completedWaitWithoutDelete)),
	waitallFunctor(HermesAPIFunctor(this, &EmberEngine::completedWaitall)),
	waitallNoDelFunctor(HermesAPIFunctor(this, &EmberEngine::completedWaitallWithoutDelete)),
	irecvFunctor(HermesAPIFunctor(this, &EmberEngine::completedIRecv)),
	isendFunctor(HermesAPIFunctor(this, &EmberEngine::completedISend)),
	barrierFunctor(HermesAPIFunctor(this, &EmberEngine::completedBarrier)),
	allreduceFunctor(HermesAPIFunctor(this, &EmberEngine::completedAllreduce)),
	reduceFunctor(HermesAPIFunctor(this, &EmberEngine::completedReduce))
{
	output = new Output();

	// Get the level of verbosity the user is asking to print out, default is 1
	// which means don't print much.
	uint32_t verbosity = (uint32_t) params.find_integer("verbose", 1);
	output->init("EmberEngine", verbosity, (uint32_t) 0, Output::STDOUT);

	// See if the user requested that we print statistics for this run
	printStats = ((uint32_t) (params.find_integer("printStats", 0)));

	jobId = params.find_integer("jobId", -1);
	if ( -1 == jobId ) {
		printStats = 0;
	}

	// Decide if we should back simulated data movement with real data
	// data mode 0 indicates no data at all, 1 indicates backed with zeros
	uint32_t paramDataMode = (uint32_t) params.find_integer("datamode", 0);
	switch(paramDataMode) {
	case 0:
		dataMode = NOBACKING;
		break;
	case 1:
		dataMode = BACKZEROS;
		break;
	default:
		output->fatal(CALL_INFO, -1, "Unknown backing mode: %" PRIu32 " (see \"datamode\" parameter)\n",
			paramDataMode);
	}

	if(dataMode != NOBACKING) {
		// Configure the empty buffer ready for use by MPI routines.
		emptyBufferSize = (uint32_t) params.find_integer("buffersize", 8192);
		emptyBuffer = (char*) malloc(sizeof(char) * emptyBufferSize);
		for(uint32_t i = 0; i < emptyBufferSize; ++i) {
			emptyBuffer[i] = 0;
		}
	}

	// Create the messaging interface we are going to use
	string msgiface = params.find_string("msgapi");
	if ( msgiface == "" ) {
        	msgapi = new MessageInterface();
    	} else {
        	Params hermesParams = params.find_prefix_params("hermesParams." );

        	msgapi = dynamic_cast<MessageInterface*>(loadModuleWithComponent(
                            msgiface, this, hermesParams));

        	if(NULL == msgapi) {
                	std::cerr << "Message API: " << msgiface << " could not be loaded." << std::endl;
                	exit(-1);
        	}
    	}

	// Create a noise distribution
	double compNoiseMean = (double) params.find_floating("noisemean", 1.0);
	double compNoiseStdDev = (double) params.find_floating("noisestddev", 0.1);
	string noiseType = params.find_string("noisegen", "constant");

	if("gaussian" == noiseType) {
		computeNoiseDistrib = new SSTGaussianDistribution(compNoiseMean, compNoiseStdDev);
	} else if ("constant" == noiseType) {
		computeNoiseDistrib = new SSTConstantDistribution(compNoiseMean);
	} else {
		output->fatal(CALL_INFO, -1, "Unknown computational noise distribution (%s)\n", noiseType.c_str());
	}

	// Create the generator
	string gentype = params.find_string("motif");
	if( gentype == "" ) {
		output->fatal(CALL_INFO, -1, "Error: You did not specify a generator for Ember to use (parameter is called \'generator\')\n");
	} else {
		Params generatorParams = params.find_prefix_params("motifParams.");

		generator = dynamic_cast<EmberGenerator*>( loadModuleWithComponent(gentype, this, generatorParams ) );

		if(NULL == generator) {
			output->fatal(CALL_INFO, -1, "Error: Could not load the generator %s for Ember\n", gentype.c_str());
		}
	}

	// Configure self link to handle event timing
	selfEventLink = configureSelfLink("self", "1ps",
		new Event::Handler<EmberEngine>(this, &EmberEngine::handleEvent));

	// Make sure we don't stop the simulation until we are ready
    	registerAsPrimaryComponent();
    	primaryComponentDoNotEndSim();

	// Create a time converter for our compute events
	nanoTimeConverter = Simulation::getSimulation()->getTimeLord()->getTimeConverter("1ns");

	uint64_t userBinWidth = (uint64_t) params.find_integer("compute_bin_width", 20);
	histoCompute = new Histogram<uint32_t, uint32_t>(COMPUTE_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("send_bin_width", 5);
	histoSend = new Histogram<uint32_t, uint32_t>(SEND_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("recv_bin_width", 5);
	histoRecv = new Histogram<uint32_t, uint32_t>(RECV_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("init_bin_width", 5);
	histoInit = new Histogram<uint32_t, uint32_t>(INIT_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("finalize_bin_width", 5);
	histoFinalize = new Histogram<uint32_t, uint32_t>(FINALIZE_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("start_bin_width", 5);
	histoStart = new Histogram<uint32_t, uint32_t>(START_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("wait_bin_width", 5);
	histoWait = new Histogram<uint32_t, uint32_t>(WAIT_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("irecv_bin_width", 5);
	histoIRecv = new Histogram<uint32_t, uint32_t>(IRECV_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("isend_bin_width", 5);
	histoISend = new Histogram<uint32_t, uint32_t>(ISEND_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("barrier_bin_width", 5);
	histoBarrier = new Histogram<uint32_t, uint32_t>(BARRIER_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("allreduce_bin_width", 5);
	histoAllreduce = new Histogram<uint32_t, uint32_t>(ALLREDUCE_HISTO_NAME, userBinWidth);

	userBinWidth = (uint64_t) params.find_integer("reduce_bin_width", 5);
	histoReduce = new Histogram<uint32_t, uint32_t>(REDUCE_HISTO_NAME, userBinWidth);

	// Set the accumulation to be the start
	accumulateTime = histoStart;

	continueProcessing = true;
}

EmberEngine::~EmberEngine() {
	switch(dataMode) {
	case BACKZEROS:
		// Free the big buffer we have been using
		free(emptyBuffer);
		break;
    default:
        break;
	}

	delete histoBarrier;
	delete histoIRecv;
	delete histoWait;
	delete histoStart;
	delete histoFinalize;
	delete histoInit;
	delete histoRecv;
	delete histoSend;
	delete histoCompute;
	delete histoAllreduce;
	delete histoReduce;
	delete computeNoiseDistrib;
	delete output;
	delete msgapi;
}

PayloadDataType EmberEngine::convertToHermesType(EmberDataType theType) {
	switch(theType) {
		case EMBER_F64:
			return DOUBLE;
		case EMBER_F32:
			return FLOAT;
		default:
			return CHAR;
			// Error?
	}
}

uint32_t EmberEngine::getDataTypeWidth(const EmberDataType theType) {
	switch(theType) {
		case EMBER_F64:
			return 8;
		case EMBER_F32:
			return 4;
		default:
			return 1;
			// error?
	}
}

void EmberEngine::init(unsigned int phase) {
	// Pass the init phases through to the communication layer
	msgapi->_componentInit(phase);
}

void EmberEngine::printHistogram(Histogram<uint32_t, uint32_t>* histo) {
        output->output("Histogram Min: %" PRIu32 "\n", histo->getBinStart());
        output->output("Histogram Max: %" PRIu32 "\n", histo->getBinEnd());
        output->output("Histogram Bin: %" PRIu32 "\n", histo->getBinWidth());
	for(uint32_t i = histo->getBinStart(); i <= histo->getBinEnd(); i += histo->getBinWidth()) {
		if( histo->getBinCountByBinStart(i) > 0 ) {
			output->output(" [%" PRIu32 ", %" PRIu32 "]   %" PRIu32 "\n",
				i, (i + histo->getBinWidth()), histo->getBinCountByBinStart(i));
		}
	}
}

void EmberEngine::finish() {
	// Tell the generator we are finishing the simulation
	generator->finish(output);

	if(printStats > 0) {
		output->output("Ember End Point Completed at: %" PRIu64 " ns\n", getCurrentSimTimeNano());
		output->output("Ember Statistics for Rank %" PRIu32 "\n", thisRank);

		output->output("- Histogram of compute times:\n");
		printHistogram(histoCompute);

		output->output("- Histogram of send times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoSend->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoSend->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoSend->getItemCount() == 0 ? 0 :
			(histoSend->getValuesSummed() / histoSend->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoSend);
		}

		output->output("- Histogram of irecv times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoIRecv->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoIRecv->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoIRecv->getItemCount() == 0 ? 0 :
			(histoIRecv->getValuesSummed() / histoIRecv->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoIRecv);
		}

		output->output("- Histogram of isend times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoISend->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoISend->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoISend->getItemCount() == 0 ? 0 :
			(histoISend->getValuesSummed() / histoISend->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoISend);
		}

		output->output("- Histogram of recv times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoRecv->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoRecv->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoRecv->getItemCount() == 0 ? 0 :
			(histoRecv->getValuesSummed() / histoRecv->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoRecv);
		}

		output->output("- Histogram of wait times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoWait->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoWait->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoWait->getItemCount() == 0 ? 0 :
			(histoWait->getValuesSummed() / histoWait->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoWait);
		}

		output->output("- Histogram of barrier times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoBarrier->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoBarrier->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoBarrier->getItemCount() == 0 ? 0 :
			(histoBarrier->getValuesSummed() / histoBarrier->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoBarrier);
		}

		output->output("- Histogram of allreduce times:\n");
		output->output("--> Total time:     %" PRIu32 "\n", histoAllreduce->getValuesSummed());
		output->output("--> Item count:     %" PRIu32 "\n", histoAllreduce->getItemCount());
		output->output("--> Average:        %" PRIu32 "\n",
			histoAllreduce->getItemCount() == 0 ? 0 :
			(histoAllreduce->getValuesSummed() / histoAllreduce->getItemCount()));
		if(printStats > 1) {
			output->output("- Distribution:\n");
			printHistogram(histoAllreduce);
		}
	}
}

void EmberEngine::setup() {
	// Notify communication layer we are done with init phase
	// and are now in final bring up state
	msgapi->_componentSetup();

	// Get my rank from the communication layer, we will
	// need to pass this to the generator
	thisRank = (uint32_t) msgapi->myWorldRank();
	uint32_t worldSize = (uint32_t) msgapi->myWorldSize();

	generator->configureEnvironment(output, thisRank, worldSize);

	char outputPrefix[256];
	sprintf(outputPrefix, "@t:%d:%d:EmberEngine::@p:@l: ", jobId, (int) thisRank);
	string outputPrefixStr = outputPrefix;
	output->setPrefix(outputPrefixStr);

	// Send an start event to this rank, this starts up the component
	EmberStartEvent* startEv = new EmberStartEvent();
	selfEventLink->send(startEv);

        // Add the init event to the queue if the motif is set to auto initialize true
        if(generator->autoInitialize()) {
                EmberInitEvent* initEv = new EmberInitEvent();
                evQueue.push(initEv);
        }

	// Update event count to ensure we are not correctly sync'd
	eventCount = (uint32_t) evQueue.size();
}

void EmberEngine::processStartEvent(EmberStartEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Start Event\n");

	issueNextEvent(0);
        accumulateTime = histoCompute;
}

ReductionOperation convertToHermesReductionOp(EmberReductionOperation op) {
	switch(op) {
		case EMBER_SUM:
			return SUM;
		default:
			return SUM;
	}
}

void EmberEngine::processAllreduceEvent(EmberAllreduceEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing an Allreduce Event\n");
	const uint32_t dataTypeWidth = getDataTypeWidth(ev->getElementType());

	switch(dataMode) {

	case NOBACKING:
		msgapi->allreduce(NULL, NULL,
			ev->getElementCount(), convertToHermesType(ev->getElementType()),
			convertToHermesReductionOp(ev->getReductionOperation()),
			ev->getCommunicator(), &allreduceFunctor);
		break;

	case BACKZEROS:
		assert(emptyBufferSize >= (ev->getElementCount() * dataTypeWidth * 2));
		msgapi->allreduce((Addr) emptyBuffer, (Addr) (emptyBuffer + (dataTypeWidth * ev->getElementCount())),
			ev->getElementCount(), convertToHermesType(ev->getElementType()),
			convertToHermesReductionOp(ev->getReductionOperation()),
			ev->getCommunicator(), &allreduceFunctor);
		break;
	}

	accumulateTime = histoAllreduce;
}

void EmberEngine::processReduceEvent(EmberReduceEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing an Reduce Event\n");
	const uint32_t dataTypeWidth = getDataTypeWidth(ev->getElementType());

	switch(dataMode) {
	case NOBACKING:
		msgapi->reduce(NULL, NULL,
			ev->getElementCount(), convertToHermesType(ev->getElementType()),
			convertToHermesReductionOp(ev->getReductionOperation()),
			(RankID) ev->getReductionRoot(),
			ev->getCommunicator(), &reduceFunctor);
		break;

	case BACKZEROS:
		assert(emptyBufferSize >= (ev->getElementCount() * dataTypeWidth * 2));

		msgapi->reduce((Addr) emptyBuffer, (Addr) (emptyBuffer + (dataTypeWidth * ev->getElementCount())),
			ev->getElementCount(), convertToHermesType(ev->getElementType()),
			convertToHermesReductionOp(ev->getReductionOperation()),
			(RankID) ev->getReductionRoot(),
			ev->getCommunicator(), &reduceFunctor);
		break;
	}

	accumulateTime = histoReduce;
}

void EmberEngine::processStopEvent(EmberStopEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Stop Event\n");

	primaryComponentOKToEndSim();
	if ( jobId >= 0 ) {
		output->output("%" PRIi32":Ember End Point Finalize completed at: %"
							PRIu64 " ns\n", jobId, getCurrentSimTimeNano());
	}
}

void EmberEngine::processInitEvent(EmberInitEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing an Init Event\n");
	msgapi->init(&initFunctor);

	accumulateTime = histoInit;
}

void EmberEngine::processBarrierEvent(EmberBarrierEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Barrier Event\n");
	msgapi->barrier(ev->getCommunicator(), &barrierFunctor);

	accumulateTime = histoBarrier;
}

void EmberEngine::processSendEvent(EmberSendEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Send Event (%s)\n", ev->getPrintableString().c_str());

	switch(dataMode) {
	case NOBACKING:
		msgapi->send(NULL, ev->getMessageSize(),
			CHAR, (RankID) ev->getSendToRank(),
			ev->getTag(), ev->getCommunicator(),
			&sendFunctor);
		break;

	case BACKZEROS:
		assert( emptyBufferSize >= ev->getMessageSize() );
		msgapi->send((Addr) emptyBuffer, ev->getMessageSize(),
			CHAR, (RankID) ev->getSendToRank(),
			ev->getTag(), ev->getCommunicator(),
			&sendFunctor);
		break;
	}

	accumulateTime = histoSend;
}

void EmberEngine::processWaitEvent(EmberWaitEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Wait Event (%s)\n", ev->getPrintableString().c_str());

    currentRecv.resize(1);

	if(ev->deleteRequestPointer()) {
		msgapi->wait( *ev->getMessageRequestHandle(), &currentRecv[0], &waitFunctor);
	} else {
		msgapi->wait( *ev->getMessageRequestHandle(), &currentRecv[0], &waitNoDelFunctor);
	}

	// Keep track of the current request handle, we will free this auto(magically).
	currentReq = ev->getMessageRequestHandle();

	accumulateTime = histoWait;
}

void EmberEngine::processWaitallEvent(EmberWaitallEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Waitall Event (%s)\n", ev->getPrintableString().c_str());

    int numReq = ev->getNumMessageRequests();
	currentRecv.resize(numReq);

	if(ev->deleteRequestPointer()) {
		msgapi->waitall(numReq, ev->getMessageRequestHandle(),
                    (MessageResponse**)&currentRecv[0], &waitallFunctor);
	} else {
		msgapi->waitall(numReq, ev->getMessageRequestHandle(),
                    (MessageResponse**)&currentRecv[0], &waitallNoDelFunctor);
	}

	// Keep track of the current request handle, we will free this auto(magically).
	currentReq = ev->getMessageRequestHandle();

	accumulateTime = histoWait;
}

void EmberEngine::processGetTimeEvent(EmberGetTimeEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Get Time Event\n");
	ev->setTime((uint64_t) getCurrentSimTimeNano());
	issueNextEvent(0);
}

void EmberEngine::processIRecvEvent(EmberIRecvEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing an IRecv Event (%s)\n", ev->getPrintableString().c_str());

	switch(dataMode) {
	case NOBACKING:
		msgapi->irecv(NULL, ev->getMessageSize(),
			CHAR, (RankID) ev->getRecvFromRank(),
			ev->getTag(), ev->getCommunicator(),
			ev->getMessageRequestHandle(), &irecvFunctor);
		break;

	case BACKZEROS:
		assert( emptyBufferSize >= ev->getMessageSize() );
		msgapi->irecv((Addr) emptyBuffer, ev->getMessageSize(),
			CHAR, (RankID) ev->getRecvFromRank(),
			ev->getTag(), ev->getCommunicator(),
			ev->getMessageRequestHandle(), &irecvFunctor);
		break;
	}
	accumulateTime = histoIRecv;
}

void EmberEngine::processISendEvent(EmberISendEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing an ISend Event (%s)\n", ev->getPrintableString().c_str());

	switch(dataMode) {
	case NOBACKING:
		msgapi->isend(NULL, ev->getMessageSize(),
			CHAR, (RankID) ev->getSendToRank(),
			ev->getTag(), ev->getCommunicator(),
			ev->getMessageRequestHandle(), &isendFunctor);
		break;

	case BACKZEROS:
	        assert( emptyBufferSize >= ev->getMessageSize() );
		msgapi->isend((Addr) emptyBuffer, ev->getMessageSize(),
			CHAR, (RankID) ev->getSendToRank(),
			ev->getTag(), ev->getCommunicator(),
			ev->getMessageRequestHandle(), &isendFunctor);
		break;
	}
	accumulateTime = histoISend;
}

void EmberEngine::processRecvEvent(EmberRecvEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Recv Event (%s)\n", ev->getPrintableString().c_str());
	currentRecv.resize(1);

	switch(dataMode) {
	case NOBACKING:
		msgapi->recv(NULL, ev->getMessageSize(),
			CHAR, (RankID) ev->getRecvFromRank(),
			ev->getTag(), ev->getCommunicator(),
			&currentRecv[0], &recvFunctor);
		break;

	case BACKZEROS:
	        assert( emptyBufferSize >= ev->getMessageSize() );
		msgapi->recv((Addr) emptyBuffer, ev->getMessageSize(),
			CHAR, (RankID) ev->getRecvFromRank(),
			ev->getTag(), ev->getCommunicator(),
			&currentRecv[0], &recvFunctor);
		break;
	}
	accumulateTime = histoRecv;
}

void EmberEngine::processFinalizeEvent(EmberFinalizeEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Finalize Event\n");
	msgapi->fini(&finalizeFunctor);

	accumulateTime = histoFinalize;
}

void EmberEngine::processComputeEvent(EmberComputeEvent* ev) {
	output->verbose(CALL_INFO, 2, 0, "Processing a Compute Event (%s)\n", ev->getPrintableString().c_str());

	// Issue the next event with a delay (essentially the time we computed something)
	const uint64_t noiseAdjustedTime = (computeNoiseDistrib->getNextDouble() * ev->getNanoSecondDelay());
	output->verbose(CALL_INFO, 2, 0, "Adjust time by noise distribution to give: %" PRIu64 "ns\n", noiseAdjustedTime);
	issueNextEvent(noiseAdjustedTime);
	accumulateTime = histoCompute;
}

void EmberEngine::completedInit(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Init, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedFinalize(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Finalize, result = %d\n", val);

	// Tell the simulator core we are finished and do not need any further
	// processing to continue
	primaryComponentOKToEndSim();

	continueProcessing = false;
	issueNextEvent(0);
}

void EmberEngine::completedBarrier(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Barrier, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedWait(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Wait, result = %d\n", val);

	// Delete the previous MessageRequest
	delete *currentReq;

	issueNextEvent(0);
}

void EmberEngine::completedWaitallWithoutDelete(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Wait, result = %d (no delete of message request)\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedWaitall(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Wait, result = %d\n", val);

	// Delete the previous MessageRequest
	delete currentReq;

	issueNextEvent(0);
}

void EmberEngine::completedWaitWithoutDelete(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Wait, result = %d (no delete of message request)\n", val);
	issueNextEvent(0);
}


void EmberEngine::completedIRecv(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed IRecv, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedISend(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed ISend, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedSend(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Send, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedRecv(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Recv, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedAllreduce(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Allreduce, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::completedReduce(int val) {
	output->verbose(CALL_INFO, 2, 0, "Completed Reduce, result = %d\n", val);
	issueNextEvent(0);
}

void EmberEngine::refillQueue() {
	generator->generate(output, generationPhase++, &evQueue);
	eventCount = evQueue.size();
}

void EmberEngine::checkQueue() {
	// Check if we have run out of events, if yes then
	// we must refill the queue to continue
	if(0 == eventCount) {
		refillQueue();
	}
}

void EmberEngine::issueNextEvent(uint32_t nanoDelay) {
	if(continueProcessing) {
		// This issues the next event on the self link
		// Check the queue, may need refilling
		checkQueue();

		if(0 == eventCount) {
			// We are completed so we can now exit
		} else {
			EmberEvent* nextEv = evQueue.front();
			evQueue.pop();
			eventCount--;

			// issue the next event to the engine for deliver later
			selfEventLink->send(nanoDelay, nanoTimeConverter, nextEv);
		}
	}
}

void EmberEngine::handleEvent(Event* ev) {
	// Accumulate the time processing the last event into a counter
	// we track these by event type
	const uint64_t sim_time_now = (uint64_t) getCurrentSimTimeNano();
	accumulateTime->add( sim_time_now - nextEventStartTimeNanoSec );
	nextEventStartTimeNanoSec = sim_time_now;

	// Cast out the event we are processing and then hand off to whatever
	// handlers we have created
	EmberEvent* eEv = static_cast<EmberEvent*>(ev);

	// Process the next event
	switch(eEv->getEventType()) {
	case SEND:
		processSendEvent( (EmberSendEvent*) eEv );
		break;
	case RECV:
		processRecvEvent( (EmberRecvEvent*) eEv );
		break;
	case IRECV:
		processIRecvEvent( (EmberIRecvEvent*) eEv);
		break;
	case ISEND:
		processISendEvent( (EmberISendEvent*) eEv);
		break;
	case WAIT:
		processWaitEvent( (EmberWaitEvent*) eEv);
		break;
	case WAITALL:
		processWaitallEvent( (EmberWaitallEvent*) eEv);
		break;
	case ALLREDUCE:
		processAllreduceEvent( (EmberAllreduceEvent*) eEv);
		break;
	case REDUCE:
		processReduceEvent( (EmberReduceEvent*) eEv);
		break;
	case BARRIER:
		processBarrierEvent( (EmberBarrierEvent*) eEv );
		break;
	case FINALIZE:
		processFinalizeEvent(dynamic_cast<EmberFinalizeEvent*>(eEv));
		break;
	case INIT:
		processInitEvent(dynamic_cast<EmberInitEvent*>(eEv));
		break;
	case COMPUTE:
		processComputeEvent(dynamic_cast<EmberComputeEvent*>(eEv));
		break;
	case GETTIME:
		processGetTimeEvent(dynamic_cast<EmberGetTimeEvent*>(eEv));
		break;
	case START:
		processStartEvent(dynamic_cast<EmberStartEvent*>(eEv));
		break;
	case STOP:
		processStopEvent(dynamic_cast<EmberStopEvent*>(eEv));
		break;
	default:

		break;
	}
	// Delete the current event
	delete ev;

}

EmberEngine::EmberEngine() :
    Component(-1)
{
    // for serialization only
}

BOOST_CLASS_EXPORT(EmberEngine)
