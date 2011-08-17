/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: bend.cc                                                         *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/


#include "bend.h"

Define_Module (Bend);

Bend::Bend()
{
}

Bend::~Bend()
{
}

void Bend::Setup()
{
	gateIdIn[0] = gate("photonicIn$i")->getId();
	gateIdIn[1] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicIn$o")->getId();
	gateIdOut[1] = gate("photonicOut$o")->getId();

	portType[0] = in;
	portType[1] = out;

	PropagationLoss = par("PropagationLoss");
	BendingLoss = par("BendingLoss");
	Latency_Bend = par("Latency_Bend");
	Angle_Bend = par("Angle_Bend");
}

double Bend::GetLatency(int indexIn, int indexOut)
{
	return Latency_Bend * 90/Angle_Bend;
}

double Bend::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		return (PropagationLoss * 2.5 * 0.5 * 3.141592) * 90/Angle_Bend;
	}
	else
	{
		return MAX_INSERTION_LOSS;
	}
}

double Bend::GetBendingLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		return BendingLoss * Angle_Bend/90;
	}
	else
	{
		return 0;
	}
}

void Bend::SetRoutingTable()
{
	routingTable[0] = 1;
	routingTable[1] = 0;
}
