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

#ifndef _INTROSPECTOR_DRAM_H
#define _INTROSPECTOR_DRAM_H

#include <sst/core/introspector.h>


using namespace SST;

#if DBG_INTROSPECTOR_DRAM
#define _INTROSPECTOR_DRAM_DBG( fmt, args...)\
         printf( "%d:Introspector_dram::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _INTROSPECTOR_DRAM_DBG( fmt, args...)
#endif



class Introspector_dram : public Introspector {
       
    public:
	Introspector_dram(Component::Params_t& params ) :
            Introspector(),
            params( params ),
            frequency( "1ns" )
        {
            _INTROSPECTOR_DRAM_DBG( "new id=%lu\n", id );

            Component::Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
                _INTROSPECTOR_DRAM_DBG("key=%s value=%s\n",
                            it->first.c_str(),it->second.c_str());
                if ( ! it->first.compare("period") ) {
		    frequency = it->second;
                }  
		else if ( ! it->first.compare("model") ) {
		    model = it->second;
                }    
                ++it;
            } 
            
            intData = 0;
            _INTROSPECTOR_DRAM_DBG("-->frequency=%s\n",frequency.c_str());

	    TimeConverter* tc = registerClock( frequency, new Clock::Handler<Introspector_dram>(this, &Introspector_dram::pullData) );

	    ///registerClock( frequency, new Clock::Handler<Introspector_dram>(this, &Introspector_dram::mpiCollectInt) );
	    ///mpionetimehandler = new Event::Handler< Introspector_dram >
	                                        ( this, &Introspector_dram::mpiOneTimeCollect );

	    printf("INTROSPECTOR_DRAM period: %ld\n",(long int)tc->getFactor());
            _INTROSPECTOR_DRAM_DBG("Done registering clock\n");
            
        }
        int Setup() {
	    std::pair<bool, int> pint;
	    
	    //get a list of relevant component. Must be done after all components are created 
	    MyCompList = getModels(model); 
	    //std::cout << " introspector_dram has MyCompList size = " << MyCompList.size() << std::endl;
	    for (std::list<IntrospectedComponent*>::iterator i = MyCompList.begin();
	        i != MyCompList.end(); ++i) {
     		    // state that we will monitor those components 
		    // (pass introspector's info to the component)
     		    monitorComponent(*i);

		    //check if the component counts the specified int/double data
		    pint = (*i)->ifMonitorIntData("dram_backgroundEnergy");
		    if(pint.first){		
			//store pointer to component and the dataID of the data of interest	
			addToIntDatabase(*i, pint.second); 
		    }
		
		    pint = (*i)->ifMonitorIntData("dram_burstEnergy");
		    if(pint.first){			
			addToIntDatabase(*i, pint.second);  
		    }
		
		    pint = (*i)->ifMonitorIntData("dram_actpreEnergy");
		    if(pint.first){			
			addToIntDatabase(*i, pint.second); 
		    }
		
		    pint = (*i)->ifMonitorIntData("dram_refreshEnergy");
		    if(pint.first){			
			addToIntDatabase(*i, pint.second); 
		    }
		

	     }
	    ///oneTimeCollect(2000000, mpionetimehandler);
            _INTROSPECTOR_DRAM_DBG("\n");
            return 0;
        }
        int Finish() {
            _INTROSPECTOR_DRAM_DBG("\n");
            return 0;
        }


    private:
        Introspector_dram( const Introspector_dram& c );
	//~Introspector_dram();
	Introspector_dram() :  Introspector() {} // for serialization only

        bool pullData( Cycle_t );
	bool mpiCollectInt( Cycle_t );
	void mpiOneTimeCollect(Event* e);

	Event::Handler< Introspector_dram > *mpionetimehandler;
        Component::Params_t    params;        
	std::string frequency;
	std::string model;
	uint64_t intData;

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
