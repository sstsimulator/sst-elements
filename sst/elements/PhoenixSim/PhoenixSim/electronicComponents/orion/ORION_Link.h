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

#ifndef ORION_LINK_H_
#define ORION_LINK_H_

#include "ORION_Config.h"



class ORION_Link {
public:
	ORION_Link();
	virtual ~ORION_Link();

	static double computeResistance(double Length, ORION_Tech_Config *conf);
	static double computeGroundCapacitance(double Length, ORION_Tech_Config *conf);
	static double computeCouplingCapacitance(double Length, ORION_Tech_Config *conf);
	static void getOptBuffering(int *k , double *h , double Length, ORION_Tech_Config *conf);
	static double LinkDynamicEnergyPerBitPerMeter(double Length,  ORION_Tech_Config *conf);
	static double LinkLeakagePowerPerMeter(double Length,  ORION_Tech_Config *conf);
	static double LinkArea(double Length, unsigned NumBits, ORION_Tech_Config *conf);

};

#endif /* ORION_LINK_H_ */
