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

#include "Arbiter_Crossbar.h"

Arbiter_Crossbar::Arbiter_Crossbar() {



}

Arbiter_Crossbar::~Arbiter_Crossbar() {

}

void Arbiter_Crossbar::init(string id, int level, int numX, int numY, int vc, int ports, int numpse, int buff, string n)
{
	PhotonicArbiter::init(id, level, numX, numY, vc, ports, numpse, buff,  n);
}


int Arbiter_Crossbar::route(ArbiterRequestMsg* rmsg)
{
	NetworkAddress* addr = (NetworkAddress*)rmsg->getDest();
	int destId = addr->id[level];


	//direction is just GW # since its a fully connected crossbar
	//std::cout<<inport<<"-->"<<destId<<endl;
	return destId;
}

void Arbiter_Crossbar::PSEsetup(int inport, int outport, PSE_STATE st)
{
	if(variant == 0){
		nextPSEState[inport*numX + outport] = st;
	}else if(variant == 1){
		nextPSEState[inport*numX + outport] = st;
		nextPSEState[inport*numX + outport + numX*numY] = st;
	}
}
