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

#ifndef OCM_CONTROL_H_
#define OCM_CONTROL_H_

#include <omnetpp.h>
#include "messages_m.h"
#include "sim_includes.h"

#include "statistics.h"

#include "AddressTranslator.h"

using namespace std;

class OCM_Control: public cSimpleModule {
public:
	OCM_Control();
	virtual ~OCM_Control();

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage* msg);
	virtual void finish();

	void trackPower();

	int activeBanks;

	cGate* phIn;
	cGate* phOut;

	cGate* dataOut;
	cGate* dataIn;
	cGate* cntrlOut;

	bool rowRx;
	bool colRx;
	int bankId;
	long connectedto;

	int chips;
	int arrays;

	simtime_t clockPeriod;

	simtime_t photonicBitPeriod;
	int wavelengths;

	int dataRx;

	StatObject* SO_EN_read;
	StatObject* SO_EN_write;
	StatObject* SO_EN_act;
	StatObject* SO_EN_back;
	StatObject* SO_EN_pre;
	StatObject* SO_EN_dq;

	double pwr_idle_standby;
	double pwr_active_standby;

	double pwr_idle_down;
	double pwr_active_down;

	double pwr_row_cmd;
	double pwr_read;
	double pwr_write;

	double pwr_dq;

	double freq;

	simtime_t tRCD;
	simtime_t tCL;
	simtime_t tRP;

	simtime_t lastStaticTime;

};

#endif /* OCM_CONTROL_H_ */
