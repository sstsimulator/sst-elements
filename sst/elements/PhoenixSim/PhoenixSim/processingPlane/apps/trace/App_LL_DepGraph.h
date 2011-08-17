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

#ifndef APP_LL_DEPGRAPH_H_
#define APP_LL_DEPGRAPH_H_

#include "Application.h"

#include <queue>

#define PACKET_SIZE 64
#define READ_THRESHOLD 4

class GraphNode {
public:

	enum OP_TYPE {
		CPU_ADD = 1,
		CPU_MUL,
		CPU_DIV,
		CPU_SUM,
		CPU_XOR,
		CPU_AND,
		CPU_SUB,
		CPU_OR,
		CPU_CMP,
		CPU_CEIL,
		CPU_FLOOR,
		CPU_CMD,
		CPU_MAX = 100,
		MEM_READ,
		MEM_WRITE,
		MEM_SEND
	};

	int node_id;

	int proc_from;
	int proc_to;



	OP_TYPE op_type;

	int num_ops;
	int op_size;

	map<int, bool> deps;

	bool executed;

	static map<int, int> done;

	static DRAM_Cfg* dramcfg;

	GraphNode(string s) {
		//parse string to get fields
		int ind = s.find(" ");
		node_id = atoi(s.substr(0, ind).c_str());
		string temp = s.substr(ind + 1);

		ind = temp.find(" ");
		proc_from = atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		ind = temp.find(" ");
		proc_to = atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		ind = temp.find(" ");
		op_type = (OP_TYPE) atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		ind = temp.find(" ");
		num_ops = atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		ind = temp.find(" ");
		op_size = atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		// Adjust the number of operations to be correct
		if (num_ops % op_size != 0) {
			opp_error("payload size must be divisible by operation size");
		}
		num_ops = num_ops / op_size;

		// If it's a cpu operation, make it a single op
		if (op_type < CPU_MAX) {
			op_size = num_ops * op_size;
			num_ops = 1;
		}

		ind = temp.find(" ");
		int numDeps = atoi(temp.substr(0, ind).c_str());
		temp = temp.substr(ind + 1);

		for (int i = 0; i < numDeps; i++) {
			ind = temp.find(" ");
			deps[atoi(temp.substr(0, ind).c_str())] = false;
			temp = temp.substr(ind + 1);
		}

		executed = false;

		//cout << "nodeid: " << node_id << " proc_from: " << proc_from << endl;
	}

	bool completed(int nid) {
		if (deps.find(nid) != deps.end()) {
			// Verify it's only being satisfied once
			if (deps[nid] == true) {
				opp_error("Dependency satisfied multiple times");
			}
			deps[nid] = true;
			return true;
		}
		return false;
	}

	bool ready() {
		map<int, bool>::iterator iter;

		bool ret = true;
		for (iter = deps.begin(); iter != deps.end(); iter++) {
			ret = ret && iter->second;
		}

		return ret;
	}

	ApplicationData* makeAppData() {
		ApplicationData* ret = NULL;

		if (op_type < CPU_MAX) {
			ret = new ApplicationData();
			ret->setType(CPU_op);
			ret->setPayloadArraySize(1);
			ret->setPayload(0, node_id);
		} else if (op_type == MEM_READ) {
			ret = new MemoryAccess();
			ret->setType((dramcfg->getAccessId(proc_to)
					== dramcfg->getAccessId(proc_from)) ? DM_readLocal
					: DM_readRemote);
			ret->setPayloadArraySize(1);
			ret->setPayload(0, node_id);

			ret->setDestId(dramcfg->getAccessId(proc_to));
			//ret->setDestId(proc_to);
			ret->setSrcId(proc_from);
			ret->setPayloadSize(64);
			ret->setId(globalMsgId++);

			((MemoryAccess*) ret)->setAddr(intuniform(0, pow(2, 30)));
			((MemoryAccess*) ret)->setAccessType(MemoryReadCmd);
			((MemoryAccess*) ret)->setPriority(0);
			((MemoryAccess*) ret)->setThreadId(0);
			((MemoryAccess*) ret)->setAccessSize(op_size * PACKET_SIZE);

		} else if (op_type == MEM_WRITE) {
			ret = new MemoryAccess();
			//ret->setType((proc_from / 4 == proc_to / 4) ? DM_writeLocal
			//		: MPI_send);
			ret->setType((proc_from == proc_to) ? DM_writeLocal : MPI_send);
			ret->setPayloadArraySize(1);
			ret->setPayload(0, node_id);

			if (ret->getType() == DM_writeLocal)
				ret->setDestId(dramcfg->getAccessId(proc_to));
			else
				ret->setDestId(proc_to);
			ret->setSrcId(proc_from);
			ret->setPayloadSize(op_size * PACKET_SIZE);
			ret->setId(globalMsgId++);

			((MemoryAccess*) ret)->setAddr(intuniform(0, pow(2, 30)));
			((MemoryAccess*) ret)->setAccessType(MemoryWriteCmd);
			((MemoryAccess*) ret)->setPriority(0);
			((MemoryAccess*) ret)->setThreadId(0);
			((MemoryAccess*) ret)->setAccessSize(op_size * PACKET_SIZE);
		} else if (op_type == MEM_SEND) {
			ret = new ApplicationData();
			ret->setType(MPI_send);
			ret->setPayloadArraySize(1);
			ret->setPayload(0, node_id);
			ret->setPayloadSize(op_size * PACKET_SIZE);
			ret->setDestId(proc_to);
			ret->setSrcId(proc_from);
			ret->setId(globalMsgId++);
		} else {
			opp_error("unknown operation type in makeAppData()");
		}

		executed = true;

		return ret;
	}

};

class App_LL_DepGraph: public Application {
public:

	App_LL_DepGraph(int i, int n, DRAM_Cfg* cfg);
	virtual ~App_LL_DepGraph() {
	}
	;

	void init();

	virtual void finish();

	virtual simtime_t process(ApplicationData* pdata); //returns the amount of time needed to process before the next message comes
	virtual ApplicationData* getFirstMsg(); //returns the next message to be processed
	virtual ApplicationData* dataArrive(ApplicationData* pdata); //data has arrived for the application
	virtual ApplicationData* msgCreated(ApplicationData* pdata);
	virtual ApplicationData* msgSent(ApplicationData* pdata);
	virtual ApplicationData* sending(ApplicationData* adata);

	void nodeCompleted(int nid);

	ApplicationData* checkQueue();
	void checkDone();

private:

	void readTrace(string fName);

	list<GraphNode*> nodes;
	map<int, GraphNode*> nodesMap;
	map<int, int> doneMap;

	queue<ApplicationData*> ready;

	int currentReadID;
	int outstandingReads;
	bool processing;

	StatObject* SO_ops;

};

#endif /* APPTRACE_LBL_H_ */
