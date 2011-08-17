#include "facet.h"

Define_Module(Facet);

Facet::Facet()
{
}

Facet::~Facet()
{
}

void Facet::Setup()
{
	gateIdIn[0] = gate("photonicIn$i")->getId();
	gateIdIn[1] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicIn$o")->getId();
	gateIdOut[1] = gate("photonicOut$o")->getId();

	portType[0] = in;
	portType[1] = out;

	IL_Facet = par("IL_Facet");
}

double Facet::GetLatency(int indexIn, int indexOut)
{
	return 0;
}

double Facet::GetPropagationLoss(int indexIn, int indexOut, double wavelength)
{
	if(indexIn != indexOut)
	{
		return IL_Facet;
	}
	else
	{
		return MAX_INSERTION_LOSS;
	}
}

void Facet::SetRoutingTable()
{
	routingTable[0] = 1;
	routingTable[1] = 0;
}
