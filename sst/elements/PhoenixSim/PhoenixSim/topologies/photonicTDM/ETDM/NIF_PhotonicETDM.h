//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef NIF_PHOTONICETDM_H_
#define NIF_PHOTONICETDM_H_

#include <omnetpp.h>
#include "NIF.h"

#include "ScheduleItem.h"

#include "ORION_Array.h"

class NIF_PhotonicETDM: public NIF {
public:
	NIF_PhotonicETDM();
	virtual ~NIF_PhotonicETDM();

	virtual bool request();
	virtual bool prepare();
	virtual simtime_t send();
	virtual void complete();

	virtual void init();

	virtual void netPortMsgArrived(cMessage* cmsg);
	virtual void processorMsgSent(ProcessorData* pdata);
	virtual void procMsgArrived(ProcessorData* pdata);

	virtual void selfMsgArrived(cMessage* cmsg);


	void sendRingCntrl();
	bool sentCntrl;

	bool firstInSlot;

	//bool goTransmit();


	bool okToSchedule(ScheduleItem* it, int slot);
	bool addToSchedule(ProcessorData* pdata, bool isXY);

	void readControlFile(string fName);
	map<int, map<int, int>*> cntrlMatrix;
	map<int, map<int, int>*> reverseMatrix;
	map<int, list<ScheduleItem*>*> schedule;

	simtime_t tdmPeriod;
	cMessage* tdmClock;

	double dataRate;
	simtime_t clockPeriod_data;
	double cntrlRate;
	simtime_t cntrlPeriod;
	double procRate;

	simtime_t slotWaitTime;

	int numWavelengths;

	simtime_t msgDuration;

	long TDMbitsAllowed;

	long bitsSent;

	bool middleOfSending;
	bool sentThisSlot;
	//bool isXYdata;
	bool originalComplete;

	PhotonicMessage* start;
	PhotonicMessage* stop;

	int numX;
	int myId;
	int myRow;
	int myCol;

	//int rx;
	//int tx;

	int numTDMslots;
	int tdmSlot;
	//int* tdmSlotTime;
	int tdmSlotTime[28];
	int tdmSlotIteration;

	bool useIOplane;

	//list<ProcessorData*> xyBuffer;

	//ProcessorData* dataToSend;

	ScheduleItem* sentItem;

	ORION_Array* xyBuffer;

	StatObject* xyPower;

	StatObject* SO_lat_queue;
	StatObject* SO_lat_slot;

	StatObject* SO_lat_xy_queue;
	StatObject* SO_lat_xy_slot;

	StatObject** SO_lat_slot_array;

	StatObject* SO_lat_trans;

	int xy_line_width;

	//----------DRAM
	simtime_t tRCD;
	simtime_t tCL;
	simtime_t tRP;
	simtime_t DRAMguardTime;

	simtime_t readPropConst;
	int maxBurst;

	int rows;
	int cols;
	int arrays;
	int banks;
	int chips;
	double freq;

	int accessUnit; //in bytes
	simtime_t accessTime;

	map<int, bool> bankSet;

	int getBankId(int addr);
	int getRow(int addr);
	int getCol(int addr);

	void tryTqueue();

	// bank, row, open
	//map<int, map<int, bool>* > openRow;

	//map<int, map<int, bool>* > precharging;

	list<ProcessorData*> tQueue;
	list<ProcessorData*> xyQueue;

};

#endif /* NIF_PHOTONICTDM_H_ */
