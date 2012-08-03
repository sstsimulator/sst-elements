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

#include "App_LL_DepGraph.h"

map<int, int> GraphNode::done;

DRAM_Cfg* GraphNode::dramcfg;

App_LL_DepGraph::App_LL_DepGraph(int i, int n, DRAM_Cfg* cfg) :
	Application(i, n, cfg) {

	GraphNode::dramcfg = cfg;

	init();
}

void App_LL_DepGraph::init() {

	string fileName = param5.substr(1, param5.length() - 2);

	readTrace(fileName);

	currentReadID = -1;
	processing = false;

	outstandingReads = 0;

	SO_ops = Statistics::registerStat("GOPS", StatObject::TIME_AVG,
			"application");
}

void App_LL_DepGraph::finish() {
	map<int, GraphNode*>::iterator iter;

	for (iter = nodesMap.begin(); iter != nodesMap.end(); iter++) {
		GraphNode *target = (*iter).second;

		if (!target->executed) {
			std::cout << "app " << id << ": node " << target->node_id
					<< " not executed.  Deps not satisfied: ";
			map<int, bool>::iterator iter;
			for (iter = target->deps.begin(); iter != target->deps.end(); iter++) {
				if (iter->second == false)
					std::cout << iter->first << " ";
			}
			std::cout << endl;
		}
	}
}

simtime_t App_LL_DepGraph::process(ApplicationData* pdata) {
	if (pdata->getType() == CPU_op) {
		processing = true;
		//return how much time the op takes
		int nid = pdata->getPayload(0);
		debug("app", "processing CPU_op ", nid, UNIT_APP);

		SO_ops->track(nodesMap[nid]->op_size / 1e9);

		switch (nodesMap[nid]->op_type) {
		case GraphNode::CPU_ADD:
			//case GraphNode::CPU_MUL:
			//case GraphNode::CPU_DIV:
		case GraphNode::CPU_SUM:
		case GraphNode::CPU_XOR:
		case GraphNode::CPU_AND:
		case GraphNode::CPU_SUB:
		case GraphNode::CPU_OR:
		case GraphNode::CPU_CMP:
			//case GraphNode::CPU_CEIL:
			//case GraphNode::CPU_FLOOR:
		case GraphNode::CPU_CMD:
			return clockPeriod * nodesMap[nid]->op_size;

			//case GraphNode::CPU_ADD:
		case GraphNode::CPU_MUL:
		case GraphNode::CPU_DIV:
			//case GraphNode::CPU_SUM:
			//case GraphNode::CPU_XOR:
			//case GraphNode::CPU_AND:
			//case GraphNode::CPU_SUB:
			//case GraphNode::CPU_OR:
			//case GraphNode::CPU_CMP:
		case GraphNode::CPU_CEIL:
		case GraphNode::CPU_FLOOR:
			//case GraphNode::CPU_CMD:
			return clockPeriod * (5 + nodesMap[nid]->op_size);

		default:
			opp_error("unknown CPU_op in process()");

		}
	}

	else {
		return 0;
	}
}

ApplicationData* App_LL_DepGraph::getFirstMsg() {
	nodeCompleted(-1);

	return checkQueue();
}

ApplicationData* App_LL_DepGraph::msgCreated(ApplicationData* adata) {

	return checkQueue();
}

ApplicationData* App_LL_DepGraph::sending(ApplicationData* adata) {
	if (adata->getType() == DM_writeLocal || adata->getType() == MPI_send) {
		int nid = adata->getPayload(0);
		debug("app", "write sending ", nid, UNIT_APP);

		// Only mark as completed if this is the last send
		if (doneMap[nid] == 1)
			nodeCompleted(nid);

		doneMap[nid]--;

		//set that it's done if this was a local write, or a write completed at the remote node
		//if (adata->getType() == DM_writeLocal){
	}
	return checkQueue();
}

ApplicationData* App_LL_DepGraph::msgSent(ApplicationData* adata) {
	/*if (adata->getType() == DM_writeLocal || adata->getType() == MPI_send) {
	 //data arrived, go ahead
	 int nid = adata->getPayload(0);
	 debug("app", "write complete (sent) ", nid, UNIT_APP);

	 if (GraphNode::done[nid] < 0) {
	 std::cout << "node_id: " << nid << endl;
	 opp_error("Already completed send/remote write");
	 }
	 if (GraphNode::done[nid] == 0)
	 checkDone();

	 }*/

	if (adata->getType() == DM_writeLocal) {
		int nid = adata->getPayload(0);
		debug("app", "write complete (sent) ", nid, UNIT_APP);

		GraphNode::done[nid]--;

		if (GraphNode::done[nid] < 0) {
			std::cout << "node_id: " << nid << endl;
			opp_error("Already completed send/remote write");
		}
		if (GraphNode::done[nid] == 0)
			checkDone();
	}

	return checkQueue();

}

ApplicationData* App_LL_DepGraph::dataArrive(ApplicationData* adata) {
	if (adata->getType() == M_readResponse || adata->getType() == MPI_send
			|| adata->getType() == CPU_op) {
		//data arrived, go ahead
		int nid;

		if (adata->getType() == M_readResponse) {

			//nid = currentReadID;
			//currentReadID = -1;

			outstandingReads--;

			if (outstandingReads < 0) {
				opp_error("App_LL_DepGraph: too many read responses received.");
			}

			nid = adata->getPayload(0);
			debug("app", "read response arrived ", nid, UNIT_APP);
		} else if (adata->getType() == CPU_op) {
			nid = adata->getPayload(0);
			debug("app", "CPU op finished ", nid, UNIT_APP);
		} else {
			nid = adata->getPayload(0);
			debug("app", "MPI msg arrived ", nid, UNIT_APP);
		}

		if (nid < 0) {
			std::cout << "id: " << id << endl;
			std::cout << "msg type: " << adata->getType() << endl;
			opp_error("application data payload not set correctly");
		}

		if (adata->getType() == CPU_op)
			processing = false;

		if (GraphNode::done[nid] == 1)
			nodeCompleted(nid);

		// Verify the destination is correct
		if (adata->getDestId() != id && adata->getType() == MPI_send)
			opp_error("Received message at wrong destination");

		//if (adata->getType() != MPI_send) {
		GraphNode::done[nid]--;
		if (GraphNode::done[nid] < 0) {
			std::cout << "node_id: " << nid << endl;

			opp_error("Already completed send/remote write");
		}
		if (GraphNode::done[nid] == 0)
			checkDone();
		//}
	}

	return checkQueue();
}

void App_LL_DepGraph::nodeCompleted(int nid) {

	map<int, GraphNode*>::iterator iter;
	static int depNodeCount = 0;

	for (iter = nodesMap.begin(); iter != nodesMap.end(); iter++) {
		GraphNode *target = (*iter).second;
		// Only consider it if the dependencies have been modified
		if (nid == -1 || target->completed(nid)) {
			if (target->ready()) {
				// Make sure it hasn't been executed previously
				if (target->executed) {
					opp_error("Graph node has already been executed");
				}
				// Make an operation for each op in the node
				cout << "Launching node " << ++depNodeCount << ": id "
						<< target->node_id << endl;
				for (int op = 0; op < target->num_ops; op++)
					ready.push(target->makeAppData());
			}
		}
	}
}

void App_LL_DepGraph::checkDone() {
	map<int, int>::iterator iter;

	for (iter = GraphNode::done.begin(); iter != GraphNode::done.end(); iter++) {
		if (iter->second > 0)
			return;
	}

	cout << "Simulation Time: " << simTime() << endl;
	Statistics::done();
}

ApplicationData* App_LL_DepGraph::checkQueue() {

	if (ready.size() > 0) {
		ApplicationData* ret = ready.front();

		if (ret->getType() == DM_readLocal || ret->getType() == DM_readRemote) {
			if (outstandingReads < READ_THRESHOLD) {
				debug("app", "starting a read at ", id, UNIT_APP);
				debug("app", "proc_to ", ret->getDestId(), UNIT_APP);
				//currentReadID = ret->getPayload(0);
				outstandingReads++;
				ready.pop();
				return ret;
			} else {
				return NULL;
			}
		} else if (ret->getType() == CPU_op) {
			if (!processing) {
				debug("app", "starting a cpu op at ", id, UNIT_APP);
				ready.pop();
				return ret;
			}
		} else {
			debug("app", "starting a write/send at ", id, UNIT_APP);
			debug("app", "proc_to ", ret->getDestId(), UNIT_APP);
			ready.pop();
			return ret;

		}

	}

	return NULL;
}

void App_LL_DepGraph::readTrace(string fileName) {

	//read file in, save it
	ifstream traceFile;
	unsigned long localCount = 0;

	if (!traceFile.is_open()) {

		traceFile.open(fileName.c_str(), ios::in);
	}

	if (!traceFile.is_open()) {
		std::cout << "Filename: " << fileName << endl;
		opp_error("cannot open trace file");
	}

	string s;
	while (!traceFile.eof()) {
		getline(traceFile, s);

		if (s.size() >= 14) {

			GraphNode *n = new GraphNode(s);

			if (n->proc_from == id) {
				localCount++;
				GraphNode::done[n->node_id] = n->num_ops;
				nodesMap[n->node_id] = n;
				doneMap[n->node_id] = n->num_ops;
			} else {
				delete n;
			}
		}
	}

	if (!traceFile.is_open()) {
		traceFile.close();
	}

	cout << "Loaded DG on processor " << id << " with " << localCount
			<< " local nodes" << endl;

}
