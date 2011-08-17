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

#include "StatObject_MMA.h"

StatObject_MMA::StatObject_MMA(string n) :
	StatObject(n) {
	type = MMA;

	min = 0;
	max = 0;
	total = 0;
	count = 0;

	first = true;
}

StatObject_MMA::~StatObject_MMA() {
	// TODO Auto-generated destructor stub
}

void StatObject_MMA::stat_track(double d) {

	if (first) {
		min = d;
		max = d;
		first = false;
	} else {

		min = (d < min) ? d : min;
		max = (d > max) ? d : max;
	}
	total += d;
	count++;

	points.push_back(d);
}

string StatObject_MMA::getHeader() {
	return "Count,Min,Avg,Max,StdDev";
}

void StatObject_MMA::writeValue(ofstream *of) {
	double stddev = 0;
	list<double>::iterator iter;
	double avg = total / count;
	for(iter = points.begin(); iter != points.end(); iter++){
		stddev += pow((avg - *iter), 2);
	}
	stddev = stddev / count;
	stddev = sqrt(stddev);

	(*of) << count << "," << min << "," << avg << "," << max << "," << stddev;
}
