/*
 * AppNeighbor.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppNeighbor.h"

AppNeighbor::AppNeighbor(int i, int n, double arr, int s) :
	Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;

	neighbors = 4;
	int row = id / sqrt(numOfProcs);
	int col = id % (int) sqrt(numOfProcs);

	if (row == 0 || row == sqrt(numOfProcs))
		neighbors--;

	if (col == 0 || col == sqrt(numOfProcs))
		neighbors--;

	//n = (int*) malloc(neighbors * sizeof(int));
	int cnt = 0;

	if (row - 1 >= 0)
		possibles[cnt++] = toId(row - 1, col);

	if (row + 1 < sqrt(numOfProcs))
		possibles[cnt++] = toId(row + 1, col);

	if (col - 1 >= 0)
		possibles[cnt++] = toId(row, col - 1);

	if (col + 1 < sqrt(numOfProcs))
		possibles[cnt++] = toId(row, col + 1);

}

AppNeighbor::~AppNeighbor() {

}

simtime_t AppNeighbor::process(ApplicationData* pdata) {
	return (simtime_t) exponential(arrival);
}

ApplicationData* AppNeighbor::getFirstMsg() {
	return msgCreated(NULL);

}

int AppNeighbor::toId(int row, int col) {
	return row * sqrt(numOfProcs) + col;
}

ApplicationData* AppNeighbor::msgCreated(ApplicationData* pdata) {

	if (numMessages == 0) {
		return NULL;
	}

	int which = uniform(0, neighbors);

	int dest =  possibles[which];

	ApplicationData* procdata = new ApplicationData();

	procdata->setType(MPI_send);
	procdata->setDestId(dest);
	//procdata->setDestId(15);
	procdata->setSrcId(id);

	procdata->setCreationTime(currentTime);

	if (size <= 0) {
		std::cout
				<< "Your exponential distribution parameter is wrong, Cntl-C and get out!"
				<< endl;
	}
	/*
	 int len = exponential(size);
	 while(len <= 0)
	 {
	 len = exponential(size);
	 }
	 */
	// Do static size
	int len = Application::sizeDist->getNewSize();

	procdata->setPayloadSize(len);

	if (numMessages > 0) {
		//std::cout<<numMessages<<endl;
		numMessages--;
	}

	return procdata;
}

ApplicationData* AppNeighbor::dataArrive(ApplicationData* pdata) {
	msgsRx++;

	return NULL;
}
