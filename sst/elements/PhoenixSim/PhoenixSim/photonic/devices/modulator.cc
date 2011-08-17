#include "modulator.h"

Define_Module(Modulator);


Modulator::Modulator()
{
	currPacketStat = NULL;
}

Modulator::~Modulator()
{
}

void Modulator::Setup()
{
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	portIdModulator = 0;

	gateIdIn[0] = gate("driver$i")->getId();
	gateIdIn[1] = gate("photonicOut$i")->getId();
	gateIdIn[2] = gate("photonicIn$i")->getId();

	gateIdOut[0] = gate("driver$o")->getId();
	gateIdOut[1] = gate("photonicOut$o")->getId();
	gateIdOut[2] = gate("photonicIn$o")->getId();

	portType[0] = in;
	portType[1] = inout;
	portType[2] = inout;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");
	Latency_Modulator = par("Latency_Modulator");

	useWDM = par("useWDM");
	numOfWavelengths = par("numOfWavelengthChannels");
	currSourcePower = par("LaserPower").doubleValue() - LineartodB(numOfWavelengths);
	currSourceWavelength = filterBaseWavelength;


	numOfCircuits = par("numOfCircuits");

	if(numOfWavelengths%numOfCircuits != 0)
	{
		throw cRuntimeError("please pick numOfCircuits so that it is a factor of numOfWavelengths");
	}

	photonicBitRate = par("O_frequency_data");
	EperBit = (double)par("Modulator_EperBit") + (double)par("Modulator_Driver_EperBit");
	Estatic = par("Modulator_Static");



	//E_thermal->track(thermalTemperatureDeviation*thermalRingTuningPower*numOfRings*(useWDM?0:numOfWavelengths-1));

	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double Modulator::GetLatency(int indexIn, int indexOut)
{
	return Latency_Modulator;
}

double Modulator::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	double IL = 0;
	if(indexIn == indexOut || (indexIn == 0 && indexOut == 2))
	{
		IL += MAX_INSERTION_LOSS;
	}
	if(indexIn == 1 || indexIn == 2)
	{
		IL += (PropagationLoss * 13) * (useWDM?1:numOfWavelengths) / 2;
	}

	if(indexOut == 1 || indexOut == 2)
	{
		IL += (PropagationLoss * 13) * (useWDM?1:numOfWavelengths) / 2;
	}
	return IL;
}

double Modulator::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	if((indexIn == 1 || indexIn == 2) && (indexOut == 1 || indexOut == 2))
	{
		return PassByRingLoss * (useWDM?1:numOfWavelengths);
	}
	else
	{
		return 0;
	}
}


double Modulator::GetModulatorLevel()
{

	return currSourcePower;
}

double Modulator::GetModulatorWavelength()
{
	double vf;
	if(useRingModel)
	{
		vf = PhyLayer::SpeedOfLight("SiGI")/ringLength;
		return vf * 93;

	}
	return currSourceWavelength;
}


void Modulator::HandleModulatorMessage(PhotonicMessage *pmsg, int indexIn)
{
	/*
	PacketStat *ps = (PacketStat*)(pmsg->getPacketStat());
	ps->signalPower = GetModulatorLevel();
	ps->crossTalkPower = 0;
	//CT calculation below accounts for RIN and modulation
	ps->crossTalkPower = dBtoLinear(ps->signalPower)/(pow(1-dBtoLinear(-ModulatorExtinctionRatio),2)/(2*NoiseBandwidth*dBtoLinear(LaserRelativeIntensityNoise)));
	ps->wavelength = GetModulatorWavelength();
	ps->passByRingLoss = 0;
	ps->dropIntoRingLoss = 0;
	ps->numWDMChannels = numOfWavelengths;
	*/

	ModulatorElement::HandleModulatorMessage(pmsg,indexIn);
	int msgType = pmsg->getMsgType();
	PacketStat *ps = (PacketStat*)(pmsg->getPacketStat());
	if(msgType == TXstart)
	{
		ps->numWDMChannels = numOfWavelengths/numOfCircuits;
	}
}

double Modulator::AddModulationEnergyDissipation(double time)
{
	double energy = (time*photonicBitRate*EperBit + time*0.5*Estatic)*(useWDM?1:numOfWavelengths);
	DynamicModulatorEnergyStat->track(energy);
	return energy;
}

void Modulator::SetRoutingTable()
{
	routingTable[0] = 1;
	routingTable[1] = 0;
	routingTable[2] = -1;
}

void Modulator::SetOffResonanceRoutingTable()
{
	routingTableOffResonance[0] = -1;
	routingTableOffResonance[1] = 2;
	routingTableOffResonance[2] = 1;
}
