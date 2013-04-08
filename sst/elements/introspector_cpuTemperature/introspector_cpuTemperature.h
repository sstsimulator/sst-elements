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

#ifndef _INTROSPECTOR_CPUTEMPERATURE_H
#define _INTROSPECTOR_CPUTEMPERATURE_H

//#include <sst/core/component.h>
#include <sst/core/introspector.h>


namespace SST {
namespace Introspector_CPUTemperature {

#if DBG_INTROSPECTOR_CPUTEMPERATURE
#define _INTROSPECTOR_CPUTEMPERATURE_DBG( fmt, args...)\
         printf( "%d:Introspector_cpuTemperature::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_CPUTEMPERATURE_DBG( fmt, args...)
#endif



class Introspector_cpuTemperature : public Introspector {
       
    public:
	Introspector_cpuTemperature(Component::Params_t& params) :
            Introspector(),
            params( params ),
            frequency( "1ns" )
        {
            _INTROSPECTOR_CPUTEMPERATURE_DBG( "new id=%lu\n", id );

            Component::Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _INTROSPECTOR_CPUTEMPERATURE_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("period") ) {
		    frequency = it->second;
                }  
		else if ( ! it->first.compare("model") ) {
		    model = it->second;
                }    
                ++it;
            } 
            
           
            _INTROSPECTOR_CPUTEMPERATURE_DBG("-->frequency=%s\n",frequency.c_str());
//             Handler = new EventHandler< Introspector_cpuTemperature, bool, Cycle_t >
//                                                 ( this, &Introspector_cpuTemperature::pullData );
//             TimeConverter* tc = registerClock( frequency, handler );
	    //TimeConverter* tc = registerClock( frequency, new Clock::Handler<Introspector_cpuTemperature>( this, &Introspector_cpuTemperature::pullData ) );

	    //printf("INTROSPECTOR_CPUTEMPERATURE period: %ld\n",
            //       (long int) tc->getFactor());
            _INTROSPECTOR_CPUTEMPERATURE_DBG("Done registering clock\n");

            
        }
        int Setup() {
	    std::pair<bool, int> pdouble;

	    //get a list of relevant component. Must be done after all components are created 
	    MyCompList = getModelsByType(model);  
	    //std::cout << " introspector_cpuTemperature has MyCompList size = " << MyCompList.size() << std::endl;
	    /*for (std::list<IntrospectedComponent*>::iterator i = MyCompList.begin();
	        i != MyCompList.end(); ++i) {
     		    // state that we will monitor those components 
		    // (pass introspector's info to the component)
     		    monitorComponent(*i);

		    //check if the component counts the specified int/double data
		    pdouble = (*i)->ifMonitorDoubleData("core_temperature");

		    if(pdouble.first){
		        //store pointer to component and the dataID of the data of interest
			addToDoubleDatabase(*i, pdouble.second);
		    }


	     }*/

            _INTROSPECTOR_CPUTEMPERATURE_DBG("\n");
            return 0;
        }
        int Finish() {
	    triggeredUpdate();
            _INTROSPECTOR_CPUTEMPERATURE_DBG("\n");
            return 0;
        }


    private:

        Introspector_cpuTemperature( const Introspector_cpuTemperature& c );
	//Introspector_cpuTemperature() {}
	Introspector_cpuTemperature() :  Introspector() {} // for serialization only

        bool pullData( Cycle_t );
	bool triggeredUpdate();	

        Component::Params_t    params;        
	std::string frequency;
	std::string model;

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

}
}

#endif
