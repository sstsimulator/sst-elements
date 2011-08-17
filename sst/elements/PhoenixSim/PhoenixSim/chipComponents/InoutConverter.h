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

#ifndef INOUTCONVERTER_H_
#define INOUTCONVERTER_H_

#include <omnetpp.h>

using namespace std;

class InoutConverter : public cSimpleModule {
public:
	InoutConverter();
	virtual ~InoutConverter();

protected:
	void initialize();
	void handleMessage(cMessage* msg);
	void finish();

	cGate* biIn;
	cGate* biOut;
	cGate* uniIn;
	cGate* uniOut;
};

#endif /* INOUTCONVERTER_H_ */
