/*
 * AppRandom.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppRandom.h"

AppRandom::AppRandom(int i, int n,  double arr, int s) : Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;
}

AppRandom::~AppRandom() {
	// TODO Auto-generated destructor stub
}



simtime_t AppRandom::process(ApplicationData* pdata)
{
	return (simtime_t)exponential(arrival);
}


ApplicationData* AppRandom::getFirstMsg()
{
	return msgCreated(NULL);

}

ApplicationData* AppRandom::msgCreated(ApplicationData* pdata)
{
	int dest = id;

	if(numMessages==0)
	{
		return NULL;
	}
/*/
	if(this->id != 0 && this->id != 2)
	{
		return NULL;
	}//*/



	while(dest == id)
		dest = intuniform(0, numOfProcs-1);
	ApplicationData* procdata = new ApplicationData();


	procdata->setType(MPI_send);
	procdata->setDestId(dest);
	//procdata->setDestId((this->id == 0)?15:14);
	procdata->setSrcId(id);

	procdata->setCreationTime(currentTime);



	int len = Application::sizeDist->getNewSize();
	debug("appRandom", "length: ", len, UNIT_APP);
	procdata->setPayloadSize(len);

	if(numMessages > 0)
	{
		//std::cout<<numMessages<<endl;
		numMessages--;
	}




	return procdata;
}


ApplicationData* AppRandom::dataArrive(ApplicationData* pdata){
	msgsRx++;

	debug("appRandom", "msg received at ", id, UNIT_APP);
	//std::cout<<id<<": complete"<<endl;
	return NULL;
}
