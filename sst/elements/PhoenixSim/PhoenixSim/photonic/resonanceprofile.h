/*
 * resonanceprofile.h
 *
 *  Created on: Mar 25, 2009
 *      Author: johnnie
 */

#ifndef _RESONANCEPROFILE_H_
#define _RESONANCEPROFILE_H_

#include <math.h>

class ResonanceProfile
{
public:
	ResonanceProfile();
	ResonanceProfile(double tmax, double fsr, double finesse, double index, double order);

	void setTmax(double tmax);
	void setFSR(double fsr);
	void setFinesse(double finesse);
	void setIndex(double index);
	void setOrder(double order);

	double getTmax() {return myTmax;}
	double getFSR() {return myFSR;}
	double getFinesse() {return myFinesse;}
	double getIndex() {return myIndex;}
	double getOrder() {return myOrder;}

	double transmission(double frequency);
	double transmissionW(double wavelength);

private:
	double myTmax;
	double myFSR;
	double myFinesse;
	double myIndex;
	double myOrder;
};

#endif /* _RESONANCEPROFILE_H_ */
