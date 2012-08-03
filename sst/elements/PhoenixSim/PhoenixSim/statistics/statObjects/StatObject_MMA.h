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

#ifndef STATOBJECT_MMA_H_
#define STATOBJECT_MMA_H_

#include "StatObject.h"

class StatObject_MMA: public StatObject {
public:
	StatObject_MMA(string n);
	virtual ~StatObject_MMA();

	virtual void stat_track(double d);

	virtual string getHeader();
	virtual void writeValue(ofstream *of);

protected:
	double min;
	double max;
	double total;
	double count;

	list<double> points;

	bool first;
};

#endif /* STATOBJECT_MMA_H_ */
