#include "electronicIOpad.h"
#include "statistics.h"

Register_Class( ElectronicIOpad)
;

long ElectronicIOpad::totalBits = 0;
long ElectronicIOpad::totalPackets = 0;

ElectronicIOpad::ElectronicIOpad() {

}

ElectronicIOpad::~ElectronicIOpad() {

}

void ElectronicIOpad::finish() {

	debug(getFullPath(), "FINISH: start", UNIT_FINISH);

	totalBits = 0;
	totalPackets = 0;

	debug(getFullPath(), "FINISH: done", UNIT_FINISH);
}

int ElectronicIOpad::numInitStages() const {
	return 2;
}

void ElectronicIOpad::initialize(int stage) {

	if (stage < 1)
		return;

	debug(getFullPath(), "INIT: start", UNIT_INIT);

	flitWidth = par("ioChannelWidth");

	clockPeriod = 1.0 / double(par("clockRate_io"));

	datarate = double(par("clockRate_io")) * flitWidth;

	//if(par("wireDoublePumping")){
	//	clockPeriod /= 2.0;   //double pumping
	//	datarate *= 2;
	//}

	bool low = par("lowSwing");

	//if (low)
	//	ePerBit = 500e-15; //got this from some paper, in darpa1/refs.  it's for a 10 cm wire, so the length is included.
	//else
		ePerBit = 1e-12;

	string statName = "Electronic IO Wire";

	E_dynamic = Statistics::registerStat(statName, StatObject::ENERGY_EVENT,
			"electronic");

	StatObject *E_count = Statistics::registerStat("Electronic IO Wire",
			StatObject::COUNT, "electronic");

	debug(getFullPath(), "INIT: done", UNIT_INIT);

}

void ElectronicIOpad::processMessage(cMessage *msg, simtime_t t,
		result_t& result) {
	cPacket* p = (cPacket*) msg;

	E_dynamic->track(p->getBitLength() * ePerBit);

	totalBits += p->getBitLength();
	totalPackets++;

	// datarate modeling
	if (datarate != 0 && msg->isPacket()) {
		simtime_t duration = ((cPacket *) msg)->getBitLength() / datarate;
		result.duration = duration;
		txfinishtime = t + duration + clockPeriod;
	} else {
		txfinishtime = t;
	}

	// propagation delay modeling
	result.delay = clockPeriod;

}

/*bool ElectronicIOpad::deliver(cMessage *msg, simtime_t at) {
 cPacket* p = (cPacket*) msg;

 int del = 1;

 //std::cout << "wire length is " << l << " mm " << endl;

 E_dynamic->track(p->getBitLength() * ePerBit);

 totalBits += p->getBitLength();
 totalPackets++;

 //simtime_t duration = p->getBitLength() / datarate;
 simtime_t duration = 0;
 simtime_t delay = del * clockPeriod;
 txfinishtime = at + duration;
 p->setDuration(duration);

 //copied from cDatarateChannel.cc
 EVCB.messageSendHop(msg, getSourceGate(), delay, duration);
 return getSourceGate()->getNextGate()->deliver(msg, at);
 }*/

