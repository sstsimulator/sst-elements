// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _UNCLEEDSCARWASH_H
#define _UNCLEEDSCARWASH_H

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>
#include <sst/core/rng/marsaglia.h>

#define WASH_BAY_EMPTY 0
#define WASH_BAY_FULL 1
#define NO_CAR 0
#define SMALL_CAR 1
#define SMALL_CAR_WASH_TIME 3 // Number of simulator cycles
#define LARGE_CAR 2
#define LARGE_CAR_WASH_TIME 5 // Number of simulator cycles (we are using minutes, but this could be anything!)
#define MINUTES_IN_A_DAY 24*60

namespace SST {
namespace SimpleCarWash {

class simpleCarWash : public SST::Component 
{
public:
    simpleCarWash(SST::ComponentId_t id, SST::Params& params);		// Constructor

    void setup()  { }
    void finish() { }

	// Link list entry; used to store the cars as they are queued for service
     struct CAR_RECORD_T {
		int EntryTime;  		// Minutes from the car wash epoch
		int CarSize;			// 0 == Small Car; 1 == Large Car
		int washTime;			// How long has the car been in the wash booth? 
		CAR_RECORD_T *ptrNext;	// The next Car in the linked list	
	};

	typedef struct CAR_RECORD_T CAR_RECORD;

	// Record to manage the "books" for the car wash's day	
	typedef struct {
		int currentTime;		// Time (minutes) since the epoch of the day
		int smallCarsWashed;	// Number of small cars washed
		int largeCarsWashed;	// Number of large cars washed
		int noCarsArrived;		// Number of minutes when no new cars arrived
	} CAR_WASH_JOURNAL;

	// Record to manage the car wash bay itself
    typedef struct {
		int Occupied;
		int timeToOccupy;
		int WashBaySize;
    } CAR_WASH_BAY;
	
	// Record to manage carwash inventory
    typedef struct {
		int soapInv;
		int brushInv;
    } CAR_WASH_INV;

    SST_ELI_REGISTER_COMPONENT(
        simpleCarWash,
        "simpleSimulation",
        "simpleCarWash",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Simple Demo Componnent",
	COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
	{ "clock", "Clock frequency", "1GHz" },
    	{ "clockcount", "Number of clock ticks to execute", "100000" }
    )

private:
    simpleCarWash();  // for serialization only
    simpleCarWash(const simpleCarWash&); // do not implement
    void operator=(const simpleCarWash&); // do not implement
    
    virtual bool tick(SST::Cycle_t);
    
    virtual bool Clock2Tick(SST::Cycle_t, uint32_t);
    virtual bool Clock3Tick(SST::Cycle_t, uint32_t);
    
    virtual void Oneshot1Callback(uint32_t);
    virtual void Oneshot2Callback();
    
    // Carwash member variables and functions

    SST::RNG::MarsagliaRNG* m_rng;

    // Car wash simulator variables
    CAR_RECORD *m_ptrCarRecordList;
    CAR_WASH_JOURNAL m_CarWash;
    CAR_WASH_JOURNAL m_HourlyReport;
    CAR_WASH_BAY m_LargeBay;
    CAR_WASH_BAY m_SmallBay;
    CAR_WASH_INV m_blueSoap;
    CAR_WASH_INV m_strawBrush;
    int m_CarWashTick;
    int64_t m_SimulationTimeLength;

    int CheckForNewCustomer();
    int QueueACar(int carSize, int clockTick);
    int *DeQueueCar();
    bool IsCarWashBayReady(CAR_WASH_BAY *ptrWashBay);
    int DumpCarRecordList();
    bool CarWashClockTick(int clockTick);
    int ShowCarArt();
    int ShowDisappointedCustomers();

    TimeConverter*      tc;
    Clock::HandlerBase* Clock3Handler;
    
    // Variables to store OneShot Callback Handlers
    OneShot::HandlerBase* callback1Handler;
    OneShot::HandlerBase* callback2Handler;
    
    std::string clock_frequency_str;
    int clock_count;
};

} // namespace simpleCarWash
} // namespace SST

#endif /* _UNCLEEDSCARWASH_H */
