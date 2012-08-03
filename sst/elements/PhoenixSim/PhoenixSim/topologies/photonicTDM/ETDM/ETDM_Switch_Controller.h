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

#ifndef TDM_SWITCH_CONTROLLER_H_
#define TDM_SWITCH_CONTROLLER_H_

#include <omnetpp.h>
#include "messages_m.h"
#include "sim_includes.h"
#include "statistics.h"

#ifdef ENABLE_ORION
#include "orion/ORION_Array.h"
#include "orion/ORION_Array_Info.h"
#include "orion/ORION_Util.h"
#include "orion/ORION_Link.h"
#endif

#include <map>

class ETDM_Switch_Controller: public cSimpleModule {
public:
	ETDM_Switch_Controller();
	virtual ~ETDM_Switch_Controller();

protected:
	virtual void handleMessage(cMessage* cmsg);
	virtual void finish();
	virtual void initialize();

	double getClockPower();

	int id;
	int myRow;
	int myCol;

	int tdmSlot;
	int numTDMslots;
	int tdmSlotIteration;
	int tdmSlotTime[28];
	int numX;
	int numY;

	int numDevices;

	double lutEnergy;

	simtime_t clockPeriod;
	cMessage* clockMsg;
	map<int, cGate*> devicePorts;



	enum PORTS {
		N = 0, E, S, W, MOD, DET
	};

	bool isRowSlot();
	void sendCntrlMessages(map<PORTS, PORTS> set);


	void readControlFile(string fName);

	int rx;
	int tx;

	StatObject* P_static;
	StatObject* E_dynamic;

	map<int, map<int, int>* > cntrlMatrix;

	map<int, map<int, int>* > printMatrix;

#ifdef ENABLE_ORION
	ORION_Array* power;
	ORION_Array_Info* info;

#endif
};

#endif /* TDM_SWITCH_CONTROLLER_H_ */
