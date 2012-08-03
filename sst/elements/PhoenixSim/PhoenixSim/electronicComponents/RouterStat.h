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

#ifndef ROUTERSTAT_H_
#define ROUTERSTAT_H_

#include <omnetpp.h>
#include "sim_includes.h"
#include "statistics.h"
#include "messages_m.h"

#include "ORION_Util.h"
#include "ORION_Link.h"
#include "ORION_Crossbar.h"
#include "ORION_Arbiter.h"
#include "ORION_Array.h"

#define NANOSEC 0.000000001

class RouterStat: public cSimpleModule {
public:
	RouterStat();
	virtual ~RouterStat();

	static double area; //in um^2

protected:
	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);


	double getClockPower();
	void calculateArea();

	static bool areaInit;

	bool rxArb;
	bool rxXB;
	bool rxIn;

	int flit_width;
	int numPorts;
	int numVC;
	int buffer_size;
	double clockRate;

	int arb_reqWidth;

	cMessage* powerMsg;
	simtime_t powerPeriod;
	simtime_t lastReport;

	int arbModel;

	int xbarModel;
	int xbarDegree;

	int buffType;
	int share;
	int data_bitline_pre_model;
	int data_end;
	int rows;


	StatObject* P_static;
	StatObject* E_dynamic;
};

#endif /* ROUTERSTAT_H_ */
