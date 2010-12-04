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

#ifndef _INTROSPECTOR_MESMTHI_H
#define _INTROSPECTOR_MESMTHI_H

#include <sst/core/introspector.h>



using namespace SST;

#if DBG_INTROSPECTOR_MESMTHI
#define _INTROSPECTOR_MESMTHI_DBG( fmt, args...)\
         printf( "%d:Introspector_mesmthi::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_MESMTHI_DBG( fmt, args...)
#endif



class Introspector_mesmthi : public Introspector {
       
    public:
	Introspector_mesmthi(Component::Params_t& params ) :
            Introspector(),
            params( params ),
            frequency( "1ns" )
        {
            _INTROSPECTOR_MESMTHI_DBG( "new id=%lu\n", id );

            Component::Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _INTROSPECTOR_MESMTHI_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("period") ) {
		    frequency = it->second;
                }  
		else if ( ! it->first.compare("model") ) {
		    model = it->second;
                }    
                ++it;
            } 
            
            totalInstructions = totalPackets = totalBusTransfers = totalNetworkTransactions = 0;
	    totalDIRreads = totalDIRwrites = 0;
	    totalL1Ireads = totalL1Iwrites = totalL1Ireadhits = totalL1Iwritehits = 0;
	    totalL1Dreads = totalL1Dwrites = totalL1Dreadhits = totalL1Dwritehits = 0;
	    totalL2reads = totalL2writes = totalL2readhits = totalL2writehits = 0;

            _INTROSPECTOR_MESMTHI_DBG("-->frequency=%s\n",frequency.c_str());

	   

	    ///registerClock( frequency, new Clock::Handler<Introspector_mesmthi>(this, &Introspector_mesmthi::mpiCollectInt) );
	    ///mpionetimehandler = new Event::Handler< Introspector_mesmthi >
	    ///                                    ( this, &Introspector_mesmthi::mpiOneTimeCollect );
	    
	    ///printf("INTROSPECTOR_MESMTHI period: %ld\n",(long int)tc->getFactor());
            _INTROSPECTOR_MESMTHI_DBG("Done registering clock\n");
            
        }
        int Setup() {
	    std::pair<bool, int> pint, pdouble;
	    
	    //get a list of relevant component. Must be done after all components are created 
	    MyCompList = getModelsByType(model);

	    ///oneTimeCollect(2000000, mpionetimehandler);
            _INTROSPECTOR_MESMTHI_DBG("\n");
            return 0;
        }
        int Finish() {
	    ///triggeredUpdate();
            _INTROSPECTOR_MESMTHI_DBG("\n");
            return 0;
        }


    private:
        Introspector_mesmthi( const Introspector_mesmthi& c );
	//~Introspector_mesmthi();
	Introspector_mesmthi() :  Introspector() {} // for serialization only

        
	
	bool triggeredUpdate();

        Component::Params_t    params;        
	std::string frequency;
	std::string model;
	uint64_t intData;
	uint64_t totalInstructions, totalPackets, totalBusTransfers, totalNetworkTransactions;
	uint64_t totalDIRreads, totalDIRwrites;
	uint64_t totalL1Ireads, totalL1Iwrites, totalL1Ireadhits, totalL1Iwritehits;
	uint64_t totalL1Dreads, totalL1Dwrites, totalL1Dreadhits, totalL1Dwritehits;
	uint64_t totalL2reads, totalL2writes, totalL2readhits, totalL2writehits;

	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Introspector);
        ar & BOOST_SERIALIZATION_NVP(params);
        ar & BOOST_SERIALIZATION_NVP(frequency);
	ar & BOOST_SERIALIZATION_NVP(model);
    }
};

#endif
