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

#ifndef SIZEDISTRIBUTION_GAMMA_H_
#define SIZEDISTRIBUTION_GAMMA_H_

#include <omnetpp.h>
#include "SizeDistribution.h"

class SizeDistribution_Gamma : public SizeDistribution {
public:
	SizeDistribution_Gamma(int mi, int ma){minimum = mi; maximum = ma;};
	virtual ~SizeDistribution_Gamma();

	virtual int getNewSize(){return (int)(gamma_d(1.5, 7) * maximum / 30.0) + minimum;}

protected:
	int minimum;
	int maximum;

};

#endif /* SIZEDISTRIBUTION_GAMMA_H_ */
