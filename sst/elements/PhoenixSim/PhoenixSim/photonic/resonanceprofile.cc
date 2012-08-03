/*
 * resonanceprofile.cc
 *
 *  Created on: Mar 25, 2009
 *      Author: johnnie
 */

#include "resonanceprofile.h"

ResonanceProfile::ResonanceProfile()
{
	setTmax(1);
	setFSR(1);
	setFinesse(1000000);
	setIndex(1);
	setOrder(1);
}

ResonanceProfile::ResonanceProfile(double tmax, double fsr, double finesse, double index, double order)
{
	setTmax(tmax);
	setFSR(fsr);
	setFinesse(finesse);
	setIndex(index);
	setOrder(order);
}

void ResonanceProfile::setTmax(double tmax)
{
	myTmax = tmax;
}

void ResonanceProfile::setFSR(double fsr)
{
	myFSR = fsr;
}

void ResonanceProfile::setFinesse(double finesse)
{
	myFinesse = finesse;
}

void ResonanceProfile::setIndex(double index)
{
	myIndex = index;
}

void ResonanceProfile::setOrder(double order)
{
	myOrder = order;
}

double ResonanceProfile::transmission(double frequency)
{
	double value = 1 + pow(2*myFinesse/M_PI,2)*pow(sin(M_PI*frequency/myFSR),2);
	value = myTmax/value;
	return value;
}

double ResonanceProfile::transmissionW(double wavelength)
{
	return transmission(3e8/(myIndex*wavelength));
}
