/*
 * AppAll2All.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "App_MITUCB_All2All.h"

int App_MITUCB_All2All::accessSent = 0;
int App_MITUCB_All2All::notifySent = 0;

int App_MITUCB_All2All::notifyRcvd = 0;
int App_MITUCB_All2All::accessRcvd = 0;

App_MITUCB_All2All::App_MITUCB_All2All(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void App_MITUCB_All2All::init() {

	next = -1;



	stage = 0;

	goMemory = false;
	doneSent = false;

	packetSize = param1;

}

App_MITUCB_All2All::~App_MITUCB_All2All() {

}

simtime_t App_MITUCB_All2All::process(ApplicationData* pdata) {

	return pow(10.0, -9);
}

ApplicationData* App_MITUCB_All2All::getFirstMsg() {
	return sending(NULL);
}

ApplicationData* App_MITUCB_All2All::sending(ApplicationData* a) {

	next++;
	if (!goMemory) {
		if (next == id)
			next++;

		if (next >= numOfProcs) {
			stage++;
			if (stage >= param2)
				goMemory = true;
			next = 0;
		}
	}

	if (goMemory) {
		if (useioplane) {
			if (next >= dramcfg->getNumDRAM()) {
				/*if (!doneSent) {

				 ApplicationData* adata = new ApplicationData();

				 adata->setType(procDone);
				 adata->setDestId(id);
				 adata->setSrcId(id);
				 adata->setPayloadSize(128);

				 debug("application", "sending procDone from ", id, UNIT_APP);

				 doneSent = true;

				 return adata;
				 } else*/
				return NULL;
			} else {
				MemoryAccess* adata = new MemoryAccess();

				adata->setType(DM_writeRemote);
				adata->setDestId(dramcfg->getAccessCore(next));
				adata->setSrcId(id);
				adata->setPayloadSize(64);

				adata->setAddr(128);
				adata->setAccessType(MemoryWriteCmd);
				adata->setPriority(0);
				adata->setThreadId(0);
				adata->setAccessSize(packetSize);

				debug("application", "sending cacheReadMemory from ", id,
						UNIT_APP);
				debug("application", "... to dram ", next, UNIT_APP);
				debug("application", "... at access point ",
						dramcfg->getAccessCore(next), UNIT_APP);

				accessSent++;

				return adata;
			}

		} else {
			/*if (!doneSent) {
			 ApplicationData* pdata = new ApplicationData();

			 pdata->setType(procDone);
			 pdata->setPayloadSize(packetSize);

			 debug("application", "sending procDone from ", id, UNIT_APP);

			 doneSent = true;

			 return pdata;
			 } else*/
			return NULL;
		}
	} else {

		if (next == id)
			next++;

		ApplicationData* pdata = new ApplicationData();

		pdata->setType(MPI_send);
		pdata->setDestId(next);
		//pdata->setDestType(node_type_pe);
		pdata->setSrcId(id);
		pdata->setPayloadSize(packetSize);

		debug("application", "sending cachWriteToRemoteCache from ", id,
				UNIT_APP);
		debug("application", "... to ", next, UNIT_APP);

		notifySent++;

		return pdata;
	}
}

ApplicationData* App_MITUCB_All2All::dataArrive(ApplicationData* pdata) {

	int type = pdata->getType();

	bytesTransferred += pdata->getPayloadSize();

	debug("app", "message received at", id, UNIT_APP);
	debug("app", "... from ", pdata->getSrcId(), UNIT_APP);

	if (type == MPI_send) {
		notifyRcvd++;
		debug("app", "notifies sent = ", notifySent, UNIT_APP);
		debug("app", "notifies received = ", notifyRcvd, UNIT_APP);
		if (!useioplane && notifySent == notifyRcvd) {
			Statistics::done();
		}
	} else if (type == M_readResponse) {
		accessRcvd++;

		if (useioplane && notifySent == notifyRcvd && accessSent == accessRcvd) {
			Statistics::done();
		}
	} else {
		std::cout << "type: " << type << endl;
		opp_error("ERROR: App all2all: unexpected data type received");
	}

	return NULL;

}

