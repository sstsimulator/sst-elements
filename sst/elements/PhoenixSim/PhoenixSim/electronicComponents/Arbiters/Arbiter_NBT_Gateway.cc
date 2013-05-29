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

#include "Arbiter_NBT_Gateway.h"

Arbiter_NBT_Gateway::Arbiter_NBT_Gateway(){


}

Arbiter_NBT_Gateway::~Arbiter_NBT_Gateway() {

}

void Arbiter_NBT_Gateway::init(string id, int level, int numX, int numY, int vc, int ports,
		int numpse, int buff,  string n)
{
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff,  n);

	nodeRow = myAddr->id[level]>>1;
	swIsN = myAddr->id[level] < numX;

}


int Arbiter_NBT_Gateway::route(ArbiterRequestMsg* rmsg)
{
	int inport = rmsg->getPortIn();
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	if (inport == NBMGR_Gateway)
	{

		int destRow = destId >> 1;

		int lenUp = 0;
		int lenDown = 0;
		// calculate to go up or down
		// use knowledge of torus, even rows on one side, odd rows on other side
		if (destRow % 2 == nodeRow % 2)
		{
			if ((swIsN && destRow == nodeRow) || (destRow < nodeRow))
			{
				return NBMGR_InjectionN;
			}
			else// (swIsN && destRow > nodeRow || !swIsN && destRow < nodeRow)
			{
				return NBMGR_InjectionS;
			}
		}
		else // !(destRow%2==nodeRow%2)
		{
			if (swIsN)
			{
				lenUp += 1;
			}
			else
			{
				lenDown += 1;
			}

			//don't count the switch at which msg turns
			lenUp += nodeRow >> 1;
			lenDown += (numX - 1 - nodeRow) >> 1;

			lenUp += destRow >> 1;
			lenDown += (numX - 1 - destRow) >> 1;

			if (lenUp < lenDown)
			{
				return NBMGR_InjectionN;
			}
			else
			{
				return NBMGR_InjectionS;
			}

		}
	}
	else if ((inport == NBMGR_InjectionN && swIsN) || (inport == NBMGR_InjectionS && !swIsN))
	{
		return NBMGR_Gateway;
	}
	else
	{
		opp_error("Should only be able to come into this switch through gateway or ejection port");
	}
	return 0; // Added to avoid Compile Warning
}

void Arbiter_NBT_Gateway::PSEsetup(int inport, int outport, PSE_STATE st) {
	if (inport == NBMGR_Gateway && outport == NBMGR_InjectionS) {
		nextPSEState[0] = st;
	}
}
