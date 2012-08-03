/*
 * AppAll2OneDRAM.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppDRAMAll2One.h"


AppAll2OneDRAM::AppAll2OneDRAM(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppAll2OneDRAM::init() {

	numSent = 0;
	doneSent = false;


}

AppAll2OneDRAM::~AppAll2OneDRAM() {

}

simtime_t AppAll2OneDRAM::process(ApplicationData* pdata) {

	return 0;
}

ApplicationData* AppAll2OneDRAM::getFirstMsg() {
	return sending(NULL);
}

ApplicationData* AppAll2OneDRAM::sending(ApplicationData* adata){

	if (numSent < param1) {
		MemoryAccess* pdata = new MemoryAccess();

		pdata->setType((param2 == 0) ? DM_writeRemote : DM_readRemote);
		pdata->setDestId(dramcfg->getAccessCore(0));
		pdata->setSrcId(id);
		pdata->setPayloadSize((param2 == 0) ? pow(2.0, param3) : 64);
		pdata->setId(globalMsgId++);

		pdata->setAddr(0);
		pdata->setAccessType((param2 == 0) ? MemoryWriteCmd : MemoryReadCmd);
		pdata->setPriority(0);
		pdata->setThreadId(0);
		pdata->setAccessSize(pow(2.0, param3));

		debug("application", "sending mem access from ", id, UNIT_APP);
		debug("application", "... to dram ", dramcfg->getAccessCore(0),
				UNIT_APP);
		debug("application", "... at access point ", dramcfg->getAccessCore(0),
				UNIT_APP);

		numSent++;

		return pdata;

	} /*else if (!doneSent) {
		ApplicationData* pdata = new ApplicationData();

		pdata->setType(procDone);

		pdata->setPayloadSize(128);

		debug("application", "sending procDone from ", id, UNIT_APP);

		doneSent = true;

		return pdata;
	} */else
		return NULL;
}

ApplicationData* AppAll2OneDRAM::dataArrive(ApplicationData* pdata) {
	return NULL;
}

