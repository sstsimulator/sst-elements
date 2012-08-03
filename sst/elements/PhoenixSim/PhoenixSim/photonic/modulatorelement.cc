/*
 * modulatorelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "modulatorelement.h"

ModulatorElement::ModulatorElement()
{
}

ModulatorElement::~ModulatorElement()
{
}

void ModulatorElement::initialize()
{
	InitializeBasicElement();
	InitializeRingElement();
	InitializeModulatorElement();
	Setup();
}

void ModulatorElement::InitializeModulatorElement()
{
	NoiseBandwidth = par("O_frequency_data");
	ModulatorExtinctionRatio = par("ModulatorExtinctionRatio");
	LaserRelativeIntensityNoise = par("LaserRelativeIntensityNoise");

	currPacketStat = NULL;
	DynamicModulatorEnergyStat = Statistics::registerStat(GetName(), StatObject::ENERGY_EVENT, "photonic");

#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==1
	if(hasPar("SizeWidth") && hasPar("SizeHeight") && hasPar("PositionLeftX") && hasPar("PositionBottomY"))
	{
		SizeWidth = par("SizeWidth");
        SizeHeight = par("SizeHeight");
		PositionLeftX = par("PositionLeftX");
        PositionBottomY = par("PositionBottomY");
        ThermalModel::registerThermalObject(groupLabel.c_str(), getId(), SizeWidth, SizeHeight, PositionLeftX, PositionBottomY);
	}
	else
	{
		throw cRuntimeError("HotSpot Simulator requires definition of SizeWidth, SizeHeight, PositionLeftX, and PositionBottomY");
	}
#endif
#endif
}

void ModulatorElement::handleMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	if(CheckAndHandleModulatorMessage(msg))
	{
		return;
	}
	else if(!CheckAndHandlePhotonicMessage(msg))
	{
		delete msg;
		throw cRuntimeError("Error 004: cMessage arrived on the invalid gateId, %d", arrivalGate);
	}
}

bool ModulatorElement::CheckAndHandleModulatorMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	int arrivalPortId = -1;
	PhotonicMessage *pmsg = (PhotonicMessage*)msg;
	for (int i = 0; i < numOfPorts; i++)
	{
		if (arrivalGate == gateIdIn[i])
		{
			arrivalPortId = i;
			break;
		}
	}
	//PacketStat *msgPktStat = (PacketStat*)(pmsg->getPacketStat());
	//int destPort = AccessRoutingTable(arrivalPortId, msgPktStat);
	if(arrivalPortId == portIdModulator)
	{
		HandleModulatorMessage(pmsg, arrivalPortId);
		HandlePhotonicMessage(pmsg, arrivalPortId);
		return true;
	}
	return false;
}

void ModulatorElement::HandleModulatorMessage(PhotonicMessage *pmsg, int indexIn)
{
	PacketStat *msgPktStat = (PacketStat*)(pmsg->getPacketStat());
	int msgType = pmsg->getMsgType();
	double energyDissipatedByMessageModulation;
	if(msgPktStat != NULL)
	{
		throw cRuntimeError("Message entered modulator port, but it already has PacketStat instantiated.");
	}
	if (msgType == TXstart)
	{
		currPacketStat = new PacketStat();
		pmsg->setPacketStat(long(currPacketStat));
		PacketStat *ps = currPacketStat;
		ps->signalPower = GetModulatorLevel();
		//ps->crossTalkPower = 0;
		ps->propagationLoss = 0;
		//CT calculation below accounts for RIN and modulation
		ps->laserNoisePower = dBtoLinear(ps->signalPower)/(pow(1-dBtoLinear(-ModulatorExtinctionRatio),2)/(2*NoiseBandwidth*dBtoLinear(LaserRelativeIntensityNoise)));
		ps->messageNoisePower = 0;
		ps->bendingLoss = 0;
		ps->passByRingLoss = 0;
		ps->dropIntoRingLoss = 0;
		ps->wavelength = GetModulatorWavelength();
		pmsg->setPacketStat(long(currPacketStat));
	}
	else if (msgType == TXstop)
	{
		pmsg->setPacketStat(long(currPacketStat));
		currPacketStat = NULL;
		energyDissipatedByMessageModulation = AddModulationEnergyDissipation(simTime().dbl() - lastMessageTimestamp[indexIn][((PacketStat*)pmsg->getPacketStat())->wavelength].dbl());
#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==1
		ThermalModel::addThermalObjectEnergy(groupLabel.c_str(), getId(), energyDissipatedByMessageModulation);
#endif
#endif
	}
}

