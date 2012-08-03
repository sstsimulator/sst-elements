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

#ifndef NIF_PACKET_CREDIT_H_
#define NIF_PACKET_CREDIT_H_

#include "NIF.h"

class NIF_Packet_Credit : public NIF {
public:
	NIF_Packet_Credit();
	virtual ~NIF_Packet_Credit();


	cGate* packetPortIn;
	cGate* packetPortOut;

	cChannel* transChannelOut;

	cMessage* sendPacketMsg;

	virtual void selfMsgArrived(cMessage *msg);

	map<int, int> credits;
	int flitWidth;

	bool stalled;

	queue<ElectronicMessage*> unacked;

	queue<cPacket*> packetQ;
	map<cPacket*, int> packetVC;


	double clockPeriod_packet;

	virtual void init();

	virtual void finish();

    virtual void netPortMsgArrived(cMessage* msg);
    virtual void ackMsgArrived(BufferAckMsg* bmsg);

    virtual void processorMsgSent(ProcessorData* pdata);

    void ackMsg();
    int roundToFlit(int length);

    virtual void init_packet() = 0;
    virtual void packetPortMsgArrived(cMessage* msg) = 0;
    bool sendPacket(cPacket* msg, bool firstTime, int vc, simtime_t delay = 0, bool isAck = false);
};

#endif /* NIF_PACKET_CREDIT_H_ */
