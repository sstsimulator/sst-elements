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

#ifndef STATOBJECT_H_
#define STATOBJECT_H_

#include <omnetpp.h>

#include <fstream>
#include <sstream>

using namespace std;

class StatObject {
public:
	StatObject(string n);
	virtual ~StatObject();

	virtual void track(double d);

	virtual void stat_track(double d) = 0;

	//display stuff
	virtual string getHeader() = 0;
	virtual void writeValue(ofstream *of) = 0;

	virtual bool equals(StatObject* so);

	virtual double getValue();

	enum STAT_TYPES {
		BEGIN = 0,
		COUNT,    //counts the number of times one is instantiated
		MMA,          //min, max, avg
		TIME_AVG,     //takes the average value over the whole time
		TOTAL,        //just counts the total from track(..)

		ENERGY_STATIC = 100,
		ENERGY_EVENT,
		ENERGY_ON,
		ENERGY_LAST,        //this one is for comparison, so you know where the
							//energy types end

		END


	};

	bool operator<(const StatObject &s1);

	static double warmup;
	static double cooldown;

	static double getMeasuredTime();

	int type;
	string name;
};

class StatObject_Static : public StatObject {
public:
	StatObject_Static(string n) : StatObject(n) {};

	virtual void track(double d);
};

#endif /* STATOBJECT_H_ */
