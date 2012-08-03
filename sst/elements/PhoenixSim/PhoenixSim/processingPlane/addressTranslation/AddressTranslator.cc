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

#include "AddressTranslator.h"


map<int, string> AddressTranslator::profile;
map<int, int> AddressTranslator::levelCounts;

int AddressTranslator::numLevels;
int AddressTranslator::concentration;

int AddressTranslator::numProcs;


AddressTranslator::AddressTranslator() {
	// TODO Auto-generated constructor stub

}

AddressTranslator::~AddressTranslator() {
	// TODO Auto-generated destructor stub
}

string AddressTranslator::untranslate_str(NetworkAddress* addr) {

	stringstream ss;

	for (int i = 0; i < addr->numLevels; i++) {
		ss << addr->id[i] << ".";
	}

	return ss.str();
}

int AddressTranslator::untranslate_pid(NetworkAddress* addr) {

	int ret = 0;

	for (int i = 0; i < addr->numLevels - 1; i++) {
		int mult = 1;
		for(int j = i + 1; j < addr->numLevels; j++){
			mult *= levelCounts[j];
		}

		ret += addr->id[i] * mult;
	}

	ret += addr->id[numLevels - 1];

	return ret;
}

void AddressTranslator::makeProfile(string levs, string prof) {


		int count = 0;

		size_t dot = prof.find(".");
		int i = 0;
		for (i = 0; dot != string::npos; i++) {
			profile[i] = prof.substr(0, dot).c_str();

			std::cout << "profile[" << i << "]: " << profile[i] << endl;

			prof = prof.substr(dot + 1);
			dot = prof.find(".");

			count++;
		}

		dot = levs.find(".");
		for (i = 0; dot != string::npos; i++) {
			levelCounts[i] = atoi(levs.substr(0, dot).c_str());

			//std::cout << "levelCounts[" << i << "]: " << levelCounts[i] << endl;

			levs = levs.substr(dot + 1);
			dot = levs.find(".");
		}

		numLevels = count;


}

int AddressTranslator::convertLevel(string l) {

	for (int i = 0; i < numLevels; i++) {
		if (l.compare(profile[i]) == 0) {
			return i;
		}
	}

	std::cout << "level: " << l << endl;

	opp_error("invalid level identifier");

}
