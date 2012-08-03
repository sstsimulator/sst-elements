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

#include "StatGroup.h"

StatGroup::StatGroup(string n, int lr, int hr, string fil) {
	name = n;

	lowRange = lr;
	highRange = hr;

	filter = fil;

}

StatGroup::~StatGroup() {
	stats.clear();
}


void StatGroup::addStat(StatObject* s, string mod){

	if(s->type >= lowRange && s->type <= highRange && (filter.compare("") != 0 ? mod.find(filter) != string::npos : true)){
		//std::cout << "group: " << name << ", filter " << filter << ", added stat " << s->name << ", type " << s->type << ", mod " << mod << endl;
		stats.push_back(s);
	}
}

bool StatGroup::empty(){
	return stats.size() == 0;
}
