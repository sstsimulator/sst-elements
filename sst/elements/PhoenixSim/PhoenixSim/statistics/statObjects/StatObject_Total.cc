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

#include "StatObject_Total.h"

StatObject_Total::StatObject_Total(string n) : StatObject(n) {
	type = TOTAL;
	total = 0;
}

StatObject_Total::~StatObject_Total() {
	// TODO Auto-generated destructor stub
}

void StatObject_Total::stat_track(double d) {

	total += d;

}

string StatObject_Total::getHeader() {
	return "Total";
}

void StatObject_Total::writeValue(ofstream *of) {
	(*of) << total;
}
