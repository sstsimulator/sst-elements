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

#ifndef NIF_Electronic_H_
#define NIF_Electronic_H_

#include "NIF_Packet_Credit.h"
#include <queue>

class NIF_Electronic: public NIF_Packet_Credit {
public:
	NIF_Electronic();
	virtual ~NIF_Electronic();

protected:

	virtual bool request();
	virtual bool prepare();
	virtual simtime_t send();
	virtual void complete();

	virtual void init_packet();


	virtual void packetPortMsgArrived(cMessage* emsg);

	ProcessorData *currData;
	ElectronicMessage* currMsg;


	int maxPacketSize;

	int currVC;
	int leftToSend;

	int bitsSent;


	int conc;

	simtime_t startSerTime;

	StatObject* SO_lat_ser;
	StatObject* SO_lat_prop;
};

#endif /* NIF_PHOTONIC_H_ */
