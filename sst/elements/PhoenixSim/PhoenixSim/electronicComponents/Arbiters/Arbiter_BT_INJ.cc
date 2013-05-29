//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "Arbiter_BT_INJ.h"

Arbiter_BT_INJ::Arbiter_BT_INJ() {

}

Arbiter_BT_INJ::~Arbiter_BT_INJ() {

}


bool Arbiter_BT_INJ::isDimensionChange(int inport, int outport){
	return inport / 2 != outport / 2;
}

int Arbiter_BT_INJ::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
		int destId = addr->id[level];
	int destx;
	int desty;
	int inport = rmsg->getPortIn();

	//if(isConc){
	//	destx = (destId % (numX * 2)) / 2;
	//	desty = (destId / (numY * 2)) / 2;
	//}else{
		destx = destId % numX;
		desty = destId / numY;
	//}
	int hopsleft, hopsright;
	// To Do: make a smarter choice on which direction to inject packet

	if (destx != 0 || destx != numX - 1)
	{
		if (destx % 2 == 0)
		{
			destx -= 1;
		} else
		{
			destx += 1;
		}
	}

	if (inport == I_N || inport == I_S)
	{
		//specific to folded torus topology
		if ((myX % 2 == 0 && destx % 2 == 1) || (myX % 2 == 1 && destx % 2 == 0))
		{
			// if the intermediate switch is on the same side of the loop as the injection point
			if (destx < myX)
			{
				return I_W;
			}
			else
			{
				return I_E;
			}
		}
		else
		{
			if (myX % 2 == 0)
			{
				//nodes == 4x4 switches
				//minimize: num of nodes to edge from injection point + num of nodes on other side from edge to intermediate turn point
				hopsleft = (myX >> 1) + (destx >> 1);
				hopsright = ((numX - myX) >> 1) + ((numX - destx - 1) >> 1);
			}
			else
			{
				hopsleft = ((myX + 1) >> 1) + ((destx - 1) >> 1);
				hopsright = ((numX - 1 - myX) >> 1) + ((numX - destx) >> 1);
			}
			if (hopsleft < hopsright)
			{
				return I_W;
			}
			else
			{
				return I_E;
			}
		}
	}
	else
	{
		if (inport == I_E)
		{
			return I_W;
		}
		else
		{
			return I_E;
		}
	}

}

void Arbiter_BT_INJ::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if (inport == I_N)
	{
		if (outport == I_E)
		{
			nextPSEState[2] = st;
		}
		else if(outport == I_W)
		{
			nextPSEState[0] = st;
		}
	}
	else if(inport == I_S)
	{
		if (outport == I_E)
		{
			nextPSEState[3] = st;
		}
		else if(outport == I_W)
		{
			nextPSEState[1] = st;
		}
	}
}

