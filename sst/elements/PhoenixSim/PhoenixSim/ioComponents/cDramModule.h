#ifndef _CDRAMMODULE_H_
#define _CDRAMMODULE_H_
#include <vector>
#include <omnetpp.h>
#include <map>
#include <queue>
#include "messages_m.h"
#include "statistics.h"

#include "sim_includes.h"

#include "NetworkAddress.h"

#include "AddressTranslator.h"

using namespace std;

//#ifdef ENABLE_DRAMSIM
#include "DRAMsim/DRAM_Sim.h"

#define WRITE_LOCK_THRESHOLD 20

class cDramModule : public cSimpleModule, CPU_CALLBACK {


public:
	cDramModule();
	virtual ~cDramModule();

	virtual void finish();

	virtual void return_transaction(int thread_id, int trans_id);


protected:
	simtime_t clk;
	simtime_t chipClk;

	cMessage *clkMessage;

	cGate* fromNif;
	cGate* toNif;
	cGate* reqOut;
	cGate* reqIn;

	int myID;

	DRAM_Sim* dramsim;

	NetworkAddress* myAddr;

	StatObject* SO_BW;
	StatObject* SO_reads;
	StatObject* SO_writes;


	static long transactionCount;

	static AddressTranslator* translator;


	int accessSize;


	queue<MemoryAccess*> pending;

	bool readLock;
	bool writeLock;
	bool outstandingRequest;

	map<int, map<int, MemoryAccess* >* > mem_requests;
	map< MemoryAccess*, simtime_t> request_times;

	queue<ProcessorData*> theQ;

	virtual int numInitStages() const;
	virtual void initialize(int stage);
	virtual void handleMessage(cMessage *msg);

	void sendRequest(ProcessorData* pdata);

	void tryNewTrans();

	map<int, int> addrOffset;

	queue<RouterCntrlMsg*> waitingNIF;

	//private:
		//int numOfNodesX;
		//int numOfNodesY;

};
#endif

