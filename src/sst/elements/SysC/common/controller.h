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

#ifndef COMMON_CONTROLLER_H
#define COMMON_CONTROLLER_H
#include <sst/core/sst_types.h>

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>

#include "../common/util.h"

#include <sst/core/component.h>
#include <vector>
#include <string>
#include <sstream>
/*namespace sc_core{
extern double time_values[6];
extern char*  time_units[6];
}
*/
namespace SST{
namespace SysC{

//Whenever SystemC completes a chomp, it should check for the next event time in
//the systemC queue, compare this to a recorded 'nextTime', if it is sooner,
//then a selfLink event should be sent with a proper delay to catch things up.
//
//If clock_ports are used, this seperate object should generate the clock
//signals in response to sst clocks
//
//If an internal thread_clock is used, then it must be known so that the control
//object can make sure that it catches itself up at least every clock cycle, but
//maybe every clock edge
//
//The control object keeping a clock will cause the first two rules to be
//invoked, combined with adapter traffic using the first two rules, it should be
//difficult for either simulator to generate an event in the past.
//

class Controller: public SST::Component{
  typedef SST::SysC::Controller         ThisType;
  typedef SST::Component                BaseType;
 protected:
  typedef uint64_t                      Time_t;
  typedef int64_t                       SignedTime_t;
  typedef SimTime_t                     SSTTime_t;
  typedef std::vector<std::string>      UnitsVector_t;
  typedef SST::Event::Handler<ThisType> Handler_t;
 
 private:
  /// Update period in units specified in SDL
  SSTTime_t               cycle_period;
  /// Update period normalized to common units (SystemC Default Time Unit)
  SSTTime_t               cycle_period_common_units;
  /// Time converter for update cycle based on units specified in SDL
  TimeConverter*          cycle_converter;
  /// Time converter that specifies the common units to in terms of SST
  TimeConverter*          sst_time_converter;
  /// Link to self, used to schedule unclocked updates
  Link*                   self_link;
  /// Pointer to the systemC simulation context
  sc_core::sc_simcontext* sc_context;
  /// Indicates the units to be used for systemc
 
 public:
  sc_core::sc_time_unit   sc_time_base;
  UnitsVector_t time_units;
  double time_values[6];
 private:
  /// Threshold for for systemC events to request crunches, should be as small
  //as possible, but large enough to account for link delays and small time gaps
  //that are meant to be 'no gap'
  Output                  out;
  Time_t                  crunch_threshold;
  TimeConverter*          crunch_threshold_converter;
  bool                    crunch_request_in_progress;
  bool                    run_ahead;

 public:
  void setup();
  void finish(){}
  bool Status(){return 0;}
  //void init(){}
  //Version of the constructor when params are not present
  Controller(SST::ComponentId_t _id) ;
  //Traditional Component Constructor
  Controller(SST::ComponentId_t _id,
             SST::Params& _params);
  /** Returns the number of standard units that SystemC is behind SST,
   * if negative, then SystemC is ahead of SST;
   * @returns the difference between SST and SystemC time */
  SignedTime_t getLag();
  /** Used by components to request a crunch
   * Imediately crunches at this point, but will implement a queueing system to
   * avoid each component advancing the clock when multiple components may be
   * wanting to operate simulataneously 
   * @param _id A key used to track component requests*/
  void requestCrunch(ComponentId_t _id);
  /** Used to create a list of components that can request a crunch from this
   * controller. used with 'requestCrunch()'. Does nothing until queue system is
   * implemented.
   * @param _obj Component to register */
  void registerComponent(SST::Component* _obj);
  /** Returns the time in standard units until nextSystemC event, this is in
   * relation to systemC clock and not SST clock
   * @returns Time in standards units until next SystemC event in relation to
   * current SystemC time */
  Time_t nextSystemCEvent();
  /** Returns the time until next SST event in standard units and in relation to
   * current SSTTime
   * @returns Time unitl next SST event */
  Time_t nextSSTEvent();
  /** returns the timeconverter for the common unit
   * @returns a pointer to the common unit timeconveter */
  inline TimeConverter* getSSTTimeConverter(){
    return sst_time_converter;
  }
  /** returns a string containing both SST time and SystemC time
   * @returns String with both time stamps*/
  std::string dualTimeStamp();
 private:
  /** The clock handler
   * Simply catches SystemC up to current SST time
   * @param _cycle The current time in SST (unused) */
  bool tic(SimTime_t _cycle);
  /** Runs the simulation up to the next clock/or next event, whichever is
   * first
   * @returns Whether or not crunch executed (always true) */
  bool crunch();
  /** Runs the SystemC until current SST time
   * @returns Success of run (always true) */
  bool catchUp();
  /** Handles the loopback event intended to allow for a buffer of incoming
   * transaction to SystemC,
   * @param _ev A nullEvent */
  void handleCrunchEvent(SST::Event *_ev);
};
#define CON_CYCLE_PERIOD      "cycle_period"
#define DEF_CON_CYCLE_PERIOD  1
#define CON_CYCLE_UNITS       "cycle_units"
#define DEF_CON_CYCLE_UNITS   ns
#define CON_CRUNCH_THRESHOLD  "crunch_threshold"
#define DEF_CON_CRUNCH_THRESHOLD 1
#define CON_CRUNCH_UNITS      "crunch_threshold_units"
#define DEF_CON_CRUNCH_UNITS  ns
#define CON_RUN_AHEAD         "run_ahead"
#define DEF_CON_RUN_AHEAD     0
#define CON_VERBOSE_LEVEL     "verbose_level"
#define DEF_CON_VERBOSE_LEVEL 1

#define CONTROLLER_PARAMS                                                     \
{CON_CYCLE_PERIOD,                                                            \
  "uINT: Number of [cycle_units] between automatic catchups",                 \
  STR(DEF_CON_CYCLE_PERIOD)},                                                 \
{CON_CYCLE_UNITS,                                                             \
  "STR: time unit to express cycle period in",                                \
  STR(DEF_CON_CYCLE_UNITS)},                                                  \
{CON_CRUNCH_THRESHOLD,                                                        \
  "uINT: Delay in [crunch_threshold_units] before initiating crunch",         \
  STR(DEF_CON_CRUNCH_THRESHOLD)},                                             \
{CON_CRUNCH_UNITS,                                                            \
  "STR: time unit to express [crunch_threshold] in",                          \
  STR(DEF_CON_CRUNCH_UNITS)},                                                 \
{CON_RUN_AHEAD,                                                               \
  "BOOL: Should systemC run ahead of SST",                                    \
  STR(DEF_CON_RUN_AHEAD)},                                                    \
{CON_VERBOSE_LEVEL,                                                           \
  "uINT: Level of output to generate",                                        \
  STR(DEF_CON_VERBOSE_LEVEL)}


//////////////////////////////////////////////////////////////////////////////
//Static Parts
//////////////////////////////////////////////////////////////////////////////
//extern double time_values[];
//extern const char* time_units[];
extern Controller* primary_controller;
inline Controller* getPrimaryController(){
  return primary_controller;
}
inline void registerPrimaryController(Controller* _obj){
  primary_controller=_obj;
}
}//namespace SysC
}//namespace SST

#endif //COMON_CONTROLLER_H
