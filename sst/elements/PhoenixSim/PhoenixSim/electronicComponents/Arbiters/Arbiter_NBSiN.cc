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

#include "Arbiter_NBSiN.h"


Arbiter_NBSiN::Arbiter_NBSiN() {
	// TODO Auto-generated constructor stub

}

Arbiter_NBSiN::~Arbiter_NBSiN() {
	// TODO Auto-generated destructor stub
}


int Arbiter_NBSiN::route(ArbiterRequestMsg* rmsg){
	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	NetworkAddress* addr2 = (NetworkAddress*) rmsg->getSrc();
	int srcId = addr2->id[level];
	int srcX = srcId % numX;
	int srcY = srcId / numX;
	int destX;
	int destY;

	destX = destId % numX;
	destY = destId / numX;

	std::cout<<"my loc: "<<myX<<":"<<myY<<" >> src: "<<srcX<<":"<<srcY<<" --> dest: "<<destX<<":"<<destY<<endl;
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
			return numX + numY - 2;
		}
	}

}

void Arbiter_NBSiN::PSEsetup(int inport, int outport, PSE_STATE st)
{
	int row, col;

	if(inport == numX + numY - 2)
	{
		if(outport >= numX - 1 && outport < numX + numY - 2)
		{
			// output goes to somewhere in this column
			row = numX - 1 - myX;
		}
		else
		{
			return;
		}
	}
	else if(inport >= myX)
	{
		row = numX - 1 - (inport + 1) ;
	}
	else // (inport < myX)
	{
		row = numX - 1 - inport;
	}

	if(outport == numX + numY - 2)
	{
		if(inport >= 0 && inport < numX - 1)
		{
			col = myY;
		}
		else
		{
			return;
		}
	}
	else if(outport - (numX - 1) < myY)
	{
		col = outport - (numX - 1);
	}
	else
	{
		col = outport + 1 - (numX - 1);
	}
	nextPSEState[row*numX + col] = st;
}

