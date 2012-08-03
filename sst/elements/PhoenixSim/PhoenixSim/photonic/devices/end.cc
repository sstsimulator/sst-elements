#include "end.h"
#include "sim_includes.h"

Define_Module(End);

End::End()
{
}

End::~End()
{
}

void End::initialize()
{
}

void End::handleMessage(cMessage *msg)
{
	debug(getFullPath(), "photonic message received", UNIT_OPTICS);
	if(((PhotonicMessage*)msg)->getMsgType() == TXstop)
	{
		delete (PacketStat*)((PhotonicMessage*)msg)->getPacketStat();
	}
	delete msg;
}
