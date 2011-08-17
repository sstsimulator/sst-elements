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

#ifndef APPTRACE_LBL_H_
#define APPTRACE_LBL_H_

#include "Application.h"

class AppTrace_LBL: public Application {
public:

	AppTrace_LBL(int i, int n, DRAM_Cfg* cfg);
	virtual ~AppTrace_LBL(){};

	void init();

	virtual void finish();

	virtual simtime_t process(ApplicationData* pdata);  //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg();  //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata);  //data has arrived for the application
	virtual ApplicationData* msgCreated(ApplicationData* pdata);


private:

	void readNextPhase();
	void readControlFile(string fName);

	ApplicationData* synch();
	ApplicationData* trace();

	string traceFileName;
	ifstream traceFile;

	bool traceDone;

	int currentPhase;
	int messagesLeftInPhase;

	// destId, size
	list< pair<int, int> > scheduled;

	static map<int, int> kernelMap;
	static map<int, int> kernelReverseMap;
	static bool initKernelMap;

	static int numProcsDone;
	static int msgsRcvd;
	static int msgsSent;

	int msgsComplete;
	int msgsArrived;

	int numPhases;
	unsigned long numMsgs;

	bool waitingSynch;
	bool sendBcast;
	int synchsRcvd;
	int synchsSent;

};

#endif /* APPTRACE_LBL_H_ */
