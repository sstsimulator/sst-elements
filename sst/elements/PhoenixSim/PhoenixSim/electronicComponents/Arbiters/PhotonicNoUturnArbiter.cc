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

#include "PhotonicNoUturnArbiter.h"

PhotonicNoUturnArbiter::PhotonicNoUturnArbiter() {
	// TODO Auto-generated constructor stub

}

PhotonicNoUturnArbiter::~PhotonicNoUturnArbiter() {
	// TODO Auto-generated destructor stub
}


int PhotonicNoUturnArbiter::pathStatus(ArbiterRequestMsg* rmsg, int outport) {

	if(rmsg->getReqType() == pathSetup){
		if(rmsg->getPortIn() == outport){

			std::cout << "outport = " << outport << endl;
			std::cout << "port in = " << rmsg->getPortIn() << endl;
			std::cout << "myAddr: " << translator->untranslate_str(myAddr) << endl;
			std::cout << "destId = " << Processor::translator->untranslate_str(((NetworkAddress*)rmsg->getDest())) << endl;

			opp_error("No U turns allowed in this router");
		}
	}

	return PhotonicArbiter::pathStatus(rmsg, outport);
}
