/*************************************************************************
 *                                                                       *
 *  POINTS - Photonic On-chip Interconnection Network Traffic Simulator  *
 *                       (c) Assaf Shacham 2006-7                        *
 *                                                                       *
 * file: includes.h                                                      *
 * description:                                                          *
 *                                                                       *
 *                                                                       *
 *************************************************************************/

#ifndef INCLUDES_H
#define INCLUDES_H

#include <omnetpp.h>
#include <string>

#include "StatObject.h"

using namespace std;

static int globalMsgId = 0;


#define FDBG1 ev<<"FDBG1 ("<<simTime()*1e9<<" ns): "
#define FDBG2 ev<<"FDBG2 ("<<simTime()*1e9<<" ns): "

#define SIM_PI 	3.14159265 	//mmm pi
#define SIM_BOLTZMANN 	1.38e-23 	// Boltzmann constant
#define SIM_E 	2.71828183 	// eeee

//which environment you're compiling and running on

// Uncomment to enable color changes
//#define ANIMATIONMODE

//uncomment to turn off insertion loss etc
//#define ENABLE_OPTICS_LOG

// Only allow node zero to initiate communications (demo purposes)
//#define NODEZERODEMO

// Enable Loading of Sesc Simulator (node model)
//#define ENABLE_SESC

// Enable Loading of DRAM accesses using DRAMsim
//#define ENABLE_DRAMSIM

// Enable HotSpot temperature simulation
//#define ENABLE_HOTSPOT

// Hotspot Granularity defines which devices are counted in hotspot model
// 0 all photonic devices and processors
// 1 just active devices and processors
// 2 just processors

#define HOTSPOT_GRANULARITY 2

//Enable ORION power model
#define ENABLE_ORION
//#define ORION_VERSION_1
#define ORION_VERSION_2

//Enables polling the system for peak power
//#define ENABLE_POWER_COLLECT

// Enable Noise Propagation
//#define ENABLE_NOISE
#define noiseGap 1e-12

// Enable error messages, turn off may save a couple cycles
#define ENABLE_DEBUG

// Enable physical simulation, loss counting through device, power model
#define ENABLE_PHYSICS


// ------- these included for future flexibility --------------//
// -------   v0.2 - don't change these at all ------------------//
//which state accumulates
#define PSE_POWER_STATE 1    //0 for thru, 1 for drop

//default pse state
#define PSE_DEFAULT_STATE 0   //0 for thru, 1 for drop
//---------------------------------------------------------------//

//debugging
void debug(string path, string s, int unit);
void debug(string path, string s, double data, int unit);
void debug(string path, string s, string data, int unit);

//unit codes
#define UNIT_NIC 0
#define UNIT_ROUTER 1
#define UNIT_APP 2
#define UNIT_PROCESSOR 3
#define UNIT_CACHE 4
#define UNIT_BUFFER 5
#define UNIT_OPTICS 6
#define UNIT_WIRES 7
#define UNIT_INIT 8
#define UNIT_DRAM 9
#define UNIT_FINISH 10
#define UNIT_DESTRUCT 11
#define UNIT_PATH_SETUP 12
#define UNIT_POWER 13
#define UNIT_PATH 14

#define UNIT_XBAR 15







#endif
