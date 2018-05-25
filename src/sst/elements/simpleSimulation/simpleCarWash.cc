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

//#include <assert.h>

#include "sst_config.h"
#include "simpleCarWash.h"

namespace SST {
namespace SimpleCarWash {

simpleCarWash::simpleCarWash(ComponentId_t id, Params& params) :
  Component(id) 
{
    int64_t RandomSeed;

    // See if any optional simulation parameters were set in the executing Python script
    // 
    clock_frequency_str = params.find<std::string>("clock", "1GHz");
    clock_count = params.find<int64_t>("clockcount", 1000);
    RandomSeed = params.find<int64_t>("randomseed", 151515);
    m_SimulationTimeLength = params.find<int64_t>("simulationtime", MINUTES_IN_A_DAY);

    std::cout << std::endl << std::endl;
    std::cout << "Welcome to Uncle Ed's Fantastic, Awesome, and Incredible Carwash!\n" << std::endl;    
    std::cout << "The system clock is configured for: " << clock_frequency_str << std::endl;
    std::cout << "Random seed used is: " << RandomSeed << std::endl;
    std::cout << "Simulation time will be: " << m_SimulationTimeLength << std::endl;
    std::cout << std::endl;
    
    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Set our Main Clock (register main clocks)
    // This will automatically be called by the SST framework
    registerClock(clock_frequency_str, new Clock::Handler<simpleCarWash>(this,
			      &simpleCarWash::tick));

#ifdef DISABLE_ONE_SHOTS 
// These are interesting, and we should do some fun stuff with them, like maybe have a car's driver
// become annoyed and jump out of our queue and leave???

    // Set some other clocks
    // Second Clock (5ns) 
    std::cout << "REGISTER CLOCK #2 at 5 ns" << std::endl;
    registerClock("5 ns", new Clock::Handler<simpleCarWash, uint32_t>(this, &simpleCarWash::Clock2Tick, 222));
    
    // Third Clock (15ns)
    std::cout << "REGISTER CLOCK #3 at 15 ns" << std::endl;
    Clock3Handler = new Clock::Handler<simpleCarWash, uint32_t>(this, &simpleCarWash::Clock3Tick, 333);
    tc = registerClock("15 ns", Clock3Handler);
#endif

    // This is the constructor, Let's set up the car was simulation
    // Random number generation can be improved! 

    m_rng = new SST::RNG::MarsagliaRNG(11,  RandomSeed);
    srand(5);					// We need a bit of randomness for our customer arrival simulation

    // Initialize all of the simulation variables and parameters (this code is reasonably self-explanatory)

    m_ptrCarRecordList = NULL;	// Clear the car queue linked list
    m_CarWash.currentTime = 0;	// Midnight (we are a 24 hour operation)
    m_CarWash.smallCarsWashed = 0;	// Clear the car's washed counters
    m_CarWash.largeCarsWashed =0;	//
    m_CarWash.noCarsArrived = 0;	//
    m_LargeBay.Occupied = WASH_BAY_EMPTY; 
    m_LargeBay.timeToOccupy = 0;
    m_SmallBay.Occupied = WASH_BAY_EMPTY;
    m_SmallBay.timeToOccupy = 0;
    m_CarWashTick = 0;
    m_HourlyReport.smallCarsWashed = 0;
    m_HourlyReport.largeCarsWashed = 0;
    m_HourlyReport.noCarsArrived = 0;

    // inventory variables (you can change these)
    
    m_blueSoap.soapInv = 50;
    m_strawBrush.brushInv = 10;


    std::cout << "The carwash simulation is now initialized and ready to run" << std::endl;

#ifdef DISABLE_ONE_SHOTS  
    // Create the OneShot Callback Handlers
    callback1Handler = new OneShot::Handler<simpleCarWash, uint32_t>(this, &simpleCarWash::Oneshot1Callback, 456);
    callback2Handler = new OneShot::Handler<simpleCarWash>(this, &simpleCarWash::Oneshot2Callback);
#endif

}

simpleCarWash::simpleCarWash() :
    Component(-1)
{
    // for serialization only
}

bool simpleCarWash::tick( Cycle_t ) 
{
	int CarType;

	m_CarWash.currentTime = m_CarWashTick;
	CarType = CheckForNewCustomer();

	if(NO_CAR == CarType) {
		m_CarWash.noCarsArrived++;
		m_HourlyReport.noCarsArrived++;
	}  // if(NO_CAR...
	else {
//		printf("Queueing a car type=%d\n", CarType);
		QueueACar(CarType, m_CarWash.currentTime);
		if(SMALL_CAR == CarType)
			m_HourlyReport.smallCarsWashed++;
		else if(LARGE_CAR == CarType)
			m_HourlyReport.largeCarsWashed++;
	} // else {...


    // Announce the top of the hour and give a little summary of where we are at with our carwashing business
    if((0 < m_CarWashTick) && (0 == (m_CarWashTick % 60))) {
		std::cout << "------------------------------------------------------------------" << std::endl;
		std::cout << "Time= " << m_CarWashTick << "(minutes) another simulated hour has passed! Summary:" << std::endl;
		std::cout << " Small Cars = " << m_HourlyReport.smallCarsWashed << std::endl; 
		std::cout << " Large Cars = " << m_HourlyReport.largeCarsWashed << std::endl;
		std::cout << " No Cars = " << m_HourlyReport.noCarsArrived << std::endl;
		std::cout << std::endl;
		// ShowCarArt(); // uncomment to see graphical of cars in queue
		m_HourlyReport.smallCarsWashed = 0;
		m_HourlyReport.largeCarsWashed = 0;
		m_HourlyReport.noCarsArrived = 0;
    }  // if(0 == (m_CarWashTick...

    if((m_SimulationTimeLength) <= m_CarWash.currentTime) {
		std::cout << std::endl << std::endl;
		std::cout << "------------------------------------------------------------------" << std::endl;
		std::cout << "Uncle Ed's Carwash Simulation has completed!" << std::endl;
		std::cout << "Here's a summary of the results:" << std::endl;
	        std::cout << "Simulation duration was:" << m_SimulationTimeLength 
                          << "(sec.) (" << (m_SimulationTimeLength / 60) << " hours) of operation." << std::endl;
		std::cout << "Small Cars Washed: " << m_CarWash.smallCarsWashed << std::endl;
		std::cout << "Large Cars Washed: " << m_CarWash.largeCarsWashed << std::endl;
		std::cout << "Total Cars washed: " << (m_CarWash.smallCarsWashed +  m_CarWash.largeCarsWashed) << std::endl;
		std::cout << "No new car arrival events: " << m_CarWash.noCarsArrived << std::endl;
		std::cout << "------------------------------------------------------------------" << std::endl;
		ShowDisappointedCustomers();
		return(true);
	}
    
    // See what carwash bays can be cycled, and if any new cars can be processed in 
    if(false == CarWashClockTick(m_CarWashTick)) {
	    m_CarWashTick++;
	    return(false);
    }
 
    std::cout << "Help! For some reason the simulator has reached code that shouldn't have been reached!" << std::endl;
    std::cout << "This would imply there is a bug that needs to be addressed??" << std::endl;
	
    return(true);
  
#ifdef TEST
    // return false so we keep going
    if(clock_count == 0) {
    primaryComponentOKToEndSim();
        return true;
    } else {
        return false;
    }

#endif

}

// This is cool, and works well, but I'm not using it at the moment...

bool simpleCarWash::Clock2Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
    // NOTE: THIS IS THE 5NS CLOCK 
    std::cout << "  CLOCK #2 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;
    
    // return false so we keep going or true to stop
    if (CycleNum == 15) {
        return true;
    } else {
        return false;
    }
}
    
bool simpleCarWash::Clock3Tick(SST::Cycle_t CycleNum, uint32_t Param)
{
#ifdef NOT_USED

// But this is pretty cool timing code, that might be fun for other events, 
// like running out of carwash soap?

    std::cout << "Uncle Ed's carwash optional timing events..." << CycleNum << std::endl;

    // NOTE: THIS IS THE 15NS CLOCK 
    std::cout << "  CLOCK #3 - TICK Num " << CycleNum << "; Param = " << Param << std::endl;    
    
    if ((CycleNum == 1) || (CycleNum == 4))  {
        std::cout << "*** REGISTERING ONESHOTS " << std::endl ;
        registerOneShot("10ns", callback1Handler);
        registerOneShot("18ns", callback2Handler);
    }
    
    // return false so we keep going or true to stop
    if (CycleNum == 15) {
	   DumpCarRecordList();
        return(true);
    }  // if(CycleNum == 15...
#endif

    return false;
}

void simpleCarWash::Oneshot1Callback(uint32_t Param)
{
    std::cout << "-------- ONESHOT #1 CALLBACK; Param = " << Param << std::endl;
    return;
}
    
void simpleCarWash::Oneshot2Callback()
{
    std::cout << "-------- ONESHOT #2 CALLBACK" << std::endl;
}

// Carwash Member Functions
int simpleCarWash::CheckForNewCustomer()
{
	int rndNumber;

	// Okay so this is a bit on the kludgy side, but let's at least make an attempt at a 
     // Random number...
	rndNumber = (int)(m_rng->generateNextInt32());		
	rndNumber = (rndNumber & 0x0000FFFF) ^ ((rndNumber & 0xFFFF0000) >> 16);
	rndNumber = abs((int)(rndNumber % 3));

//	printf("Customer Code:%d\n", rndNumber);

	return(rndNumber);
}  // int CheckForNewCustomer()


int simpleCarWash::QueueACar(int carSize, int clockTick)
{
	// Find the end of the car queue
	
	CAR_RECORD *ptr = m_ptrCarRecordList;

	if(NULL != ptr)	{			// Check to see if the list is empty
		while(NULL != ptr->ptrNext)	// If not walk down the list to the end
			ptr = ptr->ptrNext;		// 
	}  // if(NULL != ptr...

	// Allocate a bit of memory, formatted for a car record, and set the pointers. 
	if(NULL == ptr) { 				// First entry is a special case
		if(NULL == (m_ptrCarRecordList = new CAR_RECORD)) {
			printf("Error allocating memory for the first new car wash record\n");
			return(-1);
		}
		ptr = m_ptrCarRecordList;			// Anchor the new list
	}
	else {
		if(NULL == (ptr->ptrNext = new CAR_RECORD)) {
			printf("Error allocating memory to create a new car wash record\n");
			return(-1);
		}
		ptr = ptr->ptrNext;					// Index to that pointer
	}

	ptr->ptrNext = NULL;				// Set the pointer
	ptr->CarSize = carSize;				// Set the car's size
	ptr->EntryTime = clockTick;			// Save of the entry time

	if(1 == carSize) 
		ptr->washTime = SMALL_CAR_WASH_TIME;		// Three minutes to wash a small car
	else if(2 == carSize)
		ptr->washTime = LARGE_CAR_WASH_TIME;		// Five minutes to wash a big car

	return(0);
} // QueueACar()

// Pull a car record from the linked list.  There's most certainly a simpler way to do this
// but this will do the trick for the moment...

int *simpleCarWash::DeQueueCar()
{
	CAR_RECORD *car;
	CAR_RECORD *pTemp;

	car = new CAR_RECORD;
	
	// Get the next car in line				
	car->EntryTime = m_ptrCarRecordList->EntryTime;
	car->CarSize = m_ptrCarRecordList->CarSize;
	car->washTime = m_ptrCarRecordList->washTime;
	car->ptrNext = NULL;						// This isn't necessary, but it's tidy
	
	// Delete the record and reconcile the pointers
	pTemp = m_ptrCarRecordList;						// Get the record's pointer
	m_ptrCarRecordList = m_ptrCarRecordList->ptrNext;		// Remove the record, adjusting the link
	delete pTemp;									// Free the record's memory 

	return((int *)car);
}  // DeQueueCar()


// Debug Function - This simply prints out the contents of the carwash link list
// This code is unnecessary and can be deleted
int simpleCarWash::DumpCarRecordList()
{
	CAR_RECORD *ptr = m_ptrCarRecordList;	// Allocate a temporary pointer
	int count = 0;

	std::cout << "\n\n\nHere's a list of the car wash records" << std::endl;
	while(1) {
		if(NULL != ptr) {
			std::cout << "Car record[" << count << "]: " << ptr->EntryTime << " " << ptr->CarSize << std::endl;
			count++;				// Increment
			ptr = ptr->ptrNext;		// Index to the next pointer 
		} // i(NULL != ptr...
		else {
			std::cout << "Done" << std::endl;
			break;
		}
	} // while(1)...
	return(0);
}  // DumpCarRecordList()

int simpleCarWash::ShowDisappointedCustomers()
{
	CAR_RECORD *ptr = m_ptrCarRecordList;
	int SmallCarCustomers = 0;
	int LargeCarCustomers = 0;

	while(1) {
		if(NULL != ptr) {
			if(SMALL_CAR == ptr->CarSize)
				SmallCarCustomers++;
			else if(LARGE_CAR == ptr->CarSize)
				LargeCarCustomers++;
			ptr = ptr->ptrNext;		// Index to the next pointer 
		} // i(NULL != ptr...
		else {
			std::cout << std::endl;
			break;
		}
	} // while(1)...

	std::cout << "Disappointed Customers" << std::endl;
	std::cout << "-----------------------------" << std::endl;
	std::cout << "Small Cars: " << SmallCarCustomers << std::endl;
	std::cout << "Large Cars: " << LargeCarCustomers << std::endl;
	std::cout << "Total: " << SmallCarCustomers + LargeCarCustomers << std::endl;

	return(0);
}

int simpleCarWash::ShowCarArt()
{
	CAR_RECORD *ptr = m_ptrCarRecordList;

	while(1) {
		if(NULL != ptr) {
			if(SMALL_CAR == ptr->CarSize)
				std::cout << "SC: ";
			else if(LARGE_CAR == ptr->CarSize)
				std::cout << "LC: ";
			ptr = ptr->ptrNext;		// Index to the next pointer 
		} // i(NULL != ptr...
		else {
			std::cout << std::endl;
			break;
		}
	} // while(1)...
	return(0);
}  // int simpleCarWash::ShowCarArt()


bool simpleCarWash::IsCarWashBayReady(CAR_WASH_BAY *ptrWashBay)
{
	if(ptrWashBay->Occupied != WASH_BAY_EMPTY) {
		ptrWashBay->timeToOccupy--;
		if(ptrWashBay->timeToOccupy <= 0) {
			ptrWashBay->Occupied = WASH_BAY_EMPTY;
			return(true);
		}
	}   // if(ptrWashBay->Occupied

	return(false);		// Bay isn't empty
}  // bool simpleCarWash::IsWashBayEmpty()


// each clock tick we do 'workPerCycle' iterations of a simple loop.
// We have a 1/commFreq chance of sending an event of size commSize to
// one of our neighbors.
bool simpleCarWash::CarWashClockTick(int clockTick) 
{
	// This is the callback for a clock-tick (one minute has elapsed)

	CAR_RECORD *pTemp;
	
	// Now let's see if we have a bay empty, or ready to be emptied
	// Return of true implies the bay is empty and ready for a new car
	if(true == IsCarWashBayReady(&m_LargeBay)) {
		m_CarWash.largeCarsWashed++;
	}

	if(true == IsCarWashBayReady(&m_SmallBay)) {
		m_CarWash.smallCarsWashed++;
	}

	// Now that we've check the bays, let's fill any that have emptied. 
	if(NULL != m_ptrCarRecordList) {		// Are there any cars waiting? 
		// See if we can fill the small bay first
		if((m_SmallBay.Occupied == WASH_BAY_EMPTY) && (m_ptrCarRecordList->CarSize == SMALL_CAR))  { // SB - Open SM - Next
//			printf("%d Filling Small Bay\n", __LINE__);
			m_SmallBay.Occupied = WASH_BAY_FULL;
			m_SmallBay.timeToOccupy = SMALL_CAR_WASH_TIME;
			pTemp = m_ptrCarRecordList;		// Save the Linked List Pointer
			m_ptrCarRecordList = m_ptrCarRecordList->ptrNext;	// Move the link pointer
			delete pTemp;			// Free the memory
		} // if((m_SmallBay.Occupied
	}  // if(NULL != m_ptrCarRecord

	// The simulation rules stipulate that a large bay can accomodate either sized cars, but we can adjust the wash time
	// Apparently we have somewhat intelligent car-washing equipment, of course this can change too. 

	if(NULL != m_ptrCarRecordList) {		// Examine the linked list to see if there are any cars waiting to be washed?
		if(m_LargeBay.Occupied == WASH_BAY_EMPTY)  { 
//			printf("%d, Filling Large Bay\n", __LINE__);
			m_LargeBay.Occupied = WASH_BAY_FULL;
			// Set the wash time based on the car's size
			if(m_ptrCarRecordList->CarSize == LARGE_CAR)
				m_LargeBay.timeToOccupy = LARGE_CAR_WASH_TIME;
			else
				m_LargeBay.timeToOccupy = SMALL_CAR_WASH_TIME;

			pTemp = m_ptrCarRecordList;		// Save the Linked List Pointer
			m_ptrCarRecordList = m_ptrCarRecordList->ptrNext;	// Move the link pointer
			delete pTemp;			// Free the memory
		}  // if((largeBay...
	}  // if(NULL != m_ptrCarRecord...

	// Since we might have moved a large car out of the way, let's recheck and see  if we can place a small car
	if(NULL != m_ptrCarRecordList) {		// Are there any cars waiting? 
		if((m_SmallBay.Occupied == WASH_BAY_EMPTY) && (m_ptrCarRecordList->CarSize == SMALL_CAR))  {
//			printf("%d Trying again to fill Small Bay\n", __LINE__);
			m_SmallBay.Occupied = WASH_BAY_FULL;
			m_SmallBay.timeToOccupy = SMALL_CAR_WASH_TIME;
			pTemp = m_ptrCarRecordList;				// Save the Linked List Pointer
			m_ptrCarRecordList = m_ptrCarRecordList->ptrNext;	// Move the link pointer
			delete pTemp;			// Free the memory
		} // if((m_SmallBay...
	}  // if(NULL != m_ptrCarRecord...
    
    // return false so we keep going  (No errors)
    return(false);   
}

// Serialization 
} // namespace simpleCarWash
} // namespace SST


