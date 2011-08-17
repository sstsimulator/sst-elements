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

#ifndef PHOTONICARBITER_H_
#define PHOTONICARBITER_H_

#include "ElectronicArbiter.h"

class PhotonicArbiter: public ElectronicArbiter {
public:
	PhotonicArbiter();
	virtual ~PhotonicArbiter();

	virtual void init(string id,int lev, int x, int y, int vc, int ports, int numpse,
			int buff_size, string n);

protected:
	virtual list<RouterCntrlMsg*>* setupPath(ArbiterRequestMsg* rmsg,
			int outport);
	virtual void pathNotSetup(ArbiterRequestMsg* rmsg, int p);

	virtual int getOutport(ArbiterRequestMsg* rmsg);

	virtual int pathStatus(ArbiterRequestMsg* rmsg, int outport);

	virtual void PSEsetup(int inport, int outport, PSE_STATE st) = 0;
	void getPSEmessages(list<RouterCntrlMsg*>* msgs);

	virtual void cleanup(int numG);

	map<int, int> nextPSEState;
	map<int, int> currPSEState;

	bool turnaround;

	enum gateState {
		gateSetup=0, gateActive, gateFree
	};

	map<int, int> InputGateDir;
	map<int, int> OutputGateDir;
	map<int, gateState> InputGateState;
	map<int, gateState> OutputGateState;

	enum PH_PATH_CODES {
		PATH_BLOCKED = 20, TURNAROUND_OK, CANCEL_PHOTONIC, CANCEL_OK
	};

};

#endif /* PHOTONICARBITER_H_ */
