/*
 * AppOne2OneDRAM.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppDRAMOne2One.h"



AppOne2OneDRAM::AppOne2OneDRAM(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppOne2OneDRAM::init() {

	numSent = 0;
	doneSent = false;

}

AppOne2OneDRAM::~AppOne2OneDRAM() {

}

simtime_t AppOne2OneDRAM::process(ApplicationData* pdata) {

	return 0;
}

ApplicationData* AppOne2OneDRAM::getFirstMsg() {
	return sending(NULL);
}

ApplicationData* AppOne2OneDRAM::sending(ApplicationData* adata){

	if(!useioplane){
		opp_error("you can't use one2oneDRAM without enabling the ioplane");
	}
	if (id == param4) {
		if (numSent < param1) {
			MemoryAccess* pdata = new MemoryAccess();

			int dest = dramcfg->getAccessId(id);
			//int dest = 0;

			//pdata->setType((param2 == 0) ? cacheWriteToMemory
			//		: cacheReadFromMemory);
			pdata->setType((param2 == 0) ? DM_writeLocal : DM_readLocal);
			pdata->setDestId(dest);
			//pdata->setDestId(1);
			pdata->setSrcId(id);
			pdata->setPayloadSize((param2 == 0) ? pow(2.0, param3) : 64);

			pdata->setAddr(rand());
			pdata->setAccessType((param2 == 0) ? MemoryWriteCmd : MemoryReadCmd);
			pdata->setPriority(0);
			pdata->setThreadId(0);
			pdata->setAccessSize(pow(2.0, param3));

			debug("application", "sending mem access from ", id, UNIT_APP);
			//debug("application", "... to dram ", dramcfg->getAccessCore(id),
			//		UNIT_APP);
			debug("application", "... to access point ",
					dest, UNIT_APP);

			numSent++;

			return pdata;

		/*} else if (!doneSent) {
			ApplicationData* pdata = new ApplicationData();

			pdata->setType(procDone);

			pdata->setPayloadSize(128);

			debug("application", "sending procDone from ", id, UNIT_APP);

			doneSent = true;

			return pdata;
		*/} else
			return NULL;
	} else
		return NULL;
}

ApplicationData* AppOne2OneDRAM::dataArrive(ApplicationData* pdata) {
	debug("application", "data arrived", UNIT_APP);

	return NULL;
}

