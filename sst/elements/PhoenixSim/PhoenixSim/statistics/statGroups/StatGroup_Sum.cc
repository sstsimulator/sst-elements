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

#include "StatGroup_Sum.h"

StatGroup_Sum::StatGroup_Sum(string n, int lr, int hr, string fil) :
	StatGroup(n, lr, hr, fil) {

}

StatGroup_Sum::~StatGroup_Sum() {
	// TODO Auto-generated destructor stub
}

void StatGroup_Sum::display(ofstream *st) {

	list<StatObject*>::iterator it;
	double val = 0;

	for (it = stats.begin(); it != stats.end(); it++) {
		StatObject* so = (*it);

		val += so->getValue();
	}

	(*st) << "Sum:," << val << endl;
}
