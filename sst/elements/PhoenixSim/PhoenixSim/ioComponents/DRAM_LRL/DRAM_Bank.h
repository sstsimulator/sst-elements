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

#ifndef DRAM_BANK_H_
#define DRAM_BANK_H_

#include <omnetpp.h>
#include "sim_includes.h"
#include "messages_m.h"

#include "NetworkAddress.h"
#include "statistics.h"

using namespace std;

class DRAM_Bank: public cSimpleModule {
public:
	DRAM_Bank();
	virtual ~DRAM_Bank();

	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);

protected:
	int id;

	int arrays;
	int rows;
	int cols;
	int chips;
	int banks;

	int burst;
	int currentOpenRow;
	int currentOpenCol;
	bool lastAccess;

	bool precharging;
	bool writing;

	NetworkAddress* coreAddr;

	cGate* cntrlIn;
	cGate* dataIn;
	cGate* dataOut;

	double freq;

	simtime_t tRCD;
	simtime_t tCL;
	simtime_t tRP;

	cMessage* prechargeMsg;
	cMessage* writingMsg;

	StatObject* SO_read;
	StatObject* SO_write;
	StatObject* SO_BW;



};

#endif /* DRAM_BANK_H_ */
