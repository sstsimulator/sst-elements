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

#ifndef _INTROSPECTOR_ROUTER_H
#define _INTROSPECTOR_ROUTER_H

#include <sst/core/introspector.h>


using namespace SST;

#if DBG_INTROSPECTOR_ROUTER
#define _INTROSPECTOR_ROUTER_DBG( fmt, args...)\
         printf( "%d:Introspector_router::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_ROUTER_DBG( fmt, args...)
#endif



class Introspector_router : public Introspector {
       
    public:
	Introspector_router(Component::Params_t& params ) :
            Introspector(),
            params( params ),
            frequency( "1ns" )
        {
            _INTROSPECTOR_ROUTER_DBG( "new id=%lu\n", id );

            Component::Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _INTROSPECTOR_ROUTER_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("period") ) {
		    frequency = it->second;
                }  
		else if ( ! it->first.compare("model") ) {
		    model = it->second;
                }    
                ++it;
            } 
            
            totalRouterDelay = 0;
	    numLocalMessage = 0;
	    currentPower = leakagePower = runtimePower = totalPower = peakPower = 0;

            _INTROSPECTOR_ROUTER_DBG("-->frequency=%s\n",frequency.c_str());

	    TimeConverter* tc = registerClock( frequency, new Clock::Handler<Introspector_router>(this, &Introspector_router::pullData) );

	    ///registerClock( frequency, new Clock::Handler<Introspector_router>(this, &Introspector_router::mpiCollectInt) );
	    ///mpionetimehandler = new Event::Handler< Introspector_router >
	    ///                                    ( this, &Introspector_router::mpiOneTimeCollect );

	    printf("INTROSPECTOR_ROUTER period: %ld\n",(long int)tc->getFactor());
            _INTROSPECTOR_ROUTER_DBG("Done registering clock\n");
            
        }
        int Setup() {
	    std::pair<bool, int> pint, pdouble;
	    
	    //get a list of relevant component. Must be done after all components are created 
	    MyCompList = getModels(model); 
	    //std::cout << " introspector_router has MyCompList size = " << MyCompList.size() << std::endl;
	    for (std::list<IntrospectedComponent*>::iterator i = MyCompList.begin();
	        i != MyCompList.end(); ++i) {
     		    // state that we will monitor those components 
		    // (pass introspector's info to the component)
     		    monitorComponent(*i);

		    //check if the component counts the specified int/double data
		    pint = (*i)->ifMonitorIntData("router_delay");
		    if(pint.first){		
			//store pointer to component and the dataID of the data of interest	
			addToIntDatabase(*i, pint.second); 
		    }
		    pint = (*i)->ifMonitorIntData("local_message");
		    if(pint.first){		
			addToIntDatabase(*i, pint.second); 
		    }
		
		    pdouble = (*i)->ifMonitorDoubleData("current_power");
		    if(pdouble.first){		
			addToDoubleDatabase(*i, pdouble.second); 
		    }
		    pdouble = (*i)->ifMonitorDoubleData("leakage_power");
		    if(pdouble.first){		
			addToDoubleDatabase(*i, pdouble.second); 
		    }
		    pdouble = (*i)->ifMonitorDoubleData("runtime_power");
		    if(pdouble.first){		
			addToDoubleDatabase(*i, pdouble.second); 
		    }
		    pdouble = (*i)->ifMonitorDoubleData("total_power");
		    if(pdouble.first){		
			addToDoubleDatabase(*i, pdouble.second); 
		    }
		    pdouble = (*i)->ifMonitorDoubleData("peak_power");
		    if(pdouble.first){		
			addToDoubleDatabase(*i, pdouble.second); 
		    }
		
	     }
	    ///oneTimeCollect(2000000, mpionetimehandler);
            _INTROSPECTOR_ROUTER_DBG("\n");
            return 0;
        }
        int Finish() {
            _INTROSPECTOR_ROUTER_DBG("\n");
            return 0;
        }


    private:
        Introspector_router( const Introspector_router& c );
	//~Introspector_router();
	Introspector_router() :  Introspector() {} // for serialization only

        bool pullData( Cycle_t );
	bool mpiCollectInt( Cycle_t );
	void mpiOneTimeCollect(Event* e);

	Event::Handler< Introspector_router > *mpionetimehandler;
        Component::Params_t    params;        
	std::string frequency;
	std::string model;
	uint64_t totalRouterDelay, numLocalMessage, intData;
	double currentPower, leakagePower, runtimePower, totalPower, peakPower;

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
