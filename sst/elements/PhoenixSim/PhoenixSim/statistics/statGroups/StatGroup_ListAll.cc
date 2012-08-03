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

#include "StatGroup_ListAll.h"

StatGroup_ListAll::StatGroup_ListAll(string n, int lr, int hr, string fil) : StatGroup(n, lr, hr, fil) {


}

StatGroup_ListAll::~StatGroup_ListAll() {
	// TODO Auto-generated destructor stub
}


bool compareStatObjectPointer(StatObject* first, StatObject* second)
{
	return first->type < second->type;
}

void StatGroup_ListAll::display(ofstream *st) {

	//first, group them by component name
	map<string, list<StatObject*>*> objs;

	list<StatObject*>::iterator it;

	for (it = stats.begin(); it != stats.end(); it++) {
		StatObject* so = (*it);

		if (objs.find(so->name) == objs.end()) {
			objs[so->name] = new list<StatObject*> ();

		}
		objs[so->name]->push_back(so);
	}

	//sort them by type
	map<string, list<StatObject*>*>::iterator it2;

	for (it2 = objs.begin(); it2 != objs.end(); it2++) {

		it2->second->sort(compareStatObjectPointer);
	}


	//display the headers

	list<StatObject*> *first = (objs.begin())->second;

	(*st) << ",";
	for (it = first->begin(); it != first->end(); it++) {
		StatObject* so = (*it);

		(*st) << so->getHeader() << ",";

	}

	(*st) << "\n";

	//display values

	for (it2 = objs.begin(); it2 != objs.end(); it2++) {
		(*st) << it2->first << ",";
		for (it = it2->second->begin(); it != it2->second->end(); it++) {
			StatObject* so = (*it);

			so->writeValue(st);
			(*st) << ",";
		}

		(*st) << "\n";

	}
}
