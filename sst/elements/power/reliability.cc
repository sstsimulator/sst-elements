// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "reliability.h"


namespace SST {

double Reliability::calc_EM(double t_in) {
        double lambdaEM;
        lambdaEM= 1/(A_EM * exp(0.7/(8.62 * pow(10,-5) * t_in)));
        return lambdaEM;
}


double Reliability::calc_TDDB(double t_in) {
        double lambdaTDDB;
        lambdaTDDB= 1/(A_TDDB * exp(0.75/(8.62 * pow(10,-5) * t_in)));
        return lambdaTDDB;
}

double Reliability::calc_TC(double t_max, double t_min, double t_avg, double freq) {
        double lambdaTC;

        lambdaTC=Co* pow((t_max-t_min),Q)* freq;
        if (t_avg==0)
                lambdaTC=0;
        return lambdaTC;
}

        
double Reliability::compute_failurerate(double temp, double t_min, double t_max, double t_avg, double freq, /*power_state sleepstate*/bool ifSleep) {
        double l_total, l_EM, l_TDDB, l_TC;

        l_EM=calc_EM(temp);
        //if (sleepstate==SLEEP)
	if (ifSleep==true)
                l_EM=0;
        l_TDDB=calc_TDDB(temp);
        l_TC=calc_TC(t_max, t_min, t_avg, freq);
        l_total=l_EM + l_TDDB + l_TC;
        return l_total; 
}


} // namespace SST
