/*
 * AppFFT.cc
 *
 *  Created on: Feb 16, 2009
 *      Author: SolidSnake
 */

#include "AppFFT.h"

#include "DRAM_Cfg.h"

AppFFT::AppFFT(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init(n, -1);
}

AppFFT::~AppFFT() {

}

void AppFFT::init(int n, int start) {
	//correct if number of cores is not a power of 2
	stage = start;
	int y = sqrt(n);
	int x = y;

	if ((int) log2(n) != log2(n)) {
		std::cout
				<< "WARNING: App FFT: number of cores is not a power of 2, correcting...";
		numKernels = (int) pow(2.0, (int) log2(n));
		std::cout << " numKernels = " << numKernels << endl;

		int extras = n - numKernels;

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

	for (int i = start + 1; i <= log2(numKernels) + 1; i++) {
		rcvd[i] = false;
	}
	rcvd[start] = true;

	datasize = pow(2, param1);

	doRead = ((int)param4 & 0x1) > 0;
	doFFT = ((int)param4 & 0x2) > 0;
	doWrite = ((int)param4 & 0x4) > 0;

	if(doFFT && !doRead){
		stage = 0;
		rcvd[stage] = true;
	}else if(doWrite && !doFFT){
		stage = log2(numKernels);
		rcvd[stage] = true;
	}

	sentDone = false;

}

simtime_t AppFFT::process(ApplicationData* pdata) {

	/*if (pdata->getType() == procDone) {
		return 0;
	} else */if (stage == 0 || !doFFT)
		return 0;
	else if (stage == 1)
		return param2; //the Pentium M on 256K samples
	else
		return param3; //a proportion of   (FFTtime) : (5NlogN) = (linearCombTime) : (5N)    where N is the #samples
}

ApplicationData* AppFFT::getFirstMsg() {
	return dataArrive(NULL);
}

ApplicationData* AppFFT::dataArrive(ApplicationData* adata) {

	if (adata != NULL) {
		if (adata->getType() == M_readResponse) {
			rcvd[0] = true;
		} else {
			int st = adata->getPayload(0);
			rcvd[st] = true;
		}
	}

	debug("AppFFT", "Received a message", UNIT_APP);

	if (kernelId >= 0) {
		upherelabel:

		if (stage < 0 && doRead) { //the memory read stage
			if (useioplane) {
				MemoryAccess* pdata = new MemoryAccess();

				pdata->setType(SM_read);
				pdata->setDestId(dramcfg->getAccessId(id));
				pdata->setSrcId(id);
				pdata->setPayloadSize(MEM_READ_SIZE);

				pdata->setAddr(0);
				pdata->setAccessType(MemoryReadCmd);
				pdata->setPriority(0);
				pdata->setThreadId(0);
				pdata->setAccessSize(datasize);

				bytesTransferred += datasize;

				debug("app", "reading from memory ", dramcfg->getAccessId(id),
						UNIT_APP );
				debug("app", "... from  ", id, UNIT_APP);

				stage++;
				return pdata;
			} else {
				stage++;
				goto upherelabel;
			}

		} else if (stage < log2(numKernels) && doFFT) { //data transfer stage
			if (rcvd[stage]) {

				int part = kernelId / (numKernels / (pow(2.0, stage)));

				int destNode = ((kernelId + numKernels / ((int) pow(2.0, stage
						+ 1))) % (numKernels / ((int) pow(2.0, stage)))) + part
						* (numKernels / ((int) pow(2.0, stage)));

				ApplicationData* pdata = new ApplicationData();

				pdata->setType(MPI_send);
				pdata->setDestId(destNode);
				pdata->setSrcId(id);
				pdata->setPayloadArraySize(1);
				pdata->setPayload(0, stage + 1);
				pdata->setPayloadSize(datasize);

				bytesTransferred += datasize;

				stage++;

				debug("app", "sending to proc ", destNode, UNIT_APP );
				debug("app", "... from  ", id, UNIT_APP);

				return pdata;
			} else
				return NULL;

		} else if (useioplane) {
			if (stage == log2(numKernels) && doWrite) { //the memory write stage
				if (rcvd[stage]) {
					MemoryAccess* pdata = new MemoryAccess();

					pdata->setType(SM_write);
					pdata->setDestId(dramcfg->getAccessId(id));
					pdata->setSrcId(id);
					pdata->setPayloadSize(datasize);

					pdata->setAddr(128);
					pdata->setAccessType(MemoryWriteCmd);
					pdata->setPriority(0);
					pdata->setThreadId(0);
					pdata->setAccessSize(datasize);

					bytesTransferred += datasize;

					stage++;

					debug("app", "writing to memory ",
							dramcfg->getAccessId(id), UNIT_APP );
					debug("app", "... from  ", id, UNIT_APP);

					return pdata;
				}
			} /*else { //we're done
				if (!sentDone) {
					ApplicationData* pdata = new ApplicationData();

					pdata->setType(procDone);
					pdata->setDestId(id);
					pdata->setSrcId(id);
					pdata->setPayloadSize(32);

					debug("app", "sending procDone from ", id, UNIT_APP );

					sentDone = true;
					return pdata;
				}
			}*/
		}

	}

	return NULL;

}
