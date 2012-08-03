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

#ifndef ADDRESSTRANSLATOR_H_
#define ADDRESSTRANSLATOR_H_

#include "NetworkAddress.h"


using namespace std;

class AddressTranslator {
public:
	AddressTranslator();
	virtual ~AddressTranslator();

	virtual NetworkAddress* translateAddr(ApplicationData* adata) = 0;

	virtual int untranslate_pid(NetworkAddress* addr);
	static string untranslate_str(NetworkAddress* addr);

	static int convertLevel(string l);

	static void makeProfile(string levs, string prof);

	static map<int, string> profile;
	static map<int, int> levelCounts;

	static int numLevels;
	static int concentration;

	static int numProcs;
};

#endif /* ADDRESSTRANSLATOR_H_ */
