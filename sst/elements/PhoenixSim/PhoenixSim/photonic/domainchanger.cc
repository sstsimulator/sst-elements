/*
 * domainchanger.cc
 *
 *  Created on: May 9, 2009
 *      Author: johnnie
 */

#include "domainchanger.h"

Define_Module(DomainChanger);


DomainChanger::DomainChanger()
{
	inEmsg = false;
	inAmsg = false;
}

DomainChanger::~DomainChanger()
{
	cancelAndDelete(etimer);
	cancelAndDelete(ptimer);
}

void DomainChanger::finish()
{
}

void DomainChanger::initialize()
{
	etimer = new cMessage;
	ptimer = new cMessage;

	numOfWavelengths = par("numOfWavelengths");
	photonicBitRate = par("O_frequency_data");
	useWDM = par("useWDM");

	gateId_photonicConnectionInBase = gateBaseId("photonicConnectionIn$i");
	gateId_electronicConnectionOutIn = gateBaseId("photonicConnectionOut$i");
	gateId_photonicConnectionOutBase = gateBaseId("photonicConnectionOut$o");
	gateId_electronicConnectionIn = gate("electronicConnectionIn")->getId();
	gateId_electronicConnectionOut = gate("electronicConnectionOut")->getId();
}

void DomainChanger::handleMessage(cMessage *msg)
{
	int gateId = msg->getArrivalGateId();
	//std::cout<<gateId_photonicConnectionInBase<<" "<<gateId_photonicConnectionOutBase<<" "<<gateId<<endl;
	// Should handle conversion between optic domain and electric
	if(gateId == gateId_electronicConnectionIn)
	{
		msg->setKind(1);
		buffer.insert(msg);
		checkAndIssueAckOrElectronicMessage();
	}
	else if(gateId >= gateId_photonicConnectionInBase && gateId < gateId_photonicConnectionInBase + numOfWavelengths)
	{
		handlePhotonicMessage((PhotonicMessage*)msg);
	}
	else if(gateId >= gateId_electronicConnectionOutIn && gateId < gateId_electronicConnectionOutIn + numOfWavelengths)
	{
		throw cRuntimeError("Photonic message was received through a modulator gate.");
	}
	else if(msg == etimer)
	{
		handleElectronicTimer();
	}
	else if(msg == ptimer)
	{
		handleBufferOETimer();
	}
	else
	{
		throw cRuntimeError("Domain Changer received a message on an unknown gate.");
	}


}

void DomainChanger::checkAndIssueAckOrElectronicMessage()
{
	if(!inAmsg && !inEmsg && !buffer.empty())
	{
		handleElectronicMessage();
	}
}

void DomainChanger::handlePhotonicMessage(PhotonicMessage *pmsg)
{
	if(pmsg->getMsgType() == TXstart)
	{
		if(inPmsg)
		{
			cRuntimeError("Already got a TXstart");
		}
		inPmsg = true;
	}
	else if(pmsg->getMsgType() == TXstop && pmsg->getEncapsulatedPacket() != NULL)
	{

		if(!inPmsg)
		{
			cRuntimeError("Got TXstop without a preceeding TXstart");
		}
		inPmsg = false;
		cMessage *msg = (cMessage*)(pmsg->decapsulate());

		switch(msg->getKind())
		{
			case 1:

				bufferOE.insert(msg);
				checkAndIssueBufferOE();
				//ElectronicMessage *emsg;
				//emsg = (ElectronicMessage*)msg;
				//send(emsg,gateId_electronicConnectionOut);
				break;
		}
	}
	else
	{
		//throw cRuntimeError("uh oh!");
	}
	delete pmsg;

}

void DomainChanger::checkAndIssueBufferOE()
{
	simtime_t scheduleTime;

	if(gate(gateId_electronicConnectionOut)->findTransmissionChannel() != NULL)
	{
		scheduleTime = gate(gateId_electronicConnectionOut)->getTransmissionChannel()->getTransmissionFinishTime();
	}
	else
	{
		scheduleTime = simTime();
	}
	if(!bufferOE.empty())
	{
		if(!ptimer->isScheduled())
		{
			if(scheduleTime <= simTime())
			{
				scheduleAt(simTime(),ptimer);
			}
			else
			{
				scheduleAt(scheduleTime,ptimer);
			}
		}
	}
}

void DomainChanger::handleBufferOETimer()
{
	ElectronicMessage *emsg;
	emsg = (ElectronicMessage*)(bufferOE.pop());
	send(emsg,gateId_electronicConnectionOut);
	checkAndIssueBufferOE();
}

void DomainChanger::handleElectronicMessage()
{
	if(((cMessage*)(buffer.front()))->getKind() == 1)
	{
		inEmsg = true;
	}
	else
	{
		inAmsg = true;
	}
	PhotonicMessage *pmsg = new PhotonicMessage;
	pmsg->setMsgType(TXstart);
	for(int i = 0 ; i < (useWDM?numOfWavelengths:1) ; i++)
	{
		startTime = simTime();
		send(pmsg->dup(),gateId_photonicConnectionOutBase + i);
	}
	delete pmsg;

	scheduleAt(simTime() + (((cPacket*)(buffer.front()))->getBitLength())/(photonicBitRate*numOfWavelengths), etimer);
}


void DomainChanger::handleElectronicTimer()
{
	if(((cPacket*)(buffer.front()))->getKind() == 1)
	{
		inEmsg = false;
	}
	else
	{
		inAmsg = false;
	}
	PhotonicMessage *pmsg = new PhotonicMessage;
	PhotonicMessage *sendpmsg;
	//pmsg->setKind(0);
	pmsg->setMsgType(TXstop);
	//currEmsg = emsg;
	for(int i = 0 ; i < (useWDM?numOfWavelengths:1) ; i++)
	{
		sendpmsg = pmsg->dup();
		if(i + 1 == (useWDM?numOfWavelengths:1)) // if last message being transmitted
		{
			sendpmsg->encapsulate(((cPacket*)(buffer.pop())));
		}
		send(sendpmsg,gateId_photonicConnectionOutBase + i);
	}
	delete pmsg;
	checkAndIssueAckOrElectronicMessage();
}
