/*
 * AppRandom.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppDataFlow.h"

AppDataFlow::AppDataFlow(int i, int n, DRAM_Cfg* d) :
	Application(i, n, d) {
	size = param2;
	numMessages = param3;

	progress = numMessages / 10;

	int numX = sqrt(n);

	peripheral = (id < numX || id >= n - numX || id % numX == 0 || id % numX
			== numX - 1);

}

AppDataFlow::~AppDataFlow() {
	// TODO Auto-generated destructor stub
}

simtime_t AppDataFlow::process(ApplicationData* pdata) {
	/*if (pdata->getType() == cacheReadFromMemory) {
	 return 0;
	 } else {
	 return param1;
	 }*/
	return param1;
}

ApplicationData* AppDataFlow::getFirstMsg() {
	if (peripheral) {
		numMessages--;
		return readMemory();
	}

	return NULL;

}

ApplicationData* AppDataFlow::sending(ApplicationData* pdata) {

	/*if(locked){
		if(queued.size() > 0){
			locked = false;
			ApplicationData* adata = queued.front();
			queued.pop();
			ApplicationData* ret = dataArrive(adata);
			delete adata;
			return ret;
		}
	}*/

	if (peripheral) {
		//if (pdata->getType() == cacheWriteToRemoteCache) {
		if (numMessages <= 0) {
			return NULL;
		} else {
			numMessages--;
			return readMemory();
		}
		//}
	}

	return NULL;
}

ApplicationData* AppDataFlow::dataArrive(ApplicationData* pdata) {

	int numX = sqrt(numOfProcs);

	/*if (locked) {
		queued.push(pdata->dup());
		return NULL;
	} else {
		locked = true;*/

		if (pdata->getType() == M_readResponse) {

			int dest = 0;

			if (id < numX) { //top
				dest = id + numX;
			} else if (id >= numOfProcs - numX) { //bottom
				dest = id - numX;
			} else if (id % numX == 0) { //left
				dest = id + 1;
			} else if (id % numX == numX - 1) { //right
				dest = id - 1;
			} else {
				opp_error("Unexpected core receiving memory read data");
			}

			ApplicationData* adata = new ApplicationData();

			adata->setType(MPI_send);
			adata->setDestId(dest);
			adata->setSrcId(id);
			adata->setPayloadSize(pow(2.0, size));
			adata->setId(globalMsgId++);

			return adata;
		} else {

			int src = pdata->getSrcId();
			int dest = 0;

			if (src == id - 1) {
				dest = id + 1;
				if (dest / numX != id / numX) {
					return writeMemory();
				}
			} else if (src == id + 1) {
				dest = id - 1;
				if (dest / numX != id / numX) {
					return writeMemory();
				}

			} else if (src == id + numX) {
				dest = id - numX;
				if (dest < 0) {
					return writeMemory();
				}
			} else if (src == id - numX) {
				dest = id + numX;
				if (dest >= numOfProcs) {

					return writeMemory();
				}
			} else {
				opp_error("Core received a message from unexpected src");
			}

			ApplicationData* adata = new ApplicationData();

			adata->setType(MPI_send);
			adata->setDestId(dest);
			adata->setSrcId(id);
			adata->setPayloadSize(pow(2.0, size));
			adata->setId(globalMsgId++);

			return adata;

		}
	//}

	return NULL;
}

ApplicationData *AppDataFlow::writeMemory() {
	MemoryAccess* adata = new MemoryAccess();

	adata->setType(DM_writeLocal);
	adata->setDestId(id);
	adata->setSrcId(id);
	adata->setPayloadSize(pow(2.0, size));
	adata->setId(globalMsgId++);

	adata->setAddr(intuniform(0, pow(2, 30)));
	adata->setAccessType(MemoryWriteCmd);
	adata->setPriority(0);
	adata->setThreadId(0);
	adata->setAccessSize(pow(2.0, size));

	return adata;
}

ApplicationData* AppDataFlow::readMemory() {
	MemoryAccess* adata = new MemoryAccess();

	adata->setType(DM_readLocal);
	adata->setDestId(id);
	adata->setSrcId(id);
	adata->setPayloadSize(64);
	adata->setId(globalMsgId++);

	adata->setAddr(intuniform(0, pow(2, 30)));
	adata->setAccessType(MemoryReadCmd);
	adata->setPriority(0);
	adata->setThreadId(0);
	adata->setAccessSize(pow(2.0, size));

	return adata;
}
