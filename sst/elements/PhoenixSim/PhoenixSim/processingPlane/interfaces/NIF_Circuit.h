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

#ifndef NIF_CIRCUIT_H_
#define NIF_CIRCUIT_H_

#include "NIF_Packet_Credit.h"

class NIF_Circuit : public NIF_Packet_Credit {
public:
	NIF_Circuit();
	virtual ~NIF_Circuit();

	cGate* dataPortIn;
	cGate* dataPortOut;

	cChannel* dataChannelOut;

	int maxDataWidth;

	double clockPeriod_data;

	int retry;

	bool pathGood;

	simtime_t setupSent;
	simtime_t blockTime;


	StatObject* SO_lat_blocking;
	StatObject* SO_lat_setup;

	int allowedTx;
	int allowedWL;

	bool waitOnDataArrive;

	ElectronicMessage* unackedSetupMsg;

	ProcessorData *currData;

	virtual simtime_t getRetryTime();

	virtual void init_packet();

	virtual void netPortMsgArrived(cMessage* msg);


	queue<ElectronicMessage*> elMsgs;


	bool sendSetup(ProcessorData* pdata, simtime_t delay) ;
	virtual void packetPortMsgArrived(cMessage* cmsg);


	virtual void init_circuit() = 0;
	virtual void dataMsgArrived(cMessage* cmsg) = 0;

	virtual void requestMsgArrived(RouterCntrlMsg* rmsg);
	virtual void sendDataToProcessor(ElectronicMessage* emsg);
};

#endif /* NIF_CIRCUIT_H_ */
