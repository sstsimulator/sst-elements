//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "AppTrace_LBL.h"

map<int, int> AppTrace_LBL::kernelMap;
map<int, int> AppTrace_LBL::kernelReverseMap;
bool AppTrace_LBL::initKernelMap = false;

int AppTrace_LBL::numProcsDone = 0;
int AppTrace_LBL::msgsRcvd = 0;
int AppTrace_LBL::msgsSent = 0;

AppTrace_LBL::AppTrace_LBL(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	init();
}

void AppTrace_LBL::init() {

	traceFileName = param5.substr(1, param5.length() - 2);

	bool randomMapping = param1;

	currentPhase = -1;

	traceDone = false;

	synchsRcvd = 0;
	synchsSent = 0;
	waitingSynch = false;
	sendBcast = false;

	numMsgs = 0;
	numPhases = 1;

	msgsComplete = 0;

	if (!initKernelMap) {
		if (randomMapping) {
			srand(time(NULL));
			vector<int> kerns;
			for (int i = 0; i < numOfProcs; i++) {
				kerns.push_back(i);
			}

			for (int i = 0; i < numOfProcs; i++) {
				int r = rand() % kerns.size();
				kernelMap[i] = kerns[r];
				kernelReverseMap[kernelMap[i]] = i;
				kerns.erase(kerns.begin() + r);

				debug("app", "kernel ", kernelMap[i], UNIT_APP);
				debug("app", "... mapped to ", i, UNIT_APP);
			}

		} else {
			for (int i = 0; i < numOfProcs; i++) {
				kernelMap[i] = i;
				kernelReverseMap[i] = i;

			}

		}

		initKernelMap = true;
	}

	//----Send initial schedule to all NIFs and switch controllers------
	/*int numX = par("numOfNodesX");
	int numTDMslots = (numX - 1) * (numX / 2);
	cModule* target = getParentModule()->getSubmodule("NIF_PhotonicETDM");
*/
	readNextPhase();

}

void AppTrace_LBL::finish() {
	if (traceFile.is_open()) {
		traceFile.close();
	}
}

simtime_t AppTrace_LBL::process(ApplicationData* pdata) {
	return 0;
}

ApplicationData* AppTrace_LBL::getFirstMsg() {
	return msgCreated(NULL);
}

ApplicationData* AppTrace_LBL::msgCreated(ApplicationData* created) {

	if (sendBcast) {
		return synch();

	} else if (!waitingSynch) {
		return trace();
	}

	return NULL;
}

ApplicationData* AppTrace_LBL::dataArrive(ApplicationData* pdata) {
	msgsRcvd++;

	if (numProcsDone >= numOfProcs && msgsRcvd == msgsSent) {
		Statistics::done();
	}

	if (pdata->getType() == procSynch) {
		if (kernelMap[id] == 0) {
			synchsRcvd++;

			debug("app", "synch rxed at home ", id, UNIT_APP);

			if (synchsRcvd == numOfProcs) {
				sendBcast = true;
				if (!traceDone)
					readNextPhase();

				return synch();
			}
		} else {
			waitingSynch = false;

			debug("app", "synch rxed, continue ", id, UNIT_APP);

			if (!traceDone)
				readNextPhase();

			ApplicationData* a = trace();

			return a;
		}

	}

	return NULL;
}

ApplicationData* AppTrace_LBL::trace() {
	if (scheduled.size() > 0) {

		pair<int, int> info = scheduled.front();

		ApplicationData* pdata = new ApplicationData();

		pdata->setType(MPI_send);
		pdata->setDestId(kernelReverseMap[info.first]);
		pdata->setSrcId(id);
		pdata->setPayloadArraySize(0);
		pdata->setPayloadSize(info.second * 8);

		debug("app", "physical app ", id, UNIT_APP);
		debug("app", "... with kernel id ", kernelMap[id], UNIT_APP);
		debug("app", "... sending to kernel ", info.first, UNIT_APP);
		debug("app", "... at physical ", kernelReverseMap[info.first], UNIT_APP);
		debug("app", "... with size ", info.second, UNIT_APP);

		bytesTransferred += info.second;

		scheduled.pop_front();

		msgsSent++;

		return pdata;
	} else if (!traceDone) {
		if (kernelMap[id] == 0) {
			synchsRcvd++;
			return NULL;
		}

		ApplicationData* pdata = new ApplicationData();

		pdata->setType(procSynch);
		pdata->setDestId(kernelReverseMap[0]);
		pdata->setSrcId(id);
		pdata->setPayloadArraySize(0);
		pdata->setPayloadSize(32);

		waitingSynch = true;

		msgsSent++;

		return pdata;
	} else {
		numProcsDone++;

		debug("app", "procsDone = ", numProcsDone, UNIT_APP);
		debug("app", "msgsRcvd= ", msgsRcvd, UNIT_APP);
		debug("app", "msgsSent= ", msgsSent, UNIT_APP);

		if (numProcsDone >= numOfProcs && msgsRcvd == msgsSent) {
			Statistics::done();
		}
	}

	return NULL;
}

ApplicationData* AppTrace_LBL::synch() {

	debug("app", "sendBcast ", id, UNIT_APP);
	synchsSent++;
	ApplicationData* pdata = new ApplicationData();

	pdata->setType(procSynch);
	pdata->setDestId(kernelReverseMap[synchsSent]);
	pdata->setSrcId(id);
	pdata->setPayloadArraySize(0);
	pdata->setPayloadSize(32);

	if (synchsSent == numOfProcs - 1) {
		sendBcast = false;
		synchsRcvd = 0;
		synchsSent = 0;
	}

	msgsSent++;

	return pdata;
}

void AppTrace_LBL::readNextPhase() {

	if (!traceFile.is_open()) {

		traceFile.open(traceFileName.c_str(), ios::in);
		traceFile >> currentPhase;
		traceFile.get();
	}

	if (!traceFile.is_open()) {
		std::cout << "Filename: " << traceFileName << endl;
		opp_error("cannot open trace file");
	}

	debug("app", "reading phase ", currentPhase, UNIT_APP);

	int src, dest, size, phase;
	messagesLeftInPhase = 0;
	do {
		traceFile >> src;
		traceFile.get();
		traceFile >> dest;
		traceFile.get();
		traceFile >> size;
		traceFile.get();
		traceFile >> phase;
		traceFile.get();
		//std::cout<<src<<" "<<dest<<" "<<size<<" "<<endl;

		if (src == kernelMap[id] && src != dest) {
			messagesLeftInPhase++;
			scheduled.push_back(pair<int, int> (dest, size));
		}

	} while (!traceFile.eof() && phase == currentPhase);

	msgsArrived = messagesLeftInPhase;

	if (traceFile.eof())
		traceDone = true;

	currentPhase = phase; //currentPhase will always point to the next phase

}


