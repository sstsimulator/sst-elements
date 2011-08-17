/*
 * AppBcastTest.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppBcastTest.h"

int AppBcastTest::bcastRcvd = 0;
int AppBcastTest::responseRcvd = 0;

AppBcastTest::AppBcastTest(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppBcastTest::init() {

}

AppBcastTest::~AppBcastTest() {

}

simtime_t AppBcastTest::process(ApplicationData* pdata) {

	return pow(10.0, -9);
}

ApplicationData* AppBcastTest::getFirstMsg() {
	if (id == param1) {
		ApplicationData* adata = new ApplicationData();

		adata->setType(snoopReq);
		adata->setDestId(id);
		adata->setSrcId(id);
		adata->setPayloadSize(128);

		debug("application", "sending snoop bcast from ", id, UNIT_APP);

		return adata;

	}

	return NULL;
}

ApplicationData* AppBcastTest::dataArrive(ApplicationData* pdata) {

	int type = pdata->getType();

	bytesTransferred += pdata->getPayloadSize();

	debug("app", "message received at", id, UNIT_APP);
	debug("app", "... from ", pdata->getSrcId(), UNIT_APP);

	if (type == snoopReq) {
		bcastRcvd++;

		debug("app", "bcast received = ", bcastRcvd, UNIT_APP);

		ApplicationData* adata = new ApplicationData();

		adata->setType(snoopResp);
		adata->setDestId(pdata->getSrcId());
		adata->setSrcId(id);
		adata->setPayloadSize(128);

		debug("application", "sending snoop response from ", id, UNIT_APP);

		return adata;

	} else if (type == snoopResp) {
		responseRcvd++;

		debug("app", "response received = ", responseRcvd, UNIT_APP);

		if (id != param1) {
			std::cout << "id: " << id << endl;
			opp_error(
					"ERROR: App BcastTest: snoop response received at wrong destination");
		}
	} else {
		std::cout << "type: " << type << endl;
		opp_error("ERROR: App BcastTest: unexpected data type received");
	}

	return NULL;

}

