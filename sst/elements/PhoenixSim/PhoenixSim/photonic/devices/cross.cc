/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: cross.cc                                                       *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/


#include "cross.h"

Define_Module (Cross);

Cross::Cross()
{
}

Cross::~Cross()
{
}

void Cross::Setup()
{
	gateIdIn[0] = gate("photonicHorizIn$i")->getId();
	gateIdIn[1] = gate("photonicVertIn$i")->getId();
	gateIdIn[2] = gate("photonicHorizOut$i")->getId();
	gateIdIn[3] = gate("photonicVertOut$i")->getId();

	gateIdOut[0] = gate("photonicHorizIn$o")->getId();
	gateIdOut[1] = gate("photonicVertIn$o")->getId();
	gateIdOut[2] = gate("photonicHorizOut$o")->getId();
	gateIdOut[3] = gate("photonicVertOut$o")->getId();

	portType[0] = in;
	portType[1] = in;
	portType[2] = out;
	portType[3] = out;

	PropagationLoss = par("PropagationLoss");
	CrossingLoss = par("CrossingLoss");

	Latency_Cross = par("Latency_Cross");
	Crosstalk_Cross = par("Crosstalk_Cross");
}

double Cross::GetLatency(int indexIn, int indexOut)
{
	return Latency_Cross;
}

double Cross::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	return PropagationLoss * 50;
}

double Cross::GetCrossingLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		if((indexIn+2)%4 == indexOut)
		{
			return CrossingLoss;
		}
		else
		{
			return Crosstalk_Cross;
		}
	}
	else
	{
		return MAX_INSERTION_LOSS;
	}
}

void Cross::SetRoutingTable()
{
	routingTable[0] = 2;
	routingTable[1] = 3;
	routingTable[2] = 0;
	routingTable[3] = 1;
}
