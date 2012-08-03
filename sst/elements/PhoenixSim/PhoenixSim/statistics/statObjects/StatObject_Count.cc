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

#include "StatObject_Count.h"

map<string, int> StatObject_Count::cnts;

StatObject_Count::StatObject_Count(string n) : StatObject(n) {
	name = n;
	type = COUNT;

	if(cnts.find(name) == cnts.end()){
		cnts[name] = 1;
	}else{
		cnts[name]++;
	}

}

StatObject_Count::~StatObject_Count() {
	// TODO Auto-generated destructor stub
}


void StatObject_Count::stat_track(double d){


}

string StatObject_Count::getHeader(){
	return "Count";
}

void StatObject_Count::writeValue(ofstream* of){
	(*of) << cnts[name];

}

double StatObject_Count::getValue(){
	return cnts[name];
}
