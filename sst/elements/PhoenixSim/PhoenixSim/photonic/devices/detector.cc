#include "detector.h"

Define_Module(Detector);

Detector::Detector() {
}

Detector::~Detector() {
}

void Detector::Setup() {
	debug(getFullPath(), "INIT: start", UNIT_INIT);

	gateIdIn[0] = gate("photonicIn$i")->getId();
	gateIdIn[1] = gate("electronicOut$i")->getId();
	gateIdIn[2] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicIn$o")->getId();
	gateIdOut[1] = gate("electronicOut$o")->getId();
	gateIdOut[2] = gate("photonicOut$o")->getId();

	portIdDetector = 1;

	portType[0] = inout;
	portType[1] = out;
	portType[2] = inout;

	PropagationLoss = par("PropagationLoss");
	PassByRingLoss = par("PassByRingLoss");
	PassThroughRingLoss = par("PassThroughRingLoss");
	CrossingLoss = par("CrossingLoss");
	Latency_Detector = par("Latency_Detector");

    RingThrough_ER_DetectorFilter = par("RingThrough_ER_DetectorFilter");
    RingDrop_ER_DetectorFilter = par("RingDrop_ER_DetectorFilter");

	useWDM = par("useWDM");

	numOfWavelengths = par("numOfWavelengthChannels");

	EperBit = (double)par("Detector_EperBit") + (double)par("Detector_Receiver_EperBit");
	Detector_StaticPower = par("Detector_StaticPower");
	StaticDetectorEnergyStat->track(Detector_StaticPower);


	debug(getFullPath(), "INIT: done", UNIT_INIT);
}

double Detector::GetLatency(int indexIn, int indexOut)
{
	return Latency_Detector;
}

double Detector::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	double totalLoss = 0;

	if(indexIn == indexOut || (indexIn == 1 && indexOut == 2) || (indexIn == 2 && indexOut == 1))
	{
		return MAX_INSERTION_LOSS;
	}
	if(useWDM)
	{

		if(indexIn == 0)
		{
			totalLoss += 8;
		}
		if(indexIn == 1)
		{
			totalLoss += 8;
		}
		if(indexIn == 2)
		{
			totalLoss += 5;
		}
		if(indexOut == 0)
		{
			totalLoss += 8;
		}
		if(indexOut == 1)
		{
			totalLoss += 8;
		}
		if(indexOut == 2)
		{
			totalLoss += 5;
		}

		return totalLoss*PropagationLoss;
	}
	else
	{
		return (PropagationLoss * 13) * (useWDM ? 1 : numOfWavelengths);
	}
}

double Detector::GetPassByRingLoss(int indexIn, int indexOut, double wavelength)
{
	if(useWDM)
	{
		if(indexIn == 0 && indexOut == 2 || indexIn == 2 && indexOut == 0)
		{
			if(IsInResonance(wavelength))
			{
				return PassByRingLoss + RingThrough_ER_DetectorFilter;
			}
			else
			{
				return PassByRingLoss;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return PassByRingLoss * (useWDM ? 1 : numOfWavelengths);
	}
}

double Detector::GetDropIntoRingLoss(int indexIn, int indexOut,	double wavelength)
{
	if (useWDM || useRingModel) {
		if (indexIn == 0 && indexOut == 1) {
			if (IsInResonance(wavelength)) {
				return PassThroughRingLoss;
			} else {
				return PassThroughRingLoss + RingDrop_ER_DetectorFilter;
			}
		} else {
			return 0;
		}

	} else {
		return PassThroughRingLoss;
	}

}

void Detector::SetRoutingTable()
{
	routingTable[0] = 1;
	routingTable[1] = 0;
	routingTable[2] = -1;
}

void Detector::SetOffResonanceRoutingTable()
{
	routingTableOffResonance[0] = 2;
	routingTableOffResonance[1] = -1;
	routingTableOffResonance[2] = 0;
}

double Detector::AddDetectionEnergyDissipation(double time)
{
	double energy = time * photonicBitRate * EperBit * (useWDM ? 1 : numOfWavelengths);
	DynamicDetectorEnergyStat->track(energy);
	return energy;
}
