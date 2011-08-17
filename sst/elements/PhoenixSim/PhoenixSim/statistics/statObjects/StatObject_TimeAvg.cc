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

#include "StatObject_TimeAvg.h"



StatObject_TimeAvg::StatObject_TimeAvg(string n) : StatObject(n) {
	name = n;
	type = TIME_AVG;

	total = 0;

}

StatObject_TimeAvg::~StatObject_TimeAvg() {
	// TODO Auto-generated destructor stub
}


void StatObject_TimeAvg::stat_track(double d){

	total += d;
}

string StatObject_TimeAvg::getHeader(){
	return "TimeAvg";
}

void StatObject_TimeAvg::writeValue(ofstream* of){
	(*of) << total / getMeasuredTime();

}

double StatObject_TimeAvg::getValue(){
	return total / getMeasuredTime();
}
