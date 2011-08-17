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

#include "Arbiter_BT_EJ.h"

Arbiter_BT_EJ::Arbiter_BT_EJ(){
	// TODO Auto-generated constructor stub

}

Arbiter_BT_EJ::~Arbiter_BT_EJ()
{
	// TODO Auto-generated destructor stub
}

bool Arbiter_BT_EJ::isDimensionChange(int inport, int outport){
	return inport == EJ_Gateway || outport == EJ_Gateway;
}


int Arbiter_BT_EJ::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
		int destId = addr->id[level];
	int destRow;
	int inport = rmsg->getPortIn();

	//if(isConc){
	//	destRow = (destId / (numY * 2)) / 2;
	//}else{
		destRow = destId / numX;
	//}

	//if row matches, then eject
	if (myY == destRow)
	{
		return EJ_Gateway;
	}
	else
	{
		if (inport == EJ_N)
		{
			return EJ_S;
		}
		else
		{
			return EJ_N;
		}
	}
}


void Arbiter_BT_EJ::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if(inport == EJ_N && outport == EJ_Gateway)
	{
		nextPSEState[1] = st;
	}
	else if(inport == EJ_S && outport == EJ_Gateway)
	{
		nextPSEState[0] = st;
	}
}
