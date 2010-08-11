#include <sst_config.h>
#include "sst/core/serialization/element.h"


#include <sst/core/timeConverter.h>
#include "myMemEvent.h"
#include "cpu_router_power.h"


bool Cpu_router_power::clock( Cycle_t current) {
   MyMemEvent* event = NULL; 

   if (current == 100000 ) unregisterExit();

   if ( state == SEND ) {
      if ( ! event ) event = new MyMemEvent();

      if ( who == WHO_MEM ) { 
         event->address = 0x1000; 
         who = WHO_NIC;
      }
      else {
         event->address = 0x10000000; 
         who = WHO_MEM;
      }

      _CPU_ROUTER_POWER_DBG("send a MEM event address=%#lx\n", event->address );

      /* set up counts */
      //first reset all counts to zero
      power->resetCounts(&mycounts); 
      //then set up "this"-related counts
      mycounts.router_access=1;

      pdata = power->getPower(this, ROUTER, mycounts); 
      regPowerStats(pdata);

      mem->Send( (Cycle_t)3, event );
      state = WAIT;
      std::cout << "CPU " << getId() << "::clock -> sending a MEM event at cycle "<< current <<std::endl;

   }
   else {
      if ( ( event = static_cast< MyMemEvent* >( mem->Recv() ) ) ) {
         _CPU_ROUTER_POWER_DBG("got a MEM event address=%#lx\n", event->address );

         state = SEND;
         std::cout << "CPU " << getId() << "::clock -> got a MEM event at cycle "<< current <<std::endl;
	   }
   }
   return false;
}

extern "C" {
Cpu_router_power* cpu_router_powerAllocComponent( SST::ComponentId_t id, SST::Component::Params_t& params )
{
   return new Cpu_router_power( id, params );
}
}

BOOST_CLASS_EXPORT(Cpu_router_power)
BOOST_CLASS_EXPORT(SST::MyMemEvent)

