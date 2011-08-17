/*
 * AppRandom.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppDRAMRandom.h"

AppRandomDRAM::AppRandomDRAM(int i, int n, DRAM_Cfg* d) :
	Application(i, n, d) {
	arrival = param1;

	if (param3 < 0)
		numMessages = 1;
	else
		numMessages = param3;

	progress = numMessages / 10;

	std::cout << "Application: RandomDRAM reporting for duty" << endl;

	locked = false;

}

AppRandomDRAM::~AppRandomDRAM() {
	// TODO Auto-generated destructor stub
}

simtime_t AppRandomDRAM::process(ApplicationData* pdata) {
	return (simtime_t) exponential(arrival);
}

ApplicationData* AppRandomDRAM::getFirstMsg() {

	return sending(NULL);

}

ApplicationData* AppRandomDRAM::sending(ApplicationData* pdata) {

	if (numMessages == 0 || locked) {
		return NULL;
	}

	int dest;

	/*if (param4 == 0) {
	 dest = dramcfg->getAccessId(id);
	 } else {
	 dest = dramcfg->getAccessCore(intuniform(0, dramcfg->getNumDRAM() - 1));
	 }*/

	bool loc = intuniform(0, 9) >= 5;

	if (loc)
		dest = dramcfg->getAccessId(id);
	else{
		do{

			dest = dramcfg->getAccessCore(intuniform(0, dramcfg->getNumDRAM() - 1));
		}while(dest / dramcfg->procConc == id / dramcfg->procConc);
	}

	debug("app", "memory access from ", id, UNIT_APP);
	debug("app", "to ", dest, UNIT_APP);

	MemoryAccess* adata = new MemoryAccess();

	int t = intuniform(0, 1);

	//adata->setType(t == 0 ? ((dest / dramcfg->procConc == id
	//		/ dramcfg->procConc) ? DM_readLocal : DM_readRemote) : ((dest
	//		/ dramcfg->procConc == id / dramcfg->procConc) ? DM_writeLocal
	//		: DM_writeRemote));

	adata->setType(t == 0 ? ((loc) ? DM_readLocal : DM_readRemote) : ((loc) ? DM_writeLocal
			: DM_writeRemote));

	int size = sizeDist->getNewSize();
	adata->setDestId(dest);
	adata->setSrcId(id);
	adata->setPayloadSize(t == 0 ? 64 :  size);
	adata->setId(globalMsgId++);

	adata->setAddr(intuniform(0, pow(2, 30)));
	adata->setAccessType(t == 0 ? MemoryReadCmd : MemoryWriteCmd);
	adata->setPriority(0);
	adata->setThreadId(0);
	adata->setAccessSize(size);

	if (numMessages > 0 && param3 > 0) {
		numMessages--;
	}

	if (adata->getType() == DM_readRemote) {
		locked = true;
	}

	return adata;

}

ApplicationData* AppRandomDRAM::dataArrive(ApplicationData* pdata) {
	msgsRx++;

	debug("app", "message received", UNIT_APP);

	if (locked && pdata->getType() == M_readResponse) {
		locked = false;
		return sending(NULL);
	}

	return NULL;
}
