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

#ifndef _SST_RELIABILITY_H
#define _SST_RELIABILITY_H


#include <stdio.h>			
#include <math.h>
#include <string>
#include <time.h>
#include <sst/core/debug.h>
#include <sst/core/sst_types.h>



namespace SST {

    static double Q= 4;
    static double Co= 1.52e-5;
    static double A_EM=7.39125e-10;
    static double A_TDDB=1.36334e-10;
    static double A_NBTI=0.006;
    //enum power_state{ACTIVE, IDLE, SLEEP};

     class Reliability {
    public:
        Reliability(){}
        ~Reliability(){}

        double calc_EM(double t_in); 
	double calc_TDDB(double t_in);
	double calc_TC(double t_max, double t_min, double t_avg, double freq);  
	double calc_NBTI(double t_in);
	double generate_TTF(double mttf);
	double compute_localMinTTF(double temp, double t_min, double t_max, double t_avg, double freq, bool ifSleep);
       
    private:
        std::string type;
        std::string filename;

   friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    };



}
#endif // SST_RELIABILITY_H


