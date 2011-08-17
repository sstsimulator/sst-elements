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

#include "VC_Control.h"

VC_Control::VC_Control(int n) {
	numVC = n;

}

VC_Control::~VC_Control() {

}

int VC_Control_Random::getVC(int msgType, ProcessorData* pdata) {

	return intrand(numVC);

}

//all VC's are equal
int VC_Control_Random::nextVC(map<int, list<ArbiterRequestMsg*>*>* reqs,
		int curr) {
	int start = curr;
	int vc = (curr + 1) % numVC;

	while ((*reqs)[vc]->empty() && vc != (start)) {
		vc = (vc + 1) % numVC;

	}

	return vc;
}

int VC_Control_PhotonicPriority::getVC(int msgType, ProcessorData* pdata) {

	switch (msgType) {
	case pathBlocked:
	case pathTeardown:
		return 0;
		break;
	case pathACK:
		return 1;
		break;
	case pathSetup:
		return 2;
		break;
	default:
		return 3;
	}

}

int VC_Control_PhotonicPriority::nextVC(
		map<int, list<ArbiterRequestMsg*>*>* reqs, int currVC) {

	for (int i = 0; i < numVC; i++) {
		if (!(*reqs)[i]->empty()) {
			return i;
		}

	}

	return 0;
}

int VC_Control_DRAMreadWrite::getVC(int msgType, ProcessorData* pdata) {

	if (pdata == NULL) {
		return 0;
	}

	ApplicationData* app = (ApplicationData*) pdata->getEncapsulatedMsg();

	if (numVC == 3 && app->getType() == DM_writeLocal) {
		return 2;
	} else if (app->getType() == M_readResponse) {
		return 1;
	} else {
		return 0;
	}

}

int VC_Control_DRAMreadWrite::nextVC(map<int, list<ArbiterRequestMsg*>*>* reqs,
		int currVC) {

	int start = currVC;
	int vc = (currVC + 1) % numVC;

	while ((*reqs)[vc]->empty() && vc != (start)) {
		vc = (vc + 1) % numVC;

	}

	return vc;

}

