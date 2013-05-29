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

#include "StatObject.h"

double StatObject::warmup = 0;
double StatObject::cooldown = 0;

StatObject::StatObject(string n) {
	name = n;

}

StatObject::~StatObject() {
	// TODO Auto-generated destructor stub
}

bool StatObject::operator<(const StatObject &s1)
{
  
	std::cout<<type;
	std::cout<<" is ";
//	std::cout<<(type < (s1.type))?("less than "):("greater than ");  // NOTE: RECODED TO AVOID COMPILE WARNINGS
	if (type < s1.type){
	  std::cout<<"less than ";
	}
	else{
	  std::cout<<"greater than ";
	}
	std::cout<<(s1.type)<<endl;
	return type < s1.type;

}

void StatObject::track(double d) {
	double t= SIMTIME_DBL(simTime());

	if (t > warmup && t < cooldown) {
		stat_track(d);
	}
}

double StatObject::getMeasuredTime() {

	double t = SIMTIME_DBL(simTime());
	double t_meas = min(t, cooldown) - max(0.0, warmup);

	if (t_meas < 0) {
		opp_error(
				"ERROR: t_meas is less than zero.  STAT_warmup and STAT_cooldown should be set appropriately.");
	}

	return t_meas;
}

void StatObject_Static::track(double d) {

	stat_track(d);
}

bool StatObject::equals(StatObject* so) {

	return so->type == type && (so->name).compare(name) == 0;
}

double StatObject::getValue() {
	std::cout << "******name: " << name << ", type: " << type << endl;
	opp_error("StatObject: getValue() not implemented");

  return 0; // Added to avoid Compile Warning
}
