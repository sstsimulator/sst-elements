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

#ifndef VC_CONTROL_H_
#define VC_CONTROL_H_

#include <omnetpp.h>
#include <messages_m.h>

using namespace std;

class VC_Control {
public:
	VC_Control(int n);
	virtual ~VC_Control();

	int numVC;

	virtual int getVC(int msgType, ProcessorData* pdata) = 0;  //sets the VC of an outgoing electronic message
	virtual int nextVC(map<int, list<ArbiterRequestMsg*>* >* reqs, int currVC) = 0;  //used by arbiter to get next VC to arbitrate
};

class VC_Control_Random : public VC_Control {
public:
	VC_Control_Random(int n) : VC_Control(n){currVC = 0; };
	virtual ~VC_Control_Random(){};

	int currVC;

	virtual int getVC(int msgType, ProcessorData* pdata);  //sets the VC of an outgoing electronic message
	virtual int nextVC(map<int, list<ArbiterRequestMsg*>* >* reqs, int currVC);  //used by arbiter to get next VC to arbitrate
};

class VC_Control_PhotonicPriority : public VC_Control {
public:
	VC_Control_PhotonicPriority(int n) : VC_Control(n){ if(numVC != 4) {opp_error("hold the phone. PhotonicPriority virtual channel needs 4 virtual channels.");}};
	virtual ~VC_Control_PhotonicPriority(){};

	virtual int getVC(int msgType, ProcessorData* pdata);  //sets the VC of an outgoing electronic message
	virtual int nextVC(map<int, list<ArbiterRequestMsg*>* >* reqs, int currVC);  //used by arbiter to get next VC to arbitrate
};

class VC_Control_DRAMreadWrite : public VC_Control {
public:
	VC_Control_DRAMreadWrite(int n) : VC_Control(n){ if(numVC != 3 && numVC != 2) {opp_error("hold the phone. DRAMreadWrite virtual channel needs 2 virtual channels.");}};
	virtual ~VC_Control_DRAMreadWrite(){};

	virtual int getVC(int msgType, ProcessorData* pdata);  //sets the VC of an outgoing electronic message
	virtual int nextVC(map<int, list<ArbiterRequestMsg*>* >* reqs, int currVC);  //used by arbiter to get next VC to arbitrate
};



#endif /* VC_CONTROL_H_ */
