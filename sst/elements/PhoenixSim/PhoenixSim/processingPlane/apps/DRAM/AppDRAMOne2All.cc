/*
 * AppOne2AllDRAM.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppDRAMOne2All.h"



AppOne2AllDRAM::AppOne2AllDRAM(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppOne2AllDRAM::init() {

	numSent = 0;
	doneSent = false;

}

AppOne2AllDRAM::~AppOne2AllDRAM() {

}

simtime_t AppOne2AllDRAM::process(ApplicationData* pdata) {

	return 0;
}

ApplicationData* AppOne2AllDRAM::getFirstMsg() {
	return sending(NULL);
}

ApplicationData* AppOne2AllDRAM::sending(ApplicationData* adata){

	if(!useioplane){
		opp_error("you can't use one2allDRAM without enabling the ioplane");
	}
	if (id == 0) {
		if (numSent < dramcfg->getNumDRAM()) {
			MemoryAccess* pdata = new MemoryAccess();

			int destid = dramcfg->getAccessCore(numSent);

			pdata->setType((param2 == 0) ? ((destid == id) ? DM_writeLocal : DM_writeRemote)
					: ((destid == id) ? DM_readLocal : DM_readRemote));
			pdata->setDestId(destid);
			pdata->setSrcId(id);
			pdata->setPayloadSize((param2 == 0) ? pow(2.0, param3) : 64);

			pdata->setAddr(rand());
			pdata->setAccessType((param2 == 0) ? MemoryWriteCmd : MemoryReadCmd);
			pdata->setPriority(0);
			pdata->setThreadId(0);
			pdata->setAccessSize(pow(2.0, param3));

			debug("application", "sending mem access from ", id, UNIT_APP);
			//debug("application", "... to dram ", dramcfg->getAccessCore(0),
				//	UNIT_APP);
			debug("application", "... at access point ",
					destid, UNIT_APP);

			numSent++;

			return pdata;

		}/* else if (!doneSent) {
			ApplicationData* pdata = new ApplicationData();

			pdata->setType(procDone);

			pdata->setPayloadSize(128);

			debug("application", "sending procDone from ", id, UNIT_APP);

			doneSent = true;

			return pdata;
		} */else
			return NULL;
	} else
		return NULL;
}

ApplicationData* AppOne2AllDRAM::dataArrive(ApplicationData* pdata) {
	debug("application", "data arrived", UNIT_APP);

	return NULL;
}

