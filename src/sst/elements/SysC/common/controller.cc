// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/elements/SysC/common/controller.h"

#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/timeConverter.h>
using namespace SST;
using namespace SST::SysC;

#include <iostream>
using std::cout;
using std::endl;

SST::SysC::Controller* SST::SysC::primary_controller=NULL;
/*double SST::SysC::time_values[] = {
    1,       // fs
    1e3,     // ps
    1e6,     // ns
    1e9,     // us
    1e12,    // ms
    1e15     // s
};
 const char* SST::SysC::time_units[] = {
    "fs",
    "ps",
    "ns",
    "us",
    "ms",
    "s"
};
*/
//////////////////////////////////////////////////////////////////////////////
// Constructors
//////////////////////////////////////////////////////////////////////////////
Controller::Controller(SST::ComponentId_t _id) 
: BaseType(_id)
{
  out.verbose(CALL_INFO,1,0,"Constructing Controller (no params)\n");
  cycle_period=1;
  out.verbose(CALL_INFO,2,0,"getTimeConverter\n");
  cycle_converter=getTimeConverter("1ns");
  out.verbose(CALL_INFO,2,0,"registerClock\n");
  registerClock("1ns",
                new SST::Clock::Handler<ThisType>(this,&ThisType::tic)
               );
  crunch_threshold=1;
  out.verbose(CALL_INFO,2,0,"getTimeConverter\n");
  crunch_threshold_converter=getTimeConverter("1ns");
  out.verbose(CALL_INFO,2,0,"configureSelfLink\n");
  self_link = configureSelfLink("CrunchThreshold",
                                crunch_threshold_converter,
                                new Handler_t(this,
                                              &ThisType::handleCrunchEvent)
                               );
  //hoping this operates off the time converter units
  out.verbose(CALL_INFO,2,0,"Set Latency\n");
  self_link->setLatency(crunch_threshold);
  crunch_request_in_progress=false;
  //setup simulation, or retrieve previously setup simulation
  out.verbose(CALL_INFO,2,0,"get_curr_simcontect\n");
  sc_context=sc_core::sc_get_curr_simcontext();
  //registerPrimaryController(this);
  run_ahead=false;
  out.verbose(CALL_INFO,10,0,"Done Constructing\n");
}

//Traditional Component Constructor-------------------------------------------
Controller::Controller(SST::ComponentId_t _id,
                       SST::Params& _params) 
: Component(_id) 
, out("In @p() at @f:@l: ",0,0,Output::STDOUT)
{
  out.setVerboseLevel(
      _params.find_integer(CON_VERBOSE_LEVEL,DEF_CON_VERBOSE_LEVEL)
      );
  out.verbose(CALL_INFO,1,0,"Constructing Controller\n");
  //Determine self update period-------------
  FIND_REPORT_INT32(cycle_period,CON_CYCLE_PERIOD,DEF_CON_CYCLE_PERIOD);
  std::string cycle_units;
  FIND_REPORT_STRING(cycle_units,CON_CYCLE_UNITS,STR(DEF_CON_CYCLE_UNITS));
  //build string to create time_converter
  std::string cycle_string="1";
  cycle_string.append(cycle_units);
  //create time_converter
  out.verbose(CALL_INFO,2,0,"getTimeConverter '%s'\n",cycle_string.c_str());
  cycle_converter=getTimeConverter(cycle_string);
  //build string to create clock handler
  std::string period_string=std::to_string(cycle_period);
  period_string.append(cycle_units);
  //create clock handler
  out.verbose(CALL_INFO,2,0,"registerClock '%s'\n",period_string.c_str());
  registerClock(period_string,
                new SST::Clock::Handler<ThisType>(this,&ThisType::tic)
               );
  //Get crunch threshold parameter-------------
  FIND_REPORT_INT32(crunch_threshold,
                  CON_CRUNCH_THRESHOLD,
                  DEF_CON_CRUNCH_THRESHOLD
                  );
  std::string crunch_threshold_units;
  FIND_REPORT_STRING(crunch_threshold_units,
                     CON_CRUNCH_UNITS,
                     STR(DEF_CON_CRUNCH_UNITS)
                     );
  std::string crunch_string="1";
  crunch_string.append(crunch_threshold_units);
  out.verbose(CALL_INFO,2,0,"getTimeConverter '%s'\n",crunch_string.c_str());
  crunch_threshold_converter=getTimeConverter(crunch_string);
  //configure the link to allow for a crunch gap
  out.verbose(CALL_INFO,2,0,"configureSelfLink\n");
  self_link = configureSelfLink("CrunchThreshold",
                                crunch_threshold_converter,
                                new Handler_t(this,
                                              &ThisType::handleCrunchEvent)
                               );
  //hoping this operates off the time converter units
  out.verbose(CALL_INFO,2,0,"setLatency '%ld'\n",crunch_threshold);
  self_link->setLatency(crunch_threshold);
  crunch_request_in_progress=false;
  //setup simulation, or retrieve previously setup simulation
  sc_context=sc_core::sc_get_curr_simcontext();
  registerPrimaryController(this);
  FIND_REPORT_BOOL(run_ahead,CON_RUN_AHEAD,DEF_CON_RUN_AHEAD);
  out.verbose(CALL_INFO,10,0,"Done Constructing\n");
}

//////////////////////////////////////////////////////////////////////////////
//setup()
//////////////////////////////////////////////////////////////////////////////
void Controller::setup(){
  time_units.resize(6);
  time_values[0] = 1   ;       // fs
  time_values[1] = 1e3 ;     // ps
  time_values[2] = 1e6 ;     // ns
  time_values[3] = 1e9 ;     // us
  time_values[4] = 1e12;    // ms
  time_values[5] = 1e15;    // s
  time_units[0]  = "fs";
  time_units[1]  = "ps";
  time_units[2]  = "ns";
  time_units[3]  = "us";
  time_units[4]  = "ms";
  time_units[5]  = "s" ;

  //This takes place in setup to ensure that all objects have been created
  //first. Hopefully this means and systemc attributes, such as default time
  //units, are stable.
  //------------------------------------------------------------------------
  //Determine our time conversion units;
  sc_core::sc_time_params *sc_params=sc_context->m_time_params;
  //everthing in systemc is done in relation to default units, which are in
  //relation to a time_resolution which is in femto-seconds.
  //SystemC default: resolution = 1000 fs = 1 ps
  //                 default unit = 1000 resolution units = 1000 ps = 1 ns
  //we extract the time in fs
  Time_t t=sc_params->time_resolution;//*sc_params->default_time_unit;
  //changed this to put everything in terms of resolution, not default units
  //(allows everything to be integers
  int i;
  //using a built in systemc array of values, we can determine the 'timebase'
  //which is an enumerated value.
  for(i=0;i<6;++i)
    if(t==time_values[i])
      break;
  sc_time_base=static_cast<sc_core::sc_time_unit>(i);//sc_time_base=sc_core::SC_NS;
  //create a time converter based on the unit string provided by systemC
  //(very convenient)
  std::string base_string="1";
  base_string.append(time_units[sc_time_base]);
  //cout<<"getTimeConverter("""<<base_string<<""")"<<endl;
  sst_time_converter=getTimeConverter(base_string);
  //convert our cycle period into the SystemC default units
  cycle_period_common_units = 
      sst_time_converter->convertFromCoreTime(
          cycle_converter->convertToCoreTime(
              cycle_period
              )
          );
  //TODO:: output our default time units;
}

//////////////////////////////////////////////////////////////////////////////
//tic()
//////////////////////////////////////////////////////////////////////////////
bool Controller::tic(SimTime_t _cycle){
  out.verbose(CALL_INFO,100,0,"tic(%ld) %s\n",_cycle,dualTimeStamp().c_str());
  catchUp();
  if(run_ahead){
    out.verbose(CALL_INFO,200,0,"Running ahead ... ");
    sc_start(int(cycle_period_common_units),sc_time_base);
    out.verbose(CALL_INFO,200,0,"%s\n",dualTimeStamp().c_str());
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////////
//crunch()
//////////////////////////////////////////////////////////////////////////////
bool Controller::crunch(){
  ///when SysC_time > SST_time
  out.verbose(CALL_INFO,100,0,"crunch() %s\n",dualTimeStamp().c_str());
  Time_t crunch_time = nextSystemCEvent();
  int64_t lag=getLag();
  out.verbose(CALL_INFO,200,0,"Lag = %ld\n",lag);
  Time_t max_time = lag + cycle_period_common_units;
  if(crunch_time > max_time)
    crunch_time = max_time;
  if(crunch_time > 0){
    out.verbose(CALL_INFO,200,0,"crunch_time = %ld\n",crunch_time);
    sc_start(int(crunch_time),sc_time_base);
  }
  out.verbose(CALL_INFO,200,0,"%s\n",dualTimeStamp().c_str());
  return true;
}

//////////////////////////////////////////////////////////////////////////////
//catchUp()
//////////////////////////////////////////////////////////////////////////////
bool Controller::catchUp(){
  //cout<<"Controller::Catchup()";
  out.verbose(CALL_INFO,100,0,"catchup()\n");
  if(getLag()>0){
    sc_start(int(getLag()),sc_time_base);
    out.verbose(CALL_INFO,200,0,"%s\n",dualTimeStamp().c_str());
  }
  return true;
}

//////////////////////////////////////////////////////////////////////////////
//getLag()
//////////////////////////////////////////////////////////////////////////////
Controller::SignedTime_t Controller::getLag(){
  return getCurrentSimTime(sst_time_converter) 
      - sc_time_stamp().value();//.to_default_time_units();
}

//////////////////////////////////////////////////////////////////////////////
//requestCrunch()
//////////////////////////////////////////////////////////////////////////////
void Controller::requestCrunch(ComponentId_t _id){
  out.verbose(CALL_INFO,100,0,"requestCrunch()\n");
  if(!crunch_request_in_progress){
    crunch_request_in_progress=true;
    self_link->send(new NullEvent());
  }
}

//////////////////////////////////////////////////////////////////////////////
//registerComponent()
//////////////////////////////////////////////////////////////////////////////
void Controller::registerComponent(SST::Component* _obj){
}

//////////////////////////////////////////////////////////////////////////////
//nextSystemCEvent()
//////////////////////////////////////////////////////////////////////////////
Controller::Time_t Controller::nextSystemCEvent(){
  return sc_time_to_pending_activity(sc_context).value();//.to_default_time_units();
}

//////////////////////////////////////////////////////////////////////////////
//nextSSTEvent()
//////////////////////////////////////////////////////////////////////////////
Controller::Time_t Controller::nextSSTEvent(){
  //TODO: Implement
  return 0;
}
//////////////////////////////////////////////////////////////////////////////
//handleCrunchEvent()
//////////////////////////////////////////////////////////////////////////////
void Controller::handleCrunchEvent(SST::Event *_ev){
  crunch();
  crunch_request_in_progress=false;
  delete _ev;
}

std::string Controller::dualTimeStamp(){
  std::stringstream out;
  out<<"[SST Time:  "<< dec
      << getCurrentSimTime(sst_time_converter) 
      << time_units[static_cast<unsigned int>(sc_time_base)] << " -- "
      <<"SysC Time: "<<sc_time_stamp()<<"]";
  return out.str();
}
