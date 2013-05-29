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

#include "Arbiter_OM4x4.h"

Arbiter_OM4x4::Arbiter_OM4x4() {


}

Arbiter_OM4x4::~Arbiter_OM4x4() {

}




//returns the port num to go to
int Arbiter_OM4x4::route(ArbiterRequestMsg* rmsg) {

	NetworkAddress* addr = (NetworkAddress*) rmsg->getDest();
	int destId = addr->id[level];

	//do it specific for 4x4 for now
	switch(myAddr->id[level]){
	case 0: {
		return destId;
		break;
	}
	case 1: {

		break;
	}
	case 2: {
		return destId % 4;
		break;
	}
	case 3: {

		break;
	}
	case 4: {
		return destId % 4;
		break;
	}
	case 5: {

		break;
	}
	case 6: {
		return destId % 4;
		break;
	}
	case 7: {

		break;
	}


	}
	return 0; // Added to avoid Compile Warning 
}

void Arbiter_OM4x4::PSEsetup(int inport, int outport, PSE_STATE st){


	if(outport == 0){
		if(inport == 0){
			//let 'er ride
		}else if(inport == 1){
			nextPSEState[PSE_0_1] = st;
		}else if(inport == 2){
			nextPSEState[PSE_0_2] = st;
		}else if(inport == 3){
			nextPSEState[PSE_0_3] = st;
		}
	}else if(outport == 1){
		if(inport == 0){
			nextPSEState[PSE_1_0] = st;
		}else if(inport == 1){
			//let 'er ride
		}else if(inport == 2){
			nextPSEState[PSE_1_2] = st;
		}else if(inport == 3){
			nextPSEState[PSE_1_3] = st;
		}
	}else if(outport == 2){
		if(inport == 0){
			nextPSEState[PSE_2_0] = st;
		}else if(inport == 1){
			nextPSEState[PSE_2_1] = st;
		}else if(inport == 2){
			//let 'er ride
		}else if(inport == 3){
			nextPSEState[PSE_2_3] = st;
		}
	}else if(outport == 3){
		if(inport == 0){
			nextPSEState[PSE_3_0] = st;
		}else if(inport == 1){
			nextPSEState[PSE_3_1] = st;
		}else if(inport == 2){
			nextPSEState[PSE_3_2] = st;
		}else if(inport == 3){
			//let 'er ride
		}
	}
}
