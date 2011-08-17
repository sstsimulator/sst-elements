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

#ifndef SIZEDISTRIBUTION_NORMAL_H_
#define SIZEDISTRIBUTION_NORMAL_H_

#include <omnetpp.h>
#include "SizeDistribution.h"

class SizeDistribution_Normal : public SizeDistribution {
public:
	SizeDistribution_Normal(int m, int std){mean = m; stddev = std;}
	virtual ~SizeDistribution_Normal();

	virtual int getNewSize(){return normal(mean, stddev);};

private:
	int mean;
	int stddev;
};

#endif /* SIZEDISTRIBUTION_NORMAL_H_ */
