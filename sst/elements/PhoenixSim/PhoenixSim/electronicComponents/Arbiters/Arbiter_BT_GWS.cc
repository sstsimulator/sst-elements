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

#include "Arbiter_BT_GWS.h"

Arbiter_BT_GWS::Arbiter_BT_GWS(){


}

Arbiter_BT_GWS::~Arbiter_BT_GWS() {

}

bool Arbiter_BT_GWS::isDimensionChange(int inport, int outport){
	return inport == G_Gateway || outport == G_Gateway;
}

int Arbiter_BT_GWS::route(ArbiterRequestMsg* rmsg) {
	NetworkAddress* __attribute__ ((unused)) addr = (NetworkAddress*)rmsg->getDest();
//		int destId = addr->id[level];
	// To Do: make a smarter choice on which direction to inject packet
	int inport = rmsg->getPortIn();

	if (inport == G_InjectionN) {
		return G_InjectionS;
	} else if (inport == G_InjectionS) {
		return G_InjectionN;
	} else if (inport == G_Ejection) {
		return G_Gateway;
	} else {
		return intrand(2) > 1 ? G_InjectionN : G_InjectionS;
	}
}

void Arbiter_BT_GWS::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if (inport == G_Gateway && (outport == G_InjectionN || outport == G_InjectionS))
	{
		if (outport == G_InjectionN)
		{
			nextPSEState[0] = st;
		}
		else
		{
			nextPSEState[1] = st;
		}
	}
}
