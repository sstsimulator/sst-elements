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

#ifndef STATOBJECT_ENERGYSTATIC_H_
#define STATOBJECT_ENERGYSTATIC_H_

#include <omnetpp.h>

#include "StatObject.h"

class StatObject_EnergyStatic : public StatObject_Static {
public:
	StatObject_EnergyStatic(string n);
	virtual ~StatObject_EnergyStatic();

	virtual void stat_track(double d);

	virtual string getHeader();
	virtual void writeValue(ofstream *of);

	virtual double getValue();

private:
	double p;
};

#endif /* STATOBJECT_ENERGYSTATIC_H_ */
