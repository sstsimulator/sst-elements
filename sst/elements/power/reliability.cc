// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization.h"

#include "reliability.h"


namespace SST {
namespace Power {

double Reliability::calc_EM(double t_in) {
        double lambdaEM;
        lambdaEM= 1/(A_EM * exp(0.7/(0.0000862 * t_in)));
        return lambdaEM;
}


double Reliability::calc_TDDB(double t_in) {
        double lambdaTDDB;
        lambdaTDDB= 1/(A_TDDB * exp(0.75/(0.0000862 * t_in)));
        return lambdaTDDB;
}

double Reliability::calc_TC(double t_max, double t_min, double t_avg, double freq) {
        double lambdaTC;

        lambdaTC=Co* pow((t_max-t_min),Q)* freq;
        if (t_avg==0)
                lambdaTC=0;
        ////return lambdaTC;  commented for now since cyling freq is still an issue
	return (0);
}

double Reliability::calc_NBTI(double t_in) {
        double lambdaNBTI;
	double A = 1.6328;
	double B = 0.07377;
	double C = 0.01;
	double D = -0.06852;
	double beta = 0.3;
	double k = 0.0000862;
	double itemA = log2(A/(1+(2*exp(B/(k*t_in)))));
	double itemB = log2((A/(1+(2*exp(B/(k*t_in))))) - C);
	double itemC = t_in/exp((-D)/(k*t_in));

	lambdaNBTI = 1/(A_NBTI * pow((itemA-itemB) * itemC, (1/beta)));

        //lambdaNBTI=1/(A_NBTI * pow((log2(1.6328/(1+2*exp(0.07377/(8.62 * pow(10,-5) * t_in)))) - 
	//			log2((1.6328/(1+2*exp(0.07377/(8.62 * pow(10,-5) * t_in)))-0.01)) *
	//			(t_in/exp(0.06852/(8.62 * pow(10,-5) * t_in)))), 3.33) );
       
        return lambdaNBTI;
}

double Reliability::generate_TTF(double frate)
{ 
	double TTF;
	srand( (unsigned)time( NULL ) );
 	double r1;
	double r2;

	r1 = rand()/(double)RAND_MAX;
	r2 = rand()/(double)RAND_MAX;

	TTF = exp( -log2(frate) - 0.125 + (0.5*sqrt(-2*log2(r1))*sin(2*3.1415926*r2)) );

	return TTF;
}
        
double Reliability::compute_localMinTTF(double temp, double t_min, double t_max, double t_avg, double freq, /*power_state sleepstate*/bool ifSleep) {
        double l_EM, l_TDDB, l_TC, l_NBTI;
	double TTF_EM, TTF_TDDB, TTF_TC, TTF_NBTI; 
	double minTTF =99999;

        l_EM=calc_EM(temp);
        //if (sleepstate==SLEEP)
	if (ifSleep==true)
                l_EM=0;
        l_TDDB=calc_TDDB(temp);
        l_TC=calc_TC(t_max, t_min, t_avg, freq);
	l_NBTI = calc_NBTI(temp);

	if (l_EM == 0)
	    TTF_EM = 99999;
	else
	    TTF_EM = generate_TTF(l_EM);

	TTF_TDDB = generate_TTF(l_TDDB);

	if(l_TC == 0)
	    TTF_TC = 99999;
	else
	    TTF_TC = generate_TTF(l_TC);

	TTF_NBTI = generate_TTF(l_NBTI);

	////std::cout << "TTF_EM = " << TTF_EM << ", TTF_TDDB = " << TTF_TDDB << ", TTF_TC = " << TTF_TC << ", TTF_NBTI = " << TTF_NBTI << std::endl;

	if (minTTF > TTF_EM)
	    minTTF = TTF_EM;
	if (minTTF > TTF_TDDB)
	    minTTF = TTF_TDDB;
	if (minTTF > TTF_TC)
	    minTTF = TTF_TC;
	if (minTTF > TTF_NBTI)
	    minTTF = TTF_NBTI;
	

        return minTTF; 
}


} // Namespace Power
} // namespace SST
