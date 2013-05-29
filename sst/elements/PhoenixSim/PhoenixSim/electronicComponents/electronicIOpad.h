#include <omnetpp.h>
#include "messages_m.h"

//#include <cChannel.h>

#include "sim_includes.h"

class ElectronicIOpad: public cChannel {

public:
	ElectronicIOpad();
	virtual ~ElectronicIOpad();

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
	virtual bool isTransmissionChannel() const {
		return true;
	}

	virtual void 	forceTransmissionFinishTime (simtime_t t) {
		txfinishtime = t;
	}

	virtual double 	getNominalDatarate () const {
		return datarate;
	}


	StatObject* E_dynamic;

	double clockPeriod;

	simtime_t txfinishtime;

	double datarate;

	int gateIn;
	int gateOut;

	double flitWidth;

	static long totalBits;
	static long totalPackets;

	double ePerBit;

};

