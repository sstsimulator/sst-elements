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

#ifndef MUX_H_
#define MUX_H_

#include <omnetpp.h>
#include "messages_m.h"
#include "packetstat.h"
#include <queue>
using namespace std;

class mux : public cSimpleModule {
public:
	mux();
	virtual ~mux();

	virtual void handleMessage(cMessage* msg);
	virtual void initialize();
	virtual void finish();

	cGate* outgate;

	queue<cMessage*> fakeQ;
	cMessage* qMessage;
	cMessage* timerMessage;
	simtime_t clockPeriod;
	cChannel* transChannel;
};

class demux : public cSimpleModule {
public:
	demux();
	virtual ~demux();

	virtual void initialize();
	virtual void handleMessage(cMessage* msg);

	int numOut;
	map<int, cGate*> out;
};

class inoutConverter : public cSimpleModule {

public:
	inoutConverter();

protected:
	virtual void initialize();
	virtual void handleMessage(cMessage* msg);

	cGate* gin;
	cGate* gout;
	cGate* io_in;
	cGate* io_out;
};

#endif /* MUXIN_H_ */
