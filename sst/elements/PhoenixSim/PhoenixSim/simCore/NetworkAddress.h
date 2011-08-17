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

#ifndef NETWORKADDRESS_H_
#define NETWORKADDRESS_H_

#include <omnetpp.h>

#include "messages_m.h"


#include <map>

using namespace std;

class NetworkAddress {
public:
	NetworkAddress(string s, int n);
	NetworkAddress(int n);
	NetworkAddress(int i, int d, int n);

	virtual ~NetworkAddress();

	NetworkAddress* dup();

	map<int, int> id;

	static void convert(string, map<int, int>* mp);

	bool equals(NetworkAddress* addr);
	int lastId();

	int numLevels;

};

#endif /* NETWORKADDRESS_H_ */
