#ifndef _ELECTRONIC_CHANNEL_H_
#define _ELECTRONIC_CHANNEL_H_

#include <omnetpp.h>
#include "messages_m.h"

//#include <cchannel.h>
#include "sim_includes.h"

#include "ORION_Link.h"

class ElectronicChannel: public cChannel {

public:
	ElectronicChannel();
	virtual ~ElectronicChannel();

	double unitWireLength;
	double routerWidth;

protected:

	virtual void initialize(int stage);
	virtual void finish();

	//virtual bool deliver(cMessage *msg, simtime_t at);

	virtual void processMessage(cMessage *msg, simtime_t t, result_t& result);
	virtual int numInitStages() const;

	virtual simtime_t calculateDuration(cMessage *msg) const {
		if (msg->isPacket()) {
			return ((cPacket*) msg)->getBitLength() / datarate;
		}
		return 0; // Added to avoid Compile Warning
	}
	virtual simtime_t getTransmissionFinishTime() const {
		return txfinishtime;
	}
	virtual bool isBusy(){
		return simTime() < txfinishtime;
	}
	virtual bool isTransmissionChannel() const {
		return true;
	}

	virtual void 	forceTransmissionFinishTime (simtime_t t) {
		txfinishtime = t;
	}

	virtual double 	getNominalDatarate () const {
		return datarate;
	}
	StatObject* P_static;
	StatObject* E_dynamic;

	double clockPeriod;

	simtime_t txfinishtime;

	double datarate;

	bool power;
	bool lowswing;

	int spaceLengths; //in # routers it has to span (or multiples of the shortest wire)
	int routerLengths; //num of router widths

	double length; //in meters

	simtime_t propDelay; //num of buffers, aka, number of clock cycles

	double ePerBit;
	double staticPwr;
	simtime_t lastReport;

	bool energyInit;

	bool isThru;

	ORION_Tech_Config *conf;

};
#endif

