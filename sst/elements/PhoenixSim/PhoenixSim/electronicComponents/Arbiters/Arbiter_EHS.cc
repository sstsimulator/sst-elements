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

#include "Arbiter_EHS.h"

Arbiter_EHS::Arbiter_EHS() {


}

Arbiter_EHS::~Arbiter_EHS() {

}





//returns the port num to go to
int Arbiter_EHS::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
		int destId = addr->id[level];

	int destX = destId % numX;
	int destY = destId / numY;

	if (destX > myX) {

		if (destX == myX + 1 && myX % 2 == 0)
			return EM_E;
		else if (myX % 4 >= 2)
			return EM_X;
		else if (myY % 2 == 0 && myX % 4 < 2)
			return EM_N;
		else if (myY % 2 == 1 && myX % 4 < 2)
			return EM_S;

	} else if (destX < myX) {
		if (destX == myX - 1 && myX % 2 == 1)
			return EM_W;
		else if (myY % 2 == 0 && myX % 4 >= 2)
			return EM_N;
		else if (myY % 2 == 1 && myX % 4 >= 2)
			return EM_S;
		else if (myX % 4 < 2)
			return EM_X;
	} else //(destX == myX)
	{
		if (destY > myY) {
			if (destY == myY + 1 && myY % 2 == 0)
				return EM_S;
			else if (myY % 4 >= 2)
				return EM_Y;
			else if (myX % 2 == 0 && myY % 4 < 2)
				return EM_W;
			else if (myX % 2 == 1 && myY % 4 < 2)
				return EM_E;

		} else if (destY < myY) {
			if (destY == myY - 1 && myY % 2 == 1)
				return EM_N;
			else if (myX % 2 == 0 && myY % 4 >= 2)
				return EM_W;
			else if (myX % 2 == 1 && myY % 4 >= 2)
				return EM_E;
			else if (myY % 4 < 2)
				return EM_Y;
		} else {
			//if (destType == node_type_memory) {

			//	return EM_Mem;
			//} else
				return EM_Node;
		}
	}

	opp_error("ArbiterEHS: should never get here");

	return -1;
}

int Arbiter_EHS::getDownPort(ArbiterRequestMsg* rmsg, int lev) {
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[lev];

	if (lev == translator->convertLevel("DRAM") && destId == 1) {
		if (myY == 0) {
			return EM_N;
		} else if (myY == numY - 1) {
			return EM_S;
		} else if (myX == 0) {
			return EM_W;
		} else if (myX == numX - 1) {
			return EM_E;
		} else {
			return route(rmsg);
		}
	}

	return EM_Node;
}

