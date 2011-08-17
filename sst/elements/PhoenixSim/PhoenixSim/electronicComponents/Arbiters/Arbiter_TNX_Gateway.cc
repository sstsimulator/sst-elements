/*
 * Arbiter_TNX_Gateway.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#include "Arbiter_TNX_Gateway.h"

Arbiter_TNX_Gateway::Arbiter_TNX_Gateway() {
	// TODO Auto-generated constructor stub

}

Arbiter_TNX_Gateway::~Arbiter_TNX_Gateway() {
	// TODO Auto-generated destructor stub
}


int Arbiter_TNX_Gateway::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
	int destId = addr->id[level];
	int destx;
	int desty;
	int inport = rmsg->getPortIn();
	destx = destId % numX;
	desty = destId / numY;
	int hopsleft, hopsright;
	int path;

	if(inport == GW_Out)
	{
		// if the destination router is already on the same row, route directly to 4x4 switch
		if(myY == desty)
		{
			if((myX + myY) % 2 == 0)
			{
				return GW_S;
			}
			else
			{
				return GW_N;
			}
		}
		if(myY % 2 == desty % 2)
		{
			if(desty > myY)
			{
				return GW_S;
			}
			else
			{
				return GW_N;
			}
		}
		if(myY % 2 == 0)
		{
			hopsleft = (myY>>1) + (desty>>1);
			hopsright = ((numY - myY)>>1) + ((numY - desty - 1)>>1);
		}
		else
		{
			hopsleft = ((myY + 1)>>1) + (desty>>1);
			hopsright = ((numY - myY)>>1) + ((numY - desty - 1)>>1);
		}
		if(hopsleft < hopsright)
		{
			return GW_N;
		}
		else
		{
			return GW_S;
		}

	}

	// If at dest node and on X traversal
	if(destx == myX && desty == myY)
	{
		if(inport == GW_E || inport == GW_W)
		{
			return GW_Out;
		}
	}

	if(inport == GW_N)
	{
		return GW_S;
	}
	else if(inport == GW_S)
	{
		return GW_N;
	}
	else if(inport == GW_E)
	{
		return GW_W;
	}
	else
	{
		return GW_E;
	}


}

void Arbiter_TNX_Gateway::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if(inport == GW_Out && outport == GW_N)
	{
		nextPSEState[0] = st;
	}
	else if(inport == GW_E && outport == GW_Out)
	{
		nextPSEState[2] = st;
	}
	else if(inport == GW_Out && outport == GW_S)
	{
		nextPSEState[1] = st;
	}
	else if(inport == GW_W && outport == GW_Out)
	{
		nextPSEState[3] = st;
	}
}
