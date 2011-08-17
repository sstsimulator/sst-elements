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

#include "Arbiter_FB.h"

int Arbiter_FB::FB_Node = 6;
int Arbiter_FB::FB_Mem = 7;

Arbiter_FB::Arbiter_FB() {
	// TODO Auto-generated constructor stub

}

Arbiter_FB::~Arbiter_FB() {
	// TODO Auto-generated destructor stub
}

bool Arbiter_FB::isDimensionChange(int inport, int outport) {
	//this is for a mesh, or torus
	/*if (inport < 6) {
		if (outport < 6) {
			return (inport < 3) ^ (outport < 3);
		} else
			return false;
	} else if (outport < 6) {
		return true;
	} else*/
		//two NICs, which is fine
		return false;
}

//returns the port num to go to
int Arbiter_FB::route(ArbiterRequestMsg* rmsg){
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	int destX;
	int destY;

	destX = destId % numX;
	destY = destId / numY;


	debug(name, "destId ", destId, UNIT_ROUTER);
	debug(name, "...myId ", myAddr->id[level], UNIT_ROUTER);
	debug(name, "...myX ", myX, UNIT_ROUTER);
	debug(name, "...myY ", myY, UNIT_ROUTER);

	if(destX > myX)
	{
		return destX-1;
	}
	else if(destX < myX)
	{
		return destX;
	}
	else //(destX == myX)
	{
		if(destY > myY)
		{
			return numX-1+destY-1;
		}
		else if(destY < myY)
		{
			return numX-1+destY;
		}
		else
		{
			//if(destType == node_type_memory){

			//	return numPorts-1;
			//}else
			//	return 6 + (((destId % (numX*2)) % 2) + 2*((destId / (numY*2)) % 2));

			return FB_Node;
		}
	}

}


int Arbiter_FB::getDownPort(ArbiterRequestMsg* rmsg, int lev){
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	if(lev == translator->convertLevel("DRAM") && destId == 1){
		return FB_Mem;
	}

	return FB_Node;
}
