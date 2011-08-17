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

#ifndef NIF_MEMGW_H_
#define NIF_MEMGW_H_

#include "NIF_Circuit.h"

#include "NetworkAddress.h"

class NIF_MemGW: public NIF_Circuit {
public:
	NIF_MemGW();
	virtual ~NIF_MemGW();

protected:

	virtual bool request();
	virtual bool prepare();
	virtual simtime_t send();
	virtual void complete();

	virtual void init_circuit();

	virtual void dataMsgArrived(cMessage* pmsg);
	virtual void packetPortMsgArrived(cMessage* emsg);


    virtual void requestMsgArrived(RouterCntrlMsg* rmsg);



	void sendSetup(ProcessorData* pdata, simtime_t delay);

	ProcessorData *currData;
	ElectronicMessage* currMsg;

	NetworkAddress* myAddr;

	double clockPeriod;

	int id;

	int cols;
	int arrays;
	int chips;
	double photonicBitRate;
	double DRAM_freq;

	bool servicingRead;
};

#endif /* NIF_MEMGW_H_ */
