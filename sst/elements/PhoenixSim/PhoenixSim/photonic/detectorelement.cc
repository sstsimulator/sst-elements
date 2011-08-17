/*
 * detectorelement.cc
 *
 *  Created on: Oct 23, 2009
 *      Author: Johnnie Chan
 */

#include "detectorelement.h"

DetectorElement::DetectorElement()
{
}

DetectorElement::~DetectorElement()
{
}

void DetectorElement::initialize()
{
	InitializeBasicElement();
	InitializeRingElement();
	InitializeDetectorElement();
	Setup();
}

void DetectorElement::InitializeDetectorElement()
{
	photonicBitRate = par("O_frequency_data");
	DynamicDetectorEnergyStat = Statistics::registerStat(GetName(), StatObject::ENERGY_EVENT, "photonic");
	StaticDetectorEnergyStat = Statistics::registerStat(GetName()+" static", StatObject::ENERGY_STATIC, "photonic");
	StaticDetectorDummyEnergyStat = Statistics::registerStat(GetName()+" static", StatObject::ENERGY_EVENT, "photonic");
	IL_prop = Statistics::registerStat("Propagation", StatObject::MMA, "IL");
	IL_bend = Statistics::registerStat("Bending", StatObject::MMA, "IL");
	IL_cross = Statistics::registerStat("Crossing", StatObject::MMA, "IL");
	IL_pass = Statistics::registerStat("Pass By Ring", StatObject::MMA, "IL");
	IL_drop = Statistics::registerStat("Drop Into Ring", StatObject::MMA, "IL");
	IL_total = Statistics::registerStat("Total", StatObject::MMA, "IL");

	PHY_SNR_EL = Statistics::registerStat("Electrical SNR", StatObject::MMA, "PhyStats");
	PHY_SNR_OPT = Statistics::registerStat("Optical SNR", StatObject::MMA, "PhyStats");

	LaserNoisePowerStat = Statistics::registerStat("1 Laser Noise Power (mW)", StatObject::MMA, "PhyStats");
	InterMessageNoisePowerStat = Statistics::registerStat("2 Message Noise Power (mW)", StatObject::MMA, "PhyStats");
	IntraMessageNoisePowerStat = Statistics::registerStat("3 Cross Wavelength Noise Power (mW)", StatObject::MMA, "PhyStats");
	TotalNoisePowerStat = Statistics::registerStat("4 Total Optical Noise Power (mW)", StatObject::MMA, "PhyStats");
	TotalNoisePowerdBStat = Statistics::registerStat("7 Total Noise Power (dBm)", StatObject::MMA, "PhyStats");
	SignalPowerStat = Statistics::registerStat("8 Signal Power (dBm)", StatObject::MMA, "PhyStats");
	JohnsonNoisePowerStat = Statistics::registerStat("5 Johnson Noise Power", StatObject::MMA, "PhyStats");
	ShotNoisePowerStat = Statistics::registerStat("6 Shot Noise Power", StatObject::MMA, "PhyStats");

	temperature = par("ambientTemperature");


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

void DetectorElement::handleMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	if(CheckAndHandleDetectorMessage(msg))
	{
		//std::cout<<this->getFullName()<<" id:"<<msg->getId()<<endl;
		//std::cout<<" intermessage crosstalk ="<<((PacketStat*)((PhotonicMessage*)msg)->getPacketStat())->getTotalWavelengthNoisePower();
		//std::cout<<" intramessage crosstalk ="<<((PacketStat*)((PhotonicMessage*)msg)->getPacketStat())->messageNoisePower;
		//std::cout<<endl;
		return;
	}
	if(!CheckAndHandlePhotonicMessage(msg))
	{
		delete msg;
		throw cRuntimeError("Error 004: cMessage arrived on the invalid gateId, %d", arrivalGate);
	}

}

bool DetectorElement::CheckAndHandleDetectorMessage(cMessage *msg)
{
	int arrivalGate = msg->getArrivalGateId();
	int arrivalGateId = -1;
	PhotonicMessage *pmsg = (PhotonicMessage*)msg;
	for (int i = 0; i < numOfPorts; i++)
	{
		if (arrivalGate == gateIdIn[i])
		{
			arrivalGateId = i;
			break;
		}
	}
	PacketStat *msgPktStat = (PacketStat*)(pmsg->getPacketStat());
	int destPort = AccessRoutingTable(arrivalGateId, msgPktStat);
	if(destPort == portIdDetector && pmsg->getMsgType() == TXstop)
	{
		HandleDetectorMessage(pmsg, arrivalGateId, destPort);
		HandlePhotonicMessage(pmsg, arrivalGateId);
		return true;
	}
	return false;
}

void DetectorElement::HandleDetectorMessage(PhotonicMessage *pmsg, int indexIn, int indexOut)
{
	double responsivity = 1;

	PacketStat *stat = (PacketStat*) (pmsg->getPacketStat());
	double wavelength = stat->wavelength;
	double energyDissipatedByMessageDetection;



	// Note that O_NoisePower is in mW, while johnsonNoise and shotNoise are in W

	double O_NoisePower = stat->laserNoisePower + stat->messageNoisePower + stat->getTotalWavelengthNoisePower();

	double johnsonNoise = PhyLayer::ThermalNoiseVariance(temperature, photonicBitRate, 50);
	double shotNoise = PhyLayer::ShotNoiseVariance(1,1, photonicBitRate, 1);

	double osnr = (stat->signalPower) - LineartodB(O_NoisePower);

	double snr = (stat->signalPower + LineartodB(responsivity)) - LineartodB(O_NoisePower*responsivity + (johnsonNoise + shotNoise)*1e3);

	LaserNoisePowerStat->track(stat->laserNoisePower);
	InterMessageNoisePowerStat->track(stat->messageNoisePower);

	IntraMessageNoisePowerStat->track(stat->getTotalWavelengthNoisePower());
	TotalNoisePowerStat->track(O_NoisePower);
	TotalNoisePowerdBStat->track(LineartodB(O_NoisePower*responsivity + (johnsonNoise + shotNoise)*1e3));
	SignalPowerStat->track(stat->signalPower);

	JohnsonNoisePowerStat->track(johnsonNoise);
	ShotNoisePowerStat->track(shotNoise);


	PHY_SNR_EL->track(snr);
	PHY_SNR_OPT->track(osnr);

	IL_prop->track(stat->propagationLoss);
	IL_bend->track(stat->bendingLoss);
	IL_cross->track(stat->crossingLoss);
	IL_pass->track(stat->passByRingLoss);
	IL_drop->track(stat->dropIntoRingLoss);
	IL_total->track(stat->propagationLoss + stat->crossingLoss + stat->bendingLoss + stat->passByRingLoss + stat->dropIntoRingLoss);
	energyDissipatedByMessageDetection = AddDetectionEnergyDissipation(simTime().dbl() - lastMessageTimestamp[indexIn][wavelength].dbl());
#ifdef ENABLE_HOTSPOT
#if HOTSPOT_GRANULARITY==1
		//ThermalModel::addThermalObjectEnergy(groupLabel.c_str(), getId(), energyDissipatedByMessageDetection);
#endif
#endif
}

