//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef NIF_H_
#define NIF_H_

#include <omnetpp.h>
#include <queue>
#include "sim_includes.h"
#include "messages_m.h"
#include "statistics.h"

#include "NetworkAddress.h"

#include "Processor.h"

#include "VirtualChannelControl.h"

#define CONTROL_PACKET_SIZE 	32
#define ACK_LENGTH 		32
#define DATA_HEADER_SIZE	16
#define ELECTRONIC_MESSAGE_SIZE 32

using namespace std;

class NIF: public cSimpleModule {
public:
	NIF();
	virtual ~NIF();

protected:
	virtual void controller();

	virtual bool request() = 0;
	virtual bool prepare() = 0;
	virtual simtime_t send() = 0;
	virtual void complete() = 0;

	virtual void init() = 0;

	virtual void netPortMsgArrived(cMessage* cmsg) = 0;
	virtual void processorMsgSent(ProcessorData* pdata) = 0;

	virtual void procMsgArrived(ProcessorData* pdata);
	virtual void requestMsgArrived(RouterCntrlMsg* rmsg);
	virtual void sendDataToProcessor(ProcessorData* pdata);

	virtual void selfMsgArrived(cMessage* cmsg) { };

	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);

	void printProcData(ProcessorData* pdata);

	static int procDataCount;


	static int numOfNIFs;
	static int nifsDone;
	bool procIsDone;
	int numProcsDone;


	//stats
	StatObject* SO_bw;
	StatObject* SO_qTime;
	StatObject* SO_lat_conc;

	//network side
	map<int, cGate*> pIn;
	map<int, cGate*> pOut;

	//processor side
	cGate* procReqIn;
	cGate* procReqOut;
	cGate* fromProc;
	cGate* toProc;


	int concentration;
	int numNetPorts;

	bool sentRequest;


	cMessage* nextMsg;
	cMessage* completeMsg;

	list<ProcessorData*> waiting;
	queue<ProcessorData*> toProcQ;



	//static int pathsEstablished;
};

#endif /* NIF_H_ */
