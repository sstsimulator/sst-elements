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

#include "Arbiter_ET.h"

Arbiter_ET::Arbiter_ET() {

}

Arbiter_ET::~Arbiter_ET() {
	// TODO Auto-generated destructor stub
}

//returns the port num to go to
int Arbiter_ET::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	int destX;
	int destY;

	//	if(isConc){
	//		destX = (destId % (numX * 2)) / 2;
	//		destY = (destId / (numY * 2)) / 2;
	//	}else{
	destX = destId % numX;
	destY = destId / numY;
	//	}

	if (destX > myX) {
		if (myX % 2 != destX % 2 && myX < numX / 2 && destX < numX / 2) {
			return EM_W;
		} else {
			return EM_E;
		}

	} else if (destX < myX) {
		if (myX % 2 != destX % 2 && myX >= numX / 2 && destX >= numX / 2) {
			return EM_E;
		} else {
			return EM_W;
		}
	} else //(destX == myX)
	{

		if (destY > myY) {
			if (myY % 2 != destY % 2 && myY < numY / 2 && destY < numY / 2) {
				return EM_N;
			} else {
				return EM_S;
			}

		} else if (destY < myY) {

			if (myY % 2 != destY % 2 && myY >= numY / 2 && destY >= numY / 2) {
				return EM_S;
			} else {
				return EM_N;
			}

		} else {
			//if (destType == node_type_memory) {

			//	return numPorts-1;
			//} else
			//	return EM_Node + isConc*(((destId % (numX*2)) % 2) + 2*((destId / (numY*2)) % 2));

			return EM_Node;
		}
	}

}


int Arbiter_ET::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	if (lev == translator->convertLevel("DRAM") && destId == 1) {
		return EM_Mem;
	}

	return EM_Node;
}
