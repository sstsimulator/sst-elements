// Built for testing the ORION power model.

#ifndef _CPU_ROUTER_POWER_H
#define _CPU_ROUTER_POWER_H

#include <sst/core/eventFunctor.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include "../power/power.h"

using namespace SST;

#if DBG_CPU_ROUTER_POWER
#define _CPU_ROUTER_POWER_DBG( fmt, args...)\
         printf( "%d:Cpu_router_power::%s():%d: "fmt, _debug_rank, __FUNCTION__,__LINE__, ## args )
#else
#define _CPU_ROUTER_POWER_DBG( fmt, args...)
#endif

class Cpu_router_power : public IntrospectedComponent {
   typedef enum { WAIT, SEND } state_t;
   typedef enum { WHO_NIC, WHO_MEM } who_t;
   public:
      Cpu_router_power( ComponentId_t id, Params_t& params ) :
         IntrospectedComponent( id ),
         params( params ),
         state(SEND),
         who(WHO_MEM), 
         frequency( "2.2GHz" ) {
            _CPU_ROUTER_POWER_DBG( "new id=%lu\n", id );
            registerExit();

            Params_t::iterator it = params.begin(); 
            while( it != params.end() ) { 
               _CPU_ROUTER_POWER_DBG("key=%s value=%s\n", it->first.c_str(),it->second.c_str());
               if ( ! it->first.compare("clock") ) {
                  frequency = it->second;
               }    
               ++it;
            } 
            
            mem = configureLink( "MEM" );
            TimeConverter* tc = registerClock( frequency, new Clock::Handler<Cpu_router_power>(this,&Cpu_router_power::clock) );
	  
	         mem->setDefaultTimeBase(tc);
	         printf("CPU_ROUTER_POWER period: %ld\n",tc->getFactor());
            _CPU_ROUTER_POWER_DBG("Done registering clock\n");
         }

         int Setup() {
            // report/register power dissipation	    
    	      power = new Power(getId());
            power->setTech(getId(), params, ROUTER, ORION);
            return 0;
         }

         int Finish() {
            std::pair<bool, Pdissipation_t> res = readPowerStats(this);
            if(res.first) {
               using namespace io_interval; std::cout <<"ID " << getId() <<": link leakage power = " << res.second.leakagePower << " W" << std::endl;
               using namespace io_interval; std::cout <<"ID " << getId() <<": link dynamic power = " << res.second.runtimeDynamicPower << " W" << std::endl;
               using namespace io_interval; std::cout <<"ID " << getId() <<": Total power = " << res.second.currentPower << " W" << std::endl;
               /*using namespace io_interval; std::cout <<"ID " << getId() <<": TDP = " << res.second.TDP << " W" << std::endl;
               using namespace io_interval; std::cout <<"ID " << getId() <<": total energy = " << res.second.totalEnergy << " J" << std::endl;
               using namespace io_interval; std::cout <<"ID " << getId() <<": peak power = " << res.second.peak << " W" << std::endl;*/
            }
            _CPU_ROUTER_POWER_DBG("\n");
            //unregisterExit();
            return 0;
         }


   private:
      Cpu_router_power( const Cpu_router_power& c );
	   Cpu_router_power() :  IntrospectedComponent(-1) {} // for serialization only

      bool clock( Cycle_t );

      Params_t    params;
      Link*       mem;
      state_t     state;
      who_t       who;
      std::string frequency;

      Pdissipation_t pdata, pstats;
      Power *power;
      usagecounts_t mycounts;  //over-specified struct that holds usage counts of its sub-components

      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive & ar, const unsigned int version ) {
         ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
         ar & BOOST_SERIALIZATION_NVP(params);
         ar & BOOST_SERIALIZATION_NVP(mem);
         ar & BOOST_SERIALIZATION_NVP(state);
         ar & BOOST_SERIALIZATION_NVP(who);
         ar & BOOST_SERIALIZATION_NVP(frequency);
      }
};

#endif
