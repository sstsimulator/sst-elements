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

#include "StatObject_EnergyStatic.h"

StatObject_EnergyStatic::StatObject_EnergyStatic(string n) : StatObject_Static(n) {
	type = ENERGY_STATIC;
	p = 0;
}

StatObject_EnergyStatic::~StatObject_EnergyStatic() {

}




void StatObject_EnergyStatic::stat_track(double d){

	p += d;
}

string StatObject_EnergyStatic::getHeader(){
	return "E_static (J)";
}

void StatObject_EnergyStatic::writeValue(ofstream *of){


	(*of) <<  p * getMeasuredTime();
}

double StatObject_EnergyStatic::getValue(){
	return p * getMeasuredTime();
}
