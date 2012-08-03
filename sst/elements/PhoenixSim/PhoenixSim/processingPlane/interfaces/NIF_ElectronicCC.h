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

#ifndef NIF_ELECTRONICCC_H_
#define NIF_ELECTRONICCC_H_

#include "NIF_Photonic.h"
#include <queue>

#define INTMAX pow(2, 31)

class NIF_ElectronicCC: public NIF_Photonic {
public:
	NIF_ElectronicCC();


protected:

	virtual bool prepare();
	virtual simtime_t send();

	virtual void dataMsgArrived(cMessage* cmsg);

	ElectronicMessage* sendMsg;

};

#endif /* NIF_PHOTONIC_H_ */
