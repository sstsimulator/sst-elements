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

#include "serializer.h"

Define_Module(Serializer)
;
Define_Module(Parallelizer)
;

Serializer::Serializer() {

}

Serializer::~Serializer() {
	// TODO Auto-generated destructor stub
}

void Serializer::initialize() {
	// TODO Auto-generated constructor stub
	currMsgId = -1;
	numPacketsLeftStart = 0;
	numPacketsLeftStop = 0;

	numWiresOut = par("numWiresOut");
	numWiresIn = par("numWiresIn");

	numGatesIn = gateSize("in");
	numGatesOut = gateSize("out");

	rateIn = par("rateIn");
	rateOut = par("rateOut");

	if (numWiresIn >= 1 && rateIn != rateOut) {
#ifdef ENABLE_ORION

		ORION_Tech_Config *conf = Statistics::DEFAULT_ORION_CONFIG_DATA;

		power = new ORION_FF();
		power->init(NEG_DFF, 0, conf);

		string statName = "Deserializer";
		P_static = Statistics::registerStat(statName,
				StatObject::ENERGY_STATIC, "electronic");
		E_dynamic = Statistics::registerStat(statName,
				StatObject::ENERGY_EVENT, "electronic");

		P_static->track(numWiresOut * power->report_static_power());

		StatObject *E_count = Statistics::registerStat("Deserializer",
				StatObject::COUNT, "electronic");

#endif

	}
}

void Serializer::handleMessage(cMessage *msg)
{
	// this is probably safe for photonic messages that have TXstart and TXstop, since only TXstop should have a bitlength.
	if (numWiresIn >= 1 && rateIn != rateOut)
	{
#ifdef ENABLE_ORION
		cPacket *pack = (cPacket*) msg;
		double perbit = (power->e_clock + power->e_switch * 0.5
				+ power->e_keep_0 * 0.25 + power->e_keep_1 * 0.25);
		double e = perbit * pack->getBitLength() / numGatesIn;

		E_dynamic->track(e);
#endif
	}
	/*
	if (useWDM) {

		if (pmsg->getId() != currMsgId) {
			if (numPacketsLeftStart == 0 && numPacketsLeftStop == 0) {
				currMsgId = pmsg->getId();
				numPacketsLeftStart
						= ((PacketStat*) (pmsg->getPacketStat()))->numWDMChannels;
				numPacketsLeftStop = numPacketsLeftStart;
			} else {
				throw cRuntimeError("didn't finish re-serializing msg.");
			}
		}
		if (pmsg->getMsgType() == TXstart) {

			numPacketsLeftStart--;
			if (numPacketsLeftStart == 0) {
				send(msg, "out");
			} else {
				delete msg;
			}
		} else if (pmsg->getMsgType() == TXstop) {

			numPacketsLeftStop--;

			if (numPacketsLeftStop == 0) {
				if (numPacketsLeftStart == 0) {
					currMsgId = -1;
				}
				send(msg, "out");
			} else {
				delete msg;
			}
		}

	} else {
		send(msg, "out");
	}
	*/

	//just pass first message
	if(msg->getArrivalGateId() == gateBaseId("in"))
	{
		send(msg, "out");
	}
	else
	{
		delete msg;
	}
}

Parallelizer::Parallelizer() {

}

Parallelizer::~Parallelizer() {
	// TODO Auto-generated destructor stub
}

void Parallelizer::initialize() {
	// TODO Auto-generated constructor stub
	currMsgId = -1;

	numWiresOut = par("numWiresOut");
	numWiresIn = par("numWiresIn");

	numGatesIn = gateSize("in");
	numGatesOut = gateSize("out");

	rateIn = par("rateIn");
	rateOut = par("rateOut");

	lockingClockCycles = 0;

	// JC: I'm not sure what the point of checking numWiresIn >= 1 is...
	if (numWiresIn >= 1 && rateIn != rateOut) {
#ifdef ENABLE_ORION

		ORION_Tech_Config *conf = Statistics::DEFAULT_ORION_CONFIG_DATA;

		power = new ORION_FF();
		power->init(NEG_DFF, 0, conf);

		string statName = "Serializer";
		P_static = Statistics::registerStat(statName,
				StatObject::ENERGY_STATIC, "electronic");
		E_dynamic = Statistics::registerStat(statName,
				StatObject::ENERGY_EVENT, "electronic");

		P_static->track(numWiresIn * power->report_static_power());

		StatObject *E_count = Statistics::registerStat("Serializer",
				StatObject::COUNT, "electronic");

#endif

	}
}

void Parallelizer::handleMessage(cMessage *msg) {

	simtime_t delay = lockingClockCycles / rateOut;

	if (numWiresIn >= 1 && rateIn != rateOut) {
#ifdef ENABLE_ORION
		cPacket *pack = (cPacket*) msg;
		double perbit = (power->e_clock + power->e_switch * 0.5
				+ power->e_keep_0 * 0.25 + power->e_keep_1 * 0.25);
		double e = perbit * pack->getBitLength();
		E_dynamic->track(e);
#endif
	}

	for (int i = 0; i < numGatesOut - 1; i++) {
		cMessage *pmsg;
		pmsg = msg->dup();
		//pmsg->setPacketStat((long)(new PacketStat((PacketStat*)(orig_pmsg->getPacketStat()))));
		sendDelayed(pmsg, delay, "out", i);

	}

	sendDelayed(msg, delay, "out", numGatesOut - 1);
}

