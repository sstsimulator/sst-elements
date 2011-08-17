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

#include "StarCoupler.h"

Define_Module(StarCoupler);

StarCoupler::StarCoupler() {
	// TODO Auto-generated constructor stub

}

StarCoupler::~StarCoupler() {
	// TODO Auto-generated destructor stub
}


void StarCoupler::initialize()
{
	numOuts = par("numOuts");

	theIn = gate("theIn$i");

	for(int i = 0; i < numOuts; i++){
		theOuts[i] = gate("theOuts$o", i);
	}

}

void StarCoupler::finish(){


}

void StarCoupler::handleMessage(cMessage *msg)
{
	cGate* arr = msg->getArrivalGate();

	if(arr == theIn){
		for(int i = 0; i < numOuts-1; i++){
			send(msg->dup(), theOuts[i]);
		}

		send(msg, theOuts[numOuts-1]);

	}
}
