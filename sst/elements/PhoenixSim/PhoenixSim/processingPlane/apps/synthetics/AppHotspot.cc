/*
 * AppHotspot.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppHotspot.h"

int AppHotspot::spot = -1;

AppHotspot::AppHotspot(int i, int n, double arr, int s) :
	Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;

	if (spot < 0) {
		spot = uniform(0, numOfProcs);
	}

}

AppHotspot::~AppHotspot() {
	// TODO Auto-generated destructor stub
}

simtime_t AppHotspot::process(ApplicationData* pdata) {
	return (simtime_t) exponential(arrival);
}

ApplicationData* AppHotspot::getFirstMsg() {
	return msgCreated(NULL);

}

ApplicationData* AppHotspot::msgCreated(ApplicationData* pdata) {

	if (id != spot) {
		if (numMessages == 0) {
			return NULL;
		}
		/*/
		 if(this->id != 0)
		 {
		 return NULL;
		 }//*/

		ApplicationData* procdata = new ApplicationData();

		procdata->setType(MPI_send);
		procdata->setDestId(spot);
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
	}else
		return NULL;
}

ApplicationData* AppHotspot::dataArrive(ApplicationData* pdata) {
	msgsRx++;

	return NULL;
}
