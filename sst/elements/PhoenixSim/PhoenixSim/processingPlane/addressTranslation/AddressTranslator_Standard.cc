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

#include "AddressTranslator_Standard.h"

AddressTranslator_Standard::AddressTranslator_Standard() {
	// TODO Auto-generated constructor stub

}

AddressTranslator_Standard::~AddressTranslator_Standard() {
	// TODO Auto-generated destructor stub
}

NetworkAddress* AddressTranslator_Standard::translateAddr(
		ApplicationData* adata) {

	stringstream ss;

	for (int i = 0; i < numLevels; i++) {

		if (profile[i].compare(DRAM_SLOT) == 0) {
			switch (adata->getType()) {
			case SM_read:
			case SM_write:
			case DM_readLocal:
			case DM_readRemote:
			case DM_writeLocal:
			case DM_writeRemote: {
				ss << "1.";
				break;
			}

			default: {
				ss << "0.";
				break;
			}
			}
		} else if (profile[i].compare(NET_SLOT) == 0) {
			ss << adata->getDestId() / concentration << ".";
		} else if (profile[i].compare(PROC_SLOT) == 0) {
			ss << adata->getDestId() % concentration << ".";
		}

	}

	return new NetworkAddress(ss.str(), numLevels);
}

