/*
 * Arbiter_TNX_Node.cpp
 *
 *  Created on: Aug 3, 2009
 *      Author: Johnnie Chan
 */

#include "Arbiter_TNX_Node.h"

Arbiter_TNX_Node::Arbiter_TNX_Node()  {
	// TODO Auto-generated constructor stub

}

Arbiter_TNX_Node::~Arbiter_TNX_Node() {
	// TODO Auto-generated destructor stub
}


int Arbiter_TNX_Node::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
	int destId = addr->id[level];
	int destx;
	int desty;
	int inport = rmsg->getPortIn();
	int hopsleft, hopsright;
	destx = destId % numX;
	desty = destId / numY;
	// Y-X routing

	// if on X traversal, already on correct Y-dim, just need to run into the gateway
	if(inport == SW_E)
	{
		return SW_W;
	}
	if(inport == SW_W)
	{
		return SW_E;
	}

	// if reach correct Y dimension, find shortest path to correct gateway
	if(desty == myY)
	{
		if(myX % 2 != destx % 2)
		{
			if(destx > myX)
			{
				return SW_E;
			}
			else
			{
				return SW_W;
			}
		}
		if(myX % 2 == 0)
		{
			hopsleft = (myX>>1) + (destx>>1);
			hopsright = ((numX - myX)>>1) + ((numX - destx - 1)>>1);
		}
		else
		{
			hopsleft = ((myX + 1)>>1) + (destx>>1);
			hopsright = ((numX - myX)>>1) + ((numX - destx - 1)>>1);
		}
		if(hopsleft < hopsright)
		{
			return SW_W;
		}
		else
		{
			return SW_E;
		}
	}
	else
	{
		if(inport == SW_N)
		{
			return SW_S;
		}
		else //(inport == SW_S)
		{
			return SW_N;
		}
	}

}
