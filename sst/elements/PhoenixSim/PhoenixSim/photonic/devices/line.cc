/************************************************************************
*                                                                       *
*  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
*						(c) Johnnie Chan  2008							*
*                                                                       *
* file: line.cc                                                         *
* description:                                                          *
*                                                                       *
*                                                                       *
*************************************************************************/


#include "line.h"

Define_Module (Line);

Line::Line()
{
}

Line::~Line()
{
}

void Line::Setup()
{
	gateIdIn[0] = gate("photonicIn$i")->getId();
	gateIdIn[1] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicIn$o")->getId();
	gateIdOut[1] = gate("photonicOut$o")->getId();

	portType[0] = in;
	portType[1] = out;

	Length_Line = par("Length_Line");
	LatencyRate_Line = par("LatencyRate_Line");
	PropagationLoss = par("PropagationLoss");
}

double Line::GetLatency(int indexIn, int indexOut)
{
	return Length_Line*LatencyRate_Line;
}

double Line::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		return Length_Line*PropagationLoss;
	}
	else
	{
		return MAX_INSERTION_LOSS;
	}
}

void Line::SetRoutingTable()
{
	routingTable[0] = 1;
	routingTable[1] = 0;
}
