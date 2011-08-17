#include "AppRandom_FiniteQueue.h"

AppRandom_FiniteQueue::AppRandom_FiniteQueue(int i, int n, double arr, int s) :
	Application(i, n, NULL) {
	arrival = param1;
	size = param2;
	numMessages = param3;

	progress = numMessages / 10;
	first = true;

}

AppRandom_FiniteQueue::~AppRandom_FiniteQueue() {
	// TODO Auto-generated destructor stub
}

simtime_t AppRandom_FiniteQueue::process(ApplicationData* pdata) {
	return (simtime_t) exponential(arrival);
}

ApplicationData* AppRandom_FiniteQueue::getFirstMsg() {
	return msgSent(NULL);

}

ApplicationData* AppRandom_FiniteQueue::dataArrive(ApplicationData* pdata) {
	msgsRx++;


	debug("app", "msg complete at node ",id, UNIT_APP);

	return NULL;
}

ApplicationData* AppRandom_FiniteQueue::msgSent(ApplicationData* pdata) {
	int dest = id;

	if (numMessages == 0) {
		return NULL;
	}

	debug("app", "msg sent, sending another", UNIT_APP);


	while(dest == id)
	{
		dest = intuniform(0, numOfProcs-1);
	}
	ApplicationData* procdata = new ApplicationData();

	procdata->setType(MPI_send);
	procdata->setDestId(dest);
	procdata->setSrcId(id);
	procdata->setCreationTime(currentTime);

	int len = Application::sizeDist->getNewSize();

	procdata->setPayloadSize(len);

	if (numMessages > 0) {
		std::cout <<id << "-->"<< numMessages << endl;
		numMessages--;
	}
	return procdata;
}
