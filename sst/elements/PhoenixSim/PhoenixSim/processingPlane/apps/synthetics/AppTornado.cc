/*
 * AppTornado.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppTornado.h"

AppTornado::AppTornado(int i, int n, double arr, int s) :
	Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;

	neighbors = 4;
	row = id / sqrt(numOfProcs);
	col = id % (int) sqrt(numOfProcs);

	if (row == 0 || row == sqrt(numOfProcs))
		neighbors--;

	if (col == 0 || col == sqrt(numOfProcs))
		neighbors--;

	//int* n = (int*)malloc(neighbors * sizeof(int));

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

AppTornado::~AppTornado() {
	// TODO Auto-generated destructor stub
}

simtime_t AppTornado::process(ApplicationData* pdata) {
	return (simtime_t) exponential(arrival);
}

ApplicationData* AppTornado::getFirstMsg() {
	return msgCreated(NULL);

}

int AppTornado::toId(int row, int col) {
	return row * sqrt(numOfProcs) + col;
}

ApplicationData* AppTornado::msgCreated(ApplicationData* pdata) {

	if (numMessages == 0) {
		return NULL;
	}
	/*/
	 if(this->id != 0)
	 {
	 return NULL;
	 }//*/

	int which = uniform(0, neighbors);

	int start = possibles[which];

	//up to this point, just like neighbor.  now try to add more to the distance between me and the dest

	int destRow = start / sqrt(numOfProcs);
	int destCol = start % (int)sqrt(numOfProcs);

	int dest;

	if (destRow > row) {
		dest = toId(destRow + intuniform(0, sqrt(numOfProcs) - destRow - 1), destCol);
	} else if (destRow < row) {
		dest = toId(destRow - intuniform(0, destRow), destCol);
	} else if (destCol > col) {
		dest = toId(destRow, destCol + intuniform(0, sqrt(numOfProcs) - destCol - 1));
	} else if (destCol < col) {
		dest = toId(destRow, destCol - intuniform(0, destCol));
	} else{
		std::cout << "row: " << row << endl;
		std::cout << "col: " << col << endl;
		std::cout << "destRow: " << destRow << endl;
		std::cout << "destCol: " << destCol << endl;
		opp_error("something messed with tornado pattern generator ");
	}

	ApplicationData* procdata = new ApplicationData();

	procdata->setType(MPI_send);
	procdata->setDestId(dest);
	//procdata->setDestId(15);
	procdata->setSrcId(id);

	procdata->setCreationTime(currentTime);


	// Do static size
	int len = Application::sizeDist->getNewSize();

	procdata->setPayloadSize(len);

	if (numMessages > 0) {
		//std::cout<<numMessages<<endl;
		numMessages--;
	}

	return procdata;
}

ApplicationData* AppTornado::dataArrive(ApplicationData* pdata) {
	msgsRx++;

	return NULL;
}
