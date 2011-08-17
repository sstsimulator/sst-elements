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

#include "NetworkAddress.h"

NetworkAddress::NetworkAddress(string s, int n) {
	numLevels = n;

	if (n < 0 || n > 10) {
		opp_error("invalid number of network address levels: " + n);
	}

	for (int j = 0; j < numLevels; j++) {
		id[j] = -1;
	}

	convert(s, &id);

}

NetworkAddress::NetworkAddress(int n) {
	numLevels = n;

	if (n < 0 || n > 10) {
		opp_error("invalid number of network address levels: " + n);
	}

	for (int j = 0; j < numLevels; j++) {
		id[j] = -1;
	}

}

NetworkAddress::NetworkAddress(int i, int d, int n) {
	numLevels = n;

	if (n < 0 || n > 10) {
		opp_error("invalid number of network address levels: " + n);
	}

	for (int j = 0; j < numLevels; j++) {
		id[j] = -1;
	}

	id[0] = i;
	id[1] = d;

}

NetworkAddress::~NetworkAddress() {
	// TODO Auto-generated destructor stub
}

void NetworkAddress::convert(string s, map<int, int>* mp) {
	size_t dot = s.find(".");

	for (int i = 0; dot != string::npos; i++) {
		(*mp)[i] = atoi(s.substr(0, dot).c_str());

		s = s.substr(dot + 1);
		dot = s.find(".");
	}

}

NetworkAddress* NetworkAddress::dup() {
	NetworkAddress* ret = new NetworkAddress(numLevels);

	for (int i = 0; i < numLevels; i++) {
		ret->id[i] = this->id[i];
	}

	return ret;

}

bool NetworkAddress::equals(NetworkAddress* addr) {
	for (int i = 0; i < numLevels; i++) {
		if (id[i] != addr->id[i])
			return false;
	}

	return true;
}

int NetworkAddress::lastId() {

	return id[numLevels - 1];
}

