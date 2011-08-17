#include "join.h"

Define_Module (Join);

Join::Join()
{
}

Join::~Join()
{
}

void Join::Setup()
{
	gateIdIn[0] = gate("photonicIn1$i")->getId();
	gateIdIn[1] = gate("photonicIn2$i")->getId();
	gateIdIn[2] = gate("photonicOut$i")->getId();

	gateIdOut[0] = gate("photonicIn1$o")->getId();
	gateIdOut[1] = gate("photonicIn2$o")->getId();
	gateIdOut[2] = gate("photonicOut$o")->getId();

	portType[0] = in;
	portType[1] = in;
	portType[2] = out;
}


void Join::SetRoutingTable()
{
	routingTable[0] = 2;
	routingTable[1] = 2;
	routingTable[2] = -1;
}
