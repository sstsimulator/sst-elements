// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_OTF2_READER
#define _H_EMBER_OTF2_READER

#include <unordered_map>

#include "mpi/embermpigen.h"
#include "otf2/otf2.h"

namespace SST {
namespace Ember {

class EmberOTF2Generator : public EmberMessagePassingGenerator {

public:
	EmberOTF2Generator(SST::ComponentId_t, Params& params);
	~EmberOTF2Generator();
    	bool generate( std::queue<EmberEvent*>& evQ );

	SST_ELI_REGISTER_SUBCOMPONENT(
        	EmberOTF2Generator,
        	"ember",
         	"OTF2Motif",
        	SST_ELI_ELEMENT_VERSION(1,0,0),
        	"Replays an OTF2-based trace",
        	SST::Ember::EmberGenerator
    	)

	SST_ELI_DOCUMENT_PARAMS(
        	{   "arg.tracePrefix",       "Sets the location of the trace" },
        	{   "arg.addCompute",        "Add compute time to try to match trace timestamps",  "false" }
	)

	SST_ELI_DOCUMENT_STATISTICS(
	        { "time-Init", "Time spent in Init event",          "ns",  0},
	        { "time-Finalize", "Time spent in Finalize event",  "ns", 0},
	        { "time-Rank", "Time spent in Rank event",          "ns", 0},
	        { "time-Size", "Time spent in Size event",          "ns", 0},
	        { "time-Send", "Time spent in Recv event",          "ns", 0},
	        { "time-Recv", "Time spent in Recv event",          "ns", 0},
	        { "time-Irecv", "Time spent in Irecv event",        "ns", 0},
	        { "time-Isend", "Time spent in Isend event",        "ns", 0},
	        { "time-Wait", "Time spent in Wait event",          "ns", 0},
	        { "time-Waitall", "Time spent in Waitall event",    "ns", 0},
	        { "time-Waitany", "Time spent in Waitany event",    "ns", 0},
	        { "time-Compute", "Time spent in Compute event",    "ns", 0},
	        { "time-Barrier", "Time spent in Barrier event",    "ns", 0},
	        { "time-Alltoallv", "Time spent in Alltoallv event", "ns", 0},
	        { "time-Alltoall", "Time spent in Alltoall event",  "ns", 0},
	        { "time-Allreduce", "Time spent in Allreduce event", "ns", 0},
	        { "time-Reduce", "Time spent in Reduce event",      "ns", 0},
	        { "time-Bcast", "Time spent in Bcast event",        "ns", 0},
	        { "time-Gettime", "Time spent in Gettime event",    "ns", 0},
        	{ "time-Commsplit", "Time spent in Commsplit event", "ns", 0},
	        { "time-Commcreate", "Time spent in Commcreate event", "ns", 0},
    	)

	void setTimerResolution(uint64_t timerResolution) {
		m_timerResolution = timerResolution;
	}

	uint64_t getTimerResolution() {
		return m_timerResolution;
	}

	void setCurrentTime( const OTF2_TimeStamp t, bool addCompute ) {
		verbose( CALL_INFO, 4, 0, "Setting current timestamp to %" PRIu64 "\n", t );

		if (addCompute && m_addCompute) {
			compute(*eventQ, (t-currentTime)*1e9/m_timerResolution);
		}
		currentTime = t;
	}

	OTF2_TimeStamp getCurrentTime() {
		return currentTime;
	}

	std::queue<EmberEvent*>* getEventQueue() {
		return eventQ;
	}

	int getSize() {
		return m_size;
	}

	void set_inMPI(bool m) {
		m_inMPI = m;
	}

	bool get_inMPI() {
		return m_inMPI;
	}

    void compute(Queue& q, uint64_t time);
    void send(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group); 
    void isend(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID dest, uint32_t tag, Communicator group, MessageRequest* req);
    void recv(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag, Communicator group, MessageResponse* resp );
    void irecv(Queue& q, const Hermes::MemAddr& payload, uint32_t count, PayloadDataType dtype, RankID src, uint32_t tag, Communicator group,MessageRequest* req );
    void wait(Queue& q, MessageRequest* req, MessageResponse* resp);
    void barrier(Queue& q, Communicator comm);
    void bcast(Queue& q, const Hermes::MemAddr& mydata, uint32_t count, PayloadDataType dtype, int root, Communicator group);
    void allreduce( Queue& q, const Hermes::MemAddr& mydata, const Hermes::MemAddr& result, uint32_t count, PayloadDataType dtype, ReductionOperation op, Communicator group );    
    void reduce(Queue& q, const Hermes::MemAddr& mydata, const Hermes::MemAddr& result, uint32_t count, PayloadDataType dtype, ReductionOperation op, int root,Communicator group );
    void scatter(Queue& q, const Hermes::MemAddr& mydata, uint32_t count_send, PayloadDataType dtype_send, const Hermes::MemAddr& result, uint32_t count_recv, PayloadDataType dtype_recv, int root, Communicator group);
    void allgather(Queue& q, const Hermes::MemAddr& mydata, uint32_t count_send, PayloadDataType dtype_send, const Hermes::MemAddr& result, uint32_t count_recv, PayloadDataType dtype_recv, Communicator group);
    void alltoall(Queue& q, const Hermes::MemAddr& mydata, uint32_t count_send, PayloadDataType dtype_send, const Hermes::MemAddr& result, uint32_t count_recv, PayloadDataType dtype_recv, Communicator group);

    void setEventQueue( std::queue<EmberEvent*>* newQ ) {
		eventQ = newQ;
	}

	std::unordered_map<uint64_t, MessageRequest*>& getRequestMap() {
		return requestMap;
	}

	void printRequestMap() {
		verbose(CALL_INFO, 4, 0, "Request Map on Rank: %" PRIu32 " contains: %" PRIu64 " entries.\n",
			static_cast<uint32_t>(rank()), static_cast<uint64_t>(requestMap.size()) );

		for( auto reqItr = requestMap.begin() ; reqItr != requestMap.end(); reqItr++ ) {
			verbose(CALL_INFO, 4, 0, "Request: %20" PRIu64 "\n", reqItr->first );
		}
	}
 
	PayloadDataType extractDataTypeFromAttributeList( OTF2_AttributeList* attr ) {
		uint32_t attributeCount = OTF2_AttributeList_GetNumberOfElements(attr);
		verbose( CALL_INFO, 16, 0, "Attribute list on rank: %" PRIu32 " contains %" PRIu32 " attributes\n",
			static_cast<uint32_t>(rank()), attributeCount );

		return CHAR;
	}

private:
	OTF2_DefReader* traceLocalDefReader;
	OTF2_GlobalDefReader* traceGlobalDefReader;
	OTF2_GlobalDefReaderCallbacks* traceGlobalDefCallbacks;
	OTF2_GlobalEvtReader* traceGlobalEvtReader;
	OTF2_GlobalEvtReaderCallbacks* traceGlobalEvtCallbacks;
	OTF2_DefReaderCallbacks* traceLocalCallbacks;
	OTF2_EvtReaderCallbacks* traceLocalEvtCallbacks;
     	OTF2_Reader* traceReader;
	OTF2_EvtReader* traceEvtReader;

	OTF2_TimeStamp currentTime;

	std::vector<uint64_t> traceLocations;
	uint64_t traceLocationCount;
	uint64_t currentLocation;

	bool traceOpenedDefFiles;

	std::queue<EmberEvent*>* eventQ;
	std::unordered_map<uint64_t, MessageRequest*> requestMap;

	uint64_t m_size;
	uint64_t m_timerResolution;
	bool m_inMPI;
	bool m_addCompute;
};

}
}

#endif
