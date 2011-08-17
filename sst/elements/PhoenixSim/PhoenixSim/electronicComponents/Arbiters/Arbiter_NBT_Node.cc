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

#include "Arbiter_NBT_Node.h"

Arbiter_NBT_Node::Arbiter_NBT_Node() {

}

Arbiter_NBT_Node::~Arbiter_NBT_Node() {

}

void Arbiter_NBT_Node::init(string id, int level, int numX, int numY, int vc, int ports,
		int numpse, int buff,  string n)
{
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff,  n);

	isConnected = myX == myY || myX == numX - 1 - myY;

	GWisN = myAddr->id[level] >= numX*(numX>>1);

}


int Arbiter_NBT_Node::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	int destY;
	int destX;
	int inport = rmsg->getPortIn();

	int newId =  destId;

	if (newId < numX && destId % 2 == 0) // NW Quadrant
	{
		destX = newId >> 1;
		destY = newId >> 1;
	}
	else if (newId < numX && newId % 2 != 0)  // NE Quadrant
	{
		destX = numX - 1 - (newId >> 1);
		destY = newId >> 1;
	}
	else if (newId >= numX && newId % 2 == 0)  // SW Quadrant
	{
		destX = (numX >> 1) - 1 - ((newId - numX) >> 1);
		destY = newId >> 1;
	}
	else //(newId >= numOfNodesX && newId%2!=0)  // SE Quadrant
	{
		destX = newId >> 1;
		destY = newId >> 1;
	}
	//Horizontal Loop Behavior -- assumes on correct row now
	if (inport == SW_E || inport == SW_W)
	{
		if (isConnected)
		{
			if (myX == destX)
			{
				if (GWisN)
				{
					return SW_N;
				}
				else
				{
					return SW_S;
				}
			}
		}

		//else keep going around horizontal loop
		if (inport == SW_E)
		{
			return SW_W;
		}
		else
		{
			return SW_E;
		}
	}
	else //Vertical Loop Behavior (srcDir == SW_N || srcDir == SW_S) -- looking for point to turn onto row loop
	{
		if (destY == myY)
		{
			// to prevent blocking, just need to make the direction of travel for the left or right node in
			// the particular row unique. Therefore just assign one to be clockwise, other to be counter-clockwise
			// the particular assignment doesn't need to be constant for all rows, just that its unique
			//ev<<" same Y ";
			if (isConnected)
			{
				if (destX == myX)
				{
					if (GWisN)
					{
						return SW_N;
					}
					else
					{
						return SW_S;
					}
				}
			}

			if (newId % 2 == 0)
			{
				if (myX % 2 == 0)
				{
					return SW_E;
				}
				else
				{
					return SW_W;
				}
			}
			else
			{
				if (myX % 2 == 0)
				{
					return SW_W;
				}
				else
				{
					return SW_E;
				}
			}
		} else
		{
			//ev<<"keep going"<<endl;
			// keep going on vert loop
			if (inport == SW_N)
			{
				return SW_S;
			}
			else
			{
				return SW_N;
			}

		}
	}

}

void Arbiter_NBT_Node::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if (inport == outport)
	{
		opp_error("no u turns allowed in this nonblocking switch");
	}

	if (inport == SW_N)
	{
		if (outport == SW_E)
		{
			nextPSEState[4] = st;
		}
		else if (outport == SW_S)
		{
			//   vvv need to check this incase another path is using the same PSE
			nextPSEState[1] = (InputGateState[2] == gateActive && InputGateDir[2] == SW_W && OutputGateState[3] == gateActive && OutputGateDir[3] == SW_E) ? PSE_ON : st;
		}
		else if (outport == SW_W)
		{
			// do nothing
		}
	}
	else if (inport == SW_S)
	{
		if (outport == SW_W)
		{
			nextPSEState[5] = st;
		}
		else if (outport == SW_N)
		{
			nextPSEState[0] = (InputGateState[3] == gateActive && InputGateDir[3] == SW_E && OutputGateState[2] == gateActive && OutputGateDir[2] == SW_W) ? PSE_ON : st;
		}
		else if (outport == SW_E)
		{
			// do nothing
		}
	}
	else if (inport == SW_E)
	{
		if (outport == SW_N)
		{;
			nextPSEState[2] = st;
		}
		else if (outport == SW_W)
		{
			nextPSEState[1] = (InputGateState[0] == gateActive && InputGateDir[0] == SW_S && OutputGateState[1] == gateActive && OutputGateDir[1] == SW_N) ? PSE_ON : st;
		}
		else if (outport == SW_S)
		{
			// do nothing
		}
	}
	else if (inport == SW_W)
	{
		if (outport == SW_S)
		{
			nextPSEState[3] = st;
		}
		else if (outport == SW_E)
		{
			nextPSEState[0] = (InputGateState[1] == gateActive && InputGateDir[1] == SW_N && OutputGateState[0] == gateActive && OutputGateDir[0] == SW_S) ? PSE_ON : st;
		}
		else if (outport == SW_N)
		{
			// do nothing
		}
	}
	else
	{
		opp_error("unknown input direction specified");
	}
}
