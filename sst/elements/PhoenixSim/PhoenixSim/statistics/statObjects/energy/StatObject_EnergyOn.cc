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

#include "StatObject_EnergyOn.h"

StatObject_EnergyOn::StatObject_EnergyOn(string n) : StatObject(n) {
	type = ENERGY_ON;
	p = 0;

}

StatObject_EnergyOn::~StatObject_EnergyOn() {
	// TODO Auto-generated destructor stub
}



void StatObject_EnergyOn::stat_track(double d){

	p += d;
}

string StatObject_EnergyOn::getHeader(){
	return "E_On (J)";
}

void StatObject_EnergyOn::writeValue(ofstream *of){
	(*of) << p;
}

double StatObject_EnergyOn::getValue(){
	return p;
}
