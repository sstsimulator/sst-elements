/*
 * AppFFT.cc
 *
 *  Created on: Feb 16, 2009
 *      Author: SolidSnake
 */

#include "AppFFTstream.h"

#include "DRAM_Cfg.h"

AppFFTstream::AppFFTstream(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n,  dramcfg) {

	dramcfg = cfg;

	init(n, -1);

}


AppFFTstream::~AppFFTstream() {



}

void AppFFTstream::init(int n, int start) {
	//correct if number of cores is not a power of 2
	int x = sqrt(n);
	int y = sqrt(n);

	if ((int) log2(x * y) != log2(x * y)) {
		std::cout
				<< "WARNING: App FFT: number of cores is not a power of 2, correcting...";
		numKernels = (int) pow(2.0, (int) log2(x * y));
		std::cout << " numKernels = " << numKernels << endl;

		int extras = x * y - numKernels;

		std::cout << " extra cores = " << extras << endl;

		int exDim = (int) sqrt(extras); //the dimensions of the square of extra cores in the middle
		int squareStart = ((y - exDim) / 2) * x + ((x - exDim) / 2);
		int sqStX = squareStart % x;
		int sqStY = squareStart / x;
		int squareEnd = ((y - exDim) - 1) * x + ((x - exDim) - 1);
		int sqEnX = squareEnd % x;
		int sqEnY = squareEnd / x;
		int myX = id % x;
		int myY = id / x;

		if (id < squareStart)
			kernelId = id;
		else if (myX >= sqStX && myX <= sqEnX && myY >= sqStY && myY <= sqEnY) //i'm in the extras square
			kernelId = -1;
		else if (myY <= sqEnY && myX < sqStX)
			kernelId = id - (((myY - 1) - sqStY) + 1) * exDim; //to the left of the square
		else if (myY <= sqEnY && myX >= sqStX + exDim)
			kernelId = id - ((myY - sqStY) + 1) * exDim; //to the right of the square
		else
			kernelId = id - ((sqEnY - sqStY) + 1) * exDim; //past the square

	} else {
		numKernels = x * y;
		kernelId = id;
	}

	datasize = pow(2, param1);

	sentDone = false;

	busy = false;

	firstReadSent = false;

	writeAddr = 0;
	firstFFT = NULL;


}

simtime_t AppFFTstream::process(ApplicationData* pdata) {

	//if (pdata->getType() != procDone) {
		int stage = pdata->getPayload(0);
		if (stage <= 0)
			return 0;
		else if (stage == 1)
			return .043 * pdata->getPayloadSize() / pow(2, 31); //linearly scale the computation time down from what we know at 43ms for 256MB
		else
			return .0018 * pdata->getPayloadSize() / pow(2, 31);
	//} else
	//	return 0;
}

ApplicationData* AppFFTstream::sending(ApplicationData* pdata) {
	//if (pdata->getType() != procDone) {
		int stage = pdata->getPayload(0);
		if (stage != 0) //a read message, nothing
			busy = false;
	//}
  return NULL; // Added to avoid compile warning
}

ApplicationData* AppFFTstream::getFirstMsg() {
	return getNextMsg();
}

ApplicationData* AppFFTstream::getNextMsg() {

	if (kernelId >= 0) {

		if (!firstReadSent) {
			MemoryAccess* pdata = new MemoryAccess();

			pdata->setType(SM_read);
			pdata->setDestId(dramcfg->getAccessId(id));
			pdata->setSrcId(id);
			pdata->setPayloadArraySize(1);
			pdata->setPayload(0, -1);
			pdata->setPayloadSize(MEM_READ_SIZE);
			//pdata->setDataID((int) (datasize / param2)); ////NEED TO WORK THIS IN (PAYLOAD) TO MAKE THE APP WORK!!

			pdata->setAddr((int) datasize);
			pdata->setAccessType(MemoryReadCmd);
			pdata->setPriority(0);
			pdata->setThreadId(0);
			pdata->setAccessSize(param2);

			datasize -= param2;

			firstReadSent = true;

			debug("app", "reading from memory ", dramcfg->getAccessId(id),
					UNIT_APP );
			debug("app", "... from  ", id, UNIT_APP);

			return pdata;
		} else if (firstFFT != NULL) {
			ApplicationData* tmp = firstFFT;
			firstFFT = NULL;
			busy = true;
			bytesTransferred += param2;
			debug("app", "starting FFT stage at ", id, UNIT_APP );
			return tmp;

		} else if (messages.size() > 0) {

			if (!busy) {

				ApplicationData* next = messages.front();

				int stage = next->getPayload(0);
				//int dId = next->getDataID();

				messages.pop();
				delete next;

				if (stage == -1)
					stage++;

				if (stage == 0) {
					ApplicationData* ret;

					int part = kernelId / (numKernels / (pow(2.0, stage)));
					int destNode = ((kernelId + numKernels / ((int) pow(2.0,
							stage + 1))) % (numKernels
							/ ((int) pow(2.0, stage)))) + part * (numKernels
							/ ((int) pow(2.0, stage)));
					//Statistics::traffic << stage << ", " << id << ", "
					//	<< destNode << endl;

					ApplicationData* p = new ApplicationData();

					p->setType(MPI_send);
					p->setDestId(destNode);
					p->setSrcId(id);
					p->setPayloadArraySize(1);
					p->setPayload(0, stage + 1);
					p->setPayloadSize(param2);

					if (datasize >= 0) {
						MemoryAccess* pdata = new MemoryAccess();

						pdata->setType(SM_read);
						pdata->setDestId(dramcfg->getAccessId(id));
						pdata->setSrcId(id);
						pdata->setPayloadArraySize(1);
						pdata->setPayload(0, 0);
						pdata->setPayloadSize(MEM_READ_SIZE);
						//pdata->setDataID((int) (datasize / param2));

						pdata->setAddr((int) datasize);
						pdata->setAccessType(MemoryReadCmd);
						pdata->setPriority(0);
						pdata->setThreadId(0);
						pdata->setAccessSize(param2);

						datasize -= param2;

						bytesTransferred += param2;
						debug("app", "reading from memory ",
								dramcfg->getAccessId(id), UNIT_APP );
						debug("app", "... from  ", id, UNIT_APP);

						ret = pdata;
						firstFFT = p;
					} else {
						ret = p;
					}

					return ret;

				} else if (stage < log2(numKernels)) {
					int part = kernelId / (numKernels / (pow(2.0, stage)));
					int destNode = ((kernelId + numKernels / ((int) pow(2.0,
							stage + 1))) % (numKernels
							/ ((int) pow(2.0, stage)))) + part * (numKernels
							/ ((int) pow(2.0, stage)));
					//Statistics::traffic << stage << ", " << id << ", "
					//	<< destNode << endl;

					ApplicationData* pdata = new ApplicationData();

					pdata->setType(MPI_send);
					pdata->setDestId(destNode);
					pdata->setSrcId(id);
					pdata->setPayloadArraySize(1);
					pdata->setPayload(0, stage + 1);
					pdata->setPayloadSize(param2);

					bytesTransferred += param2;

					debug("app", "sending to proc ", destNode, UNIT_APP );
					debug("app", "... from  ", id, UNIT_APP);

					busy = true;

					return pdata;

				} else if (stage == log2(numKernels)) { //the memory write stage

					MemoryAccess* pdata = new MemoryAccess();

					pdata->setType(SM_write);
					pdata->setDestId(dramcfg->getAccessId(id));
					pdata->setSrcId(id);
					pdata->setPayloadSize(param2);
					pdata->setPayloadArraySize(1);
					pdata->setPayload(0, stage + 1);

					pdata->setAddr(writeAddr);
					pdata->setAccessType(MemoryWriteCmd);
					pdata->setPriority(0);
					pdata->setThreadId(0);
					pdata->setAccessSize(param2);

					bytesTransferred += param2;

					writeAddr += param2;

					busy = true;

					debug("app", "writing to memory ",
							dramcfg->getAccessId(id), UNIT_APP );
					debug("app", "... from  ", id, UNIT_APP);

					return pdata;

				} /*else { //we're done
					if (!sentDone) {
						ApplicationData* pdata = new ApplicationData();

						pdata->setType(procDone);

						pdata->setPayloadSize(32);

						debug("app", "sending procDone from ", id, UNIT_APP );

						sentDone = true;
						return pdata;
					}
				}*/
			}
		}

	}

	return NULL;
}

ApplicationData* AppFFTstream::dataArrive(ApplicationData* pdata) {

	debug("app", "receivied message at ", id, UNIT_APP );
	debug("app", "...from ", pdata->getSrcId(), UNIT_APP );

	messages.push(pdata->dup());

	return getNextMsg();

}
