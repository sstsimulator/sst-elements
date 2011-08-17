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

#ifndef STATGROUP_H_
#define STATGROUP_H_

#include <fstream>
#include <string>
#include <sstream>
#include <list>

#include "StatObject.h"

using namespace std;

class StatGroup {
public:
	StatGroup(string n, int lr, int hr, string fil);
	virtual ~StatGroup();

	virtual void addStat(StatObject* s, string mod);

	virtual void display(ofstream *st) = 0;

	virtual bool empty();

	string name;

protected:

	list<StatObject*> stats;



	string filter;
	int highRange;
	int lowRange;
};

#endif /* STATGROUP_H_ */
