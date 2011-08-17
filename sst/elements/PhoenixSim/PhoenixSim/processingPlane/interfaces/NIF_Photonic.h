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

#ifndef NIF_PHOTONIC_H_
#define NIF_PHOTONIC_H_

#include "NIF_Circuit.h"
#include <queue>

#define INTMAX pow(2, 31)

class NIF_Photonic: public NIF_Circuit {
public:
	NIF_Photonic();
	virtual ~NIF_Photonic();

protected:

	virtual bool request();
	virtual bool prepare();
	virtual simtime_t send();
	virtual void complete();

	virtual void init_circuit();

	virtual void dataMsgArrived(cMessage* pmsg);


	PhotonicMessage* start;
	PhotonicMessage* stop;


	simtime_t msgDuration;
	simtime_t enqueueTimeOfSendingMessage;
	double guardTimeGap;

	NetworkAddress* sourceAddr;
	NetworkAddress* destAddr;

	int bitsSent;


	static double DetectorSensitivity;

	//static int numX;
	//static int numY;


	StatObject* SO_lat_trans;



	void putInPacketQ(ProcessorData* pdata);

	string NIFselection;
	double NIFthreshold;
	int maxPacketSize;
	int numOfNodesX;


};

#endif /* NIF_PHOTONIC_H_ */
