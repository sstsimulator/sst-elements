/*
 * AppOne2One.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppOne2One.h"

int AppOne2One::accessSent = 0;
int AppOne2One::notifySent = 0;

int AppOne2One::notifyRcvd = 0;
int AppOne2One::accessRcvd = 0;

AppOne2One::AppOne2One(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppOne2One::init() {

	next = -1;

	stage = 0;

	goMemory = false;
	doneSent = false;

	packetSize = param1;

}

AppOne2One::~AppOne2One() {

}

simtime_t AppOne2One::process(ApplicationData* pdata) {

	return pow(10.0, -9);
}

ApplicationData* AppOne2One::getFirstMsg() {

	if (id == param2) {

		if (param4 == 0) {
			ApplicationData* pdata = new ApplicationData();

			pdata->setType(MPI_send);
			pdata->setDestId(param3);
			pdata->setSrcId(id);
			pdata->setPayloadSize(param1);

			debug("application", "sending MPI_send from ", id, UNIT_APP);
			debug("application", "... to ", param3, UNIT_APP);

			return pdata;
		} else if (param4 == 1) {
			MemoryAccess* adata = new MemoryAccess();

			adata->setDestId(param3);
			adata->setSrcId(id);

			adata->setType(param2 == param3 ? DM_writeLocal : DM_writeRemote);
			adata->setPayloadSize(param1);

			adata->setAddr(128);
			adata->setAccessType(MemoryWriteCmd);
			adata->setPriority(0);
			adata->setThreadId(0);
			adata->setAccessSize(param1);

			debug("application", "sending DM_writeRemote from ", id, UNIT_APP);
			debug("application", "... at access point ", param3, UNIT_APP);

			return adata;

		} else if (param4 == 2) {
			MemoryAccess* adata = new MemoryAccess();

			adata->setDestId(param3);
			adata->setSrcId(id);

			adata->setType(param2 == param3 ? DM_readLocal : DM_readRemote);
			adata->setPayloadSize(64);

			adata->setAddr(128);
			adata->setAccessType(MemoryReadCmd);
			adata->setPriority(0);
			adata->setThreadId(0);
			adata->setAccessSize(param1);

			debug("application", "sending DM_readRemote from ", id, UNIT_APP);
			debug("application", "... at access point ", param3, UNIT_APP);

			return adata;

		}

	}

	return NULL;

}

ApplicationData* AppOne2One::dataArrive(ApplicationData* pdata) {

	int type = pdata->getType();

	bytesTransferred += pdata->getPayloadSize();

	debug("app", "message received at", id, UNIT_APP);
	debug("app", "... from ", pdata->getSrcId(), UNIT_APP);

	return NULL;

}

