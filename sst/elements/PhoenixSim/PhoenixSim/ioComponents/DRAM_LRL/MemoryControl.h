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

#ifndef MEMORYCONTROL_H_
#define MEMORYCONTROL_H_

#include <omnetpp.h>
#include <queue>
#include "sim_includes.h"
#include "messages_m.h"

using namespace std;

#define DRAM_CNTRL_SIZE 64

class MemoryControl: public cSimpleModule {
public:
	MemoryControl();
	virtual ~MemoryControl();

protected:
	virtual void initialize();
	virtual void finish();
	virtual void handleMessage(cMessage* msg);

	virtual void setSwitchRead(int dimm, int on, simtime_t delay);
	virtual void setSwitchMods(int dimm, int on, simtime_t delay);
	virtual void setSwitchWrite(int dimm, int on, simtime_t delay);
	virtual void turnSwitchOff(simtime_t delay);

	int getDIMM(int addr);
	virtual void sendReadcommands(simtime_t delay);
	virtual void sendWritecommands(simtime_t delay);
	void write(MemoryAccess* mem, bool first, simtime_t delay);

	void startNewTransaction(simtime_t delay);

	void readMutex();

	int numDIMMs;

	simtime_t tRCD;
	simtime_t tCL;
	simtime_t tRP;

	int rows;
	int cols;
	int arrays;
	int banks;
	int chips;
	double freq;

	bool servicingRead;
	bool readPathSetup;

	int accessUnit; //in bytes
	simtime_t accessTime;

	simtime_t ToF;  //estimation of worst-case time of flight through network

	ProcessorData* currentAccess;
	list<ProcessorData*> waiting;
	int chunk;

	cGate* fromNIF;
	cGate* toNIF;
	cGate* nifReqIn;
	cGate* nifReqOut;

	cGate* toDIMMs;
	map<int, cGate*> switchCntrl;

	cMessage* moreCommands;
	cMessage* unlockMsg;

	simtime_t photonicBitPeriod;

	bool locked;
};

class MemoryControl_E : public MemoryControl {
public:
	MemoryControl_E();

protected:

	virtual void setSwitchRead(int dimm, int on, simtime_t delay);
	virtual void setSwitchMods(int dimm, int on, simtime_t delay);
	virtual void setSwitchWrite(int dimm, int on, simtime_t delay);
	virtual void turnSwitchOff(simtime_t delay);
	virtual void sendReadcommands(simtime_t delay);
	virtual void sendWritecommands(simtime_t delay);
};

#endif /* MEMORYCONTROL_H_ */
