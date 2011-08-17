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

#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include <omnetpp.h>
#include "messages_m.h"
#include "packetstat.h"
#include "sim_includes.h"

#include "StatObject.h"
#include "statistics.h"

#ifdef ENABLE_ORION
#include "orion/ORION_Array.h"
#include "orion/ORION_Array_Info.h"
#include "orion/ORION_Util.h"
#endif


using namespace std;

class Serializer : public cSimpleModule
{
public:
	Serializer();
	virtual ~Serializer();
	virtual void initialize();
	virtual void handleMessage(cMessage* msg);
private:
	int currMsgId;
	int numPacketsLeftStart;
	int numPacketsLeftStop;


	// number of real wires/waveguides
	int numWiresIn;
	int numWiresOut;

	// number of wires/waveguides that are represented in this model
	// typical use: many-bit electrical bus is represented as a single omnet wire
	int numGatesIn;
	int numGatesOut;

	double rateIn;
	double rateOut;


#ifdef ENABLE_ORION
	ORION_FF* power;


	StatObject* P_static;
	StatObject* E_dynamic;

#endif
};

class Parallelizer : public cSimpleModule
{
public:
	Parallelizer();
	virtual ~Parallelizer();
	virtual void initialize();
	virtual void handleMessage(cMessage* msg);
private:

	int lockingClockCycles;

	int currMsgId;

	int numWiresIn;
	int numWiresOut;

	// number of wires/waveguides that are represented in this model
	// typical use: many-bit electrical bus is represented as a single omnet wire
	int numGatesIn;
	int numGatesOut;

	double rateIn;
	double rateOut;

#ifdef ENABLE_ORION
	ORION_FF* power;


	StatObject* P_static;
	StatObject* E_dynamic;

#endif


};

#endif /* _SERIALIZER_H_ */
