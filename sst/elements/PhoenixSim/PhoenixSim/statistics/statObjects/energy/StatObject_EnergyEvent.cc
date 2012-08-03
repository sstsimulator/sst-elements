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

#include "StatObject_EnergyEvent.h"

StatObject_EnergyEvent::StatObject_EnergyEvent(string n) : StatObject(n) {
	type = ENERGY_EVENT;
	e = 0;

}

StatObject_EnergyEvent::~StatObject_EnergyEvent() {
	// TODO Auto-generated destructor stub
}



void StatObject_EnergyEvent::stat_track(double d){

	e += d;
}

string StatObject_EnergyEvent::getHeader(){
	return "E_dynamic (J)";
}

void StatObject_EnergyEvent::writeValue(ofstream *of){
	(*of) << e;
}

double StatObject_EnergyEvent::getValue(){
	return e;
}
