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

#ifndef SIZEDISTRIBUTION_CONTROL_H_
#define SIZEDISTRIBUTION_CONTROL_H_

#include <omnetpp.h>
#include "SizeDistribution.h"

class SizeDistribution_Control : public SizeDistribution {
public:
	SizeDistribution_Control(int sm, double smP, int lg, int lgdev){small = sm; smallP = smP; largemean = lg; largedev = lgdev;}
	virtual ~SizeDistribution_Control();

	virtual int getNewSize(){if(uniform(0, 1) < smallP){ return small; } else { return (int)normal(largemean, largedev);}}
private:
	double smallP;
	int small;
	int largemean;
	int largedev;
};

#endif /* SIZEDISTRIBUTION_CONTROL_H_ */
