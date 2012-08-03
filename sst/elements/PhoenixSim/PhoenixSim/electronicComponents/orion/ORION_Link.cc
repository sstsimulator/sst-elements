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

#include "ORION_Link.h"

ORION_Link::ORION_Link() {
	// TODO Auto-generated constructor stub

}

ORION_Link::~ORION_Link() {
	// TODO Auto-generated destructor stub
}

/*-------------------------------------------------------------------------
 *                             ORION 2.0
 *
 *         					Copyright 2009
 *  	Princeton University, and Regents of the University of California
 *                         All Rights Reserved
 *
 *
 *  ORION 2.0 was developed by Bin Li at Princeton University and Kambiz Samadi at
 *  University of California, San Diego. ORION 2.0 was built on top of ORION 1.0.
 *  ORION 1.0 was developed by Hangsheng Wang, Xinping Zhu and Xuning Chen at
 *  Princeton University.
 *
 *  If your use of this software contributes to a published paper, we
 *  request that you cite our paper that appears on our website
 *  http://www.princeton.edu/~peh/orion.html
 *
 *  Permission to use, copy, and modify this software and its documentation is
 *  granted only under the following terms and conditions.  Both the
 *  above copyright notice and this permission notice must appear in all copies
 *  of the software, derivative works or modified versions, and any portions
 *  thereof, and both notices must appear in supporting documentation.
 *
 *  This software may be distributed (but not offered for sale or transferred
 *  for compensation) to third parties, provided such third parties agree to
 *  abide by the terms and conditions of this notice.
 *
 *  This software is distributed in the hope that it will be useful to the
 *  community, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "ORION_Link.h"

/* Link power and area model is only supported for 90nm, 65nm, 45nm and 32nm */

// The following function computes the wire resistance considering
// width-spacing combination and a width-dependent resistivity model
double ORION_Link::computeResistance(double Length, ORION_Tech_Config *conf) {
	double r, rho;
	int widthSpacingConfig = conf->PARM("width_spacing");

	if (widthSpacingConfig == SWIDTH_SSPACE) {
		// rho is in "ohm.m"
		rho = 2.202e-8 + (1.030e-15 / (conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")));
		r = ((rho * Length) / ((conf->PARM("WireMinWidth") - 2 * conf->PARM(
				"WireBarrierThickness")) * (conf->PARM("WireMetalThickness")
				- conf->PARM("WireBarrierThickness")))); // r is in "ohm"
	} else if (widthSpacingConfig == SWIDTH_DSPACE) {
		// rho is in "ohm.m"
		rho = 2.202e-8 + (1.030e-15 / (conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")));
		r = ((rho * Length) / ((conf->PARM("WireMinWidth") - 2 * conf->PARM(
				"WireBarrierThickness")) * (conf->PARM("WireMetalThickness")
				- conf->PARM("WireBarrierThickness")))); // r is in "ohm"
	} else if (widthSpacingConfig == DWIDTH_SSPACE) {
		// rho is in "ohm.m"
		rho = 2.202e-8 + (1.030e-15 / (2 * conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")));
		r = ((rho * Length) / ((2 * conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")) * (conf->PARM(
				"WireMetalThickness") - conf->PARM("WireBarrierThickness")))); // r is in "ohm"
	} else if (widthSpacingConfig == DWIDTH_DSPACE) {
		// rho is in "ohm.m"

		rho = 2.202e-8 + (1.030e-15 / (2 * conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")));
		r = ((rho * Length) / ((2 * conf->PARM("WireMinWidth") - 2
				* conf->PARM("WireBarrierThickness")) * (conf->PARM(
				"WireMetalThickness") - conf->PARM("WireBarrierThickness")))); // r is in "ohm"

	} else {
		fprintf(stderr,
				"ERROR, Specified width spacing configuration is not supported.\n");
		exit(1);
	}


	return r;
}

// COMPUTE WIRE CAPACITANCE using PTM models
// The parameter "Length" has units of "m"
double ORION_Link::computeGroundCapacitance(double Length,
		ORION_Tech_Config *conf) {
	double c_g;
	int widthSpacingConfig = conf->PARM("width_spacing");

	if (widthSpacingConfig == SWIDTH_SSPACE) {
		double A = (conf->PARM("WireMinWidth") / conf->PARM(
				"WireDielectricThickness"));
		double B = 2.04 * pow((conf->PARM("WireMinSpacing") / (conf->PARM(
				"WireMinSpacing") + 0.54
				* conf->PARM("WireDielectricThickness"))), 1.77);
		double C = pow((conf->PARM("WireMetalThickness") / (conf->PARM(
				"WireMetalThickness") + 4.53 * conf->PARM(
				"WireDielectricThickness"))), 0.07);
		c_g = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length; // c_g is in "F"
	} else if (widthSpacingConfig == SWIDTH_DSPACE) {
		double minSpacingNew = 2 * conf->PARM("WireMinSpacing") + conf->PARM(
				"WireMinWidth");
		double A = (conf->PARM("WireMinWidth") / conf->PARM(
				"WireDielectricThickness"));
		double B = 2.04 * pow((minSpacingNew / (minSpacingNew + 0.54
				* conf->PARM("WireDielectricThickness"))), 1.77);
		double C = pow((conf->PARM("WireMetalThickness") / (conf->PARM(
				"WireMetalThickness") + 4.53 * conf->PARM(
				"WireDielectricThickness"))), 0.07);
		c_g = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length; // c_g is in "F"
	} else if (widthSpacingConfig == DWIDTH_SSPACE) {
		double minWidthNew = 2 * conf->PARM("WireMinWidth");
		double A = (minWidthNew / conf->PARM("WireDielectricThickness"));
		double B = 2.04 * pow((conf->PARM("WireMinSpacing") / (conf->PARM(
				"WireMinSpacing") + 0.54
				* conf->PARM("WireDielectricThickness"))), 1.77);
		double C = pow((conf->PARM("WireMetalThickness") / (conf->PARM(
				"WireMetalThickness") + 4.53 * conf->PARM(
				"WireDielectricThickness"))), 0.07);
		c_g = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length; // c_g is in "F"
	} else if (widthSpacingConfig == DWIDTH_DSPACE) {
		double minWidthNew = 2 * conf->PARM("WireMinWidth");
		double minSpacingNew = 2 * conf->PARM("WireMinSpacing");
		double A = (minWidthNew / conf->PARM("WireDielectricThickness"));
		double B = 2.04 * pow((minSpacingNew / (minSpacingNew + 0.54
				* conf->PARM("WireDielectricThickness"))), 1.77);
		double C = pow((conf->PARM("WireMetalThickness") / (conf->PARM(
				"WireMetalThickness") + 4.53 * conf->PARM(
				"WireDielectricThickness"))), 0.07);
		c_g = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length; // c_g is in "F"
	} else {
		fprintf(stderr,
				"ERROR, Specified width spacing configuration is not supported.\n");
		exit(1);
	}

	return c_g; // Ground capacitance is in "F"
}

// Computes the coupling capacitance considering cross-talk
// The parameter "Length" has units of "m"
double ORION_Link::computeCouplingCapacitance(double Length,
		ORION_Tech_Config *conf) {
	double c_c;
	int widthSpacingConfig = conf->PARM("width_spacing");

	if (widthSpacingConfig == SWIDTH_SSPACE) {
		double A = 1.14 * (conf->PARM("WireMetalThickness") / conf->PARM(
				"WireMinSpacing")) * exp(-4 * conf->PARM("WireMinSpacing")
				/ (conf->PARM("WireMinSpacing") + 8.01 * conf->PARM(
						"WireDielectricThickness")));
		double B = 2.37 * pow((conf->PARM("WireMinWidth") / (conf->PARM(
				"WireMinWidth") + 0.31 * conf->PARM("WireMinSpacing"))), 0.28);
		double C = pow((conf->PARM("WireDielectricThickness") / (conf->PARM(
				"WireDielectricThickness") + 8.96
				* conf->PARM("WireMinSpacing"))), 0.76) * exp(-2 * conf->PARM(
				"WireMinSpacing") / (conf->PARM("WireMinSpacing") + 6
				* conf->PARM("WireDielectricThickness")));
		c_c = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length;
	} else if (widthSpacingConfig == SWIDTH_DSPACE) {
		double minSpacingNew = 2 * conf->PARM("WireMinSpacing") + conf->PARM(
				"WireMinWidth");
		double A = 1.14 * (conf->PARM("WireMetalThickness") / minSpacingNew)
				* exp(-4 * minSpacingNew / (minSpacingNew + 8.01 * conf->PARM(
						"WireDielectricThickness")));
		double B = 2.37 * pow((conf->PARM("WireMinWidth") / (conf->PARM(
				"WireMinWidth") + 0.31 * minSpacingNew)), 0.28);
		double C = pow((conf->PARM("WireDielectricThickness") / (conf->PARM(
				"WireDielectricThickness") + 8.96 * minSpacingNew)), 0.76)
				* exp(-2 * minSpacingNew / (minSpacingNew + 6 * conf->PARM(
						"WireDielectricThickness")));
		c_c = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length;
	} else if (widthSpacingConfig == DWIDTH_SSPACE) {
		double minWidthNew = 2 * conf->PARM("WireMinWidth");
		double A = 1.14 * (conf->PARM("WireMetalThickness") / conf->PARM(
				"WireMinSpacing")) * exp(-4 * conf->PARM("WireMinSpacing")
				/ (conf->PARM("WireMinSpacing") + 8.01 * conf->PARM(
						"WireDielectricThickness")));
		double B = 2.37 * pow((2 * minWidthNew / (2 * minWidthNew + 0.31
				* conf->PARM("WireMinSpacing"))), 0.28);
		double C = pow((conf->PARM("WireDielectricThickness") / (conf->PARM(
				"WireDielectricThickness") + 8.96
				* conf->PARM("WireMinSpacing"))), 0.76) * exp(-2 * conf->PARM(
				"WireMinSpacing") / (conf->PARM("WireMinSpacing") + 6
				* conf->PARM("WireDielectricThickness")));
		c_c = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length;
	} else if (widthSpacingConfig == DWIDTH_DSPACE) {
		double minWidthNew = 2 * conf->PARM("WireMinWidth");
		double minSpacingNew = 2 * conf->PARM("WireMinSpacing");
		double A = 1.14 * (conf->PARM("WireMetalThickness") / minSpacingNew)
				* exp(-4 * minSpacingNew / (minSpacingNew + 8.01 * conf->PARM(
						"WireDielectricThickness")));
		double B = 2.37 * pow((minWidthNew / (minWidthNew + 0.31
				* minSpacingNew)), 0.28);
		double C = pow((conf->PARM("WireDielectricThickness") / (conf->PARM(
				"WireDielectricThickness") + 8.96 * minSpacingNew)), 0.76)
				* exp(-2 * minSpacingNew / (minSpacingNew + 6 * conf->PARM(
						"WireDielectricThickness")));
		c_c = conf->PARM("WireDielectricConstant") * 8.85e-12 * (A + (B * C))
				* Length;
	} else {
		fprintf(stderr,
				"ERROR, Specified width spacing configuration is not supported.\n");
		exit(1);
	}

	return c_c; // Coupling capacitance is in "F"
}

// OPTIMUM K and H CALCULATION
// Computes the optimum number and size of repeaters for the link

void ORION_Link::getOptBuffering(int *k, double *h, double Length,
		ORION_Tech_Config *conf) {
	double r;
	double c_g;
	double c_c;
	if (conf->PARM("buffering_scheme") == MIN_DELAY) {
		if (conf->PARM("shielding")) {
			r = computeResistance(Length, conf);
			c_g = 2 * computeGroundCapacitance(Length, conf);
			c_c = 2 * computeCouplingCapacitance(Length, conf);

			*k = (int) sqrt(((0.4 * r * c_g) + (0.57 * r * c_c)) / (0.7
					* conf->PARM("BufferDriveResistance") * conf->PARM(
					"BufferInputCapacitance"))); //k is the number of buffers to be inserted
			*h = sqrt(((0.7 * conf->PARM("BufferDriveResistance") * c_g) + (1.4
					* 1.5 * conf->PARM("BufferDriveResistance") * c_c)) / (0.7
					* r * conf->PARM("BufferInputCapacitance"))); //the size of the buffers to be inserted
		} else {
			r = computeResistance(Length, conf);

			c_g = 2 * computeGroundCapacitance(Length, conf);
			c_c = 2 * computeCouplingCapacitance(Length, conf);

			*k = (int) sqrt(((0.4 * r * c_g) + (1.51 * r * c_c)) / (0.7
					* conf->PARM("BufferDriveResistance") * conf->PARM(
					"BufferInputCapacitance")));
			*h = sqrt(((0.7 * conf->PARM("BufferDriveResistance") * c_g) + (1.4
					* 2.2 * conf->PARM("BufferDriveResistance") * c_c)) / (0.7
					* r * conf->PARM("BufferInputCapacitance")));

		}
	} else if (conf->PARM("buffering_scheme") == STAGGERED) {
		r = computeResistance(Length, conf);
		c_g = 2 * computeGroundCapacitance(Length, conf);
		c_c = 2 * computeCouplingCapacitance(Length, conf);

		*k = (int) sqrt(((0.4 * r * c_g) + (0.57 * r * c_c)) / (0.7
				* conf->PARM("BufferDriveResistance") * conf->PARM(
				"BufferInputCapacitance")));
		*h = sqrt(((0.7 * conf->PARM("BufferDriveResistance") * c_g) + (1.4
				* 1.5 * conf->PARM("BufferDriveResistance") * c_c)) / (0.7 * r
				* conf->PARM("BufferInputCapacitance")));
	} else {
		fprintf(stderr, "ERROR, Specified buffering scheme is not supported.\n");
		exit(1);
	}
}

// unit will be joule/bit/meter
double ORION_Link::LinkDynamicEnergyPerBitPerMeter(double Length,
		ORION_Tech_Config *conf) {
	double cG = 2 * computeGroundCapacitance(Length, conf);
	double cC = 2 * computeCouplingCapacitance(Length, conf);
	double totalWireC = cC + cG; // for dyn power
	int k;
	double h;
	int *ptr_k = &k;
	double *ptr_h = &h;

	getOptBuffering(ptr_k, ptr_h, Length, conf);
	double bufferC = ((double) k) * (conf->PARM("BufferInputCapacitance")) * h;

	return (((totalWireC + bufferC) * conf->PARM("Vdd") * conf->PARM("Vdd"))
			/ Length);
}

// unit will be Watt/meter
double ORION_Link::LinkLeakagePowerPerMeter(double Length,
		ORION_Tech_Config *conf) {
	int k;
	double h;
	int *ptr_k = &k;
	double *ptr_h = &h;
	getOptBuffering(ptr_k, ptr_h, Length, conf);

	double pmosLeakage = conf->PARM("BufferPMOSOffCurrent") * h * k;
	double nmosLeakage = conf->PARM("BufferNMOSOffCurrent") * h * k;
	return ((conf->PARM("Vdd") * (pmosLeakage + nmosLeakage) / 2) / Length);
}

double ORION_Link::LinkArea(double Length, unsigned NumBits,
		ORION_Tech_Config *conf) {
	// Link area has units of "um^2"
	double deviceArea = 0, routingArea = 0;
	int k;
	double h;
	int *ptr_k = &k;
	double *ptr_h = &h;
	getOptBuffering(ptr_k, ptr_h, Length, conf);
	if (conf->PARM("TECH_POINT") == 90)
		deviceArea = NumBits * k * ((h * 0.43) + 1.38);
	else if (conf->PARM("TECH_POINT") <= 65)
		deviceArea = (NumBits) * k * ((h * 0.45) + 0.65)
				* conf->PARM("SCALE_T");

	routingArea = (NumBits * (conf->PARM("WireMinWidth") * 1e6 + conf->PARM(
			"WireMinSpacing") * 1e6) + conf->PARM("WireMinSpacing") * 1e6)
			* Length * 1e6;
	return deviceArea + routingArea;
}

