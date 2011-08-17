/*
 * AppRandom.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "App_MITUCB_Random.h"

App_MITUCB_Random::App_MITUCB_Random(int i, int n,  double arr, int s) : Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;
}

App_MITUCB_Random::~App_MITUCB_Random() {
	// TODO Auto-generated destructor stub
}



simtime_t App_MITUCB_Random::process(ApplicationData* pdata)
{
	return (simtime_t)exponential(arrival);
}

ApplicationData* App_MITUCB_Random::getFirstMsg()
{
	return msgSent(NULL);

}

ApplicationData* App_MITUCB_Random::msgSent(ApplicationData* pdata)
{
	int dest = id;

	if(numMessages==0)
	{
		return NULL;
	}
	while(dest == id)
	{
		if(id >= 0 && id < 256)
		{
			dest = intuniform(256,271);
			//dest = 256;
		}
		else
		{
			//dest = intuniform(0, 255);
			return NULL;
		}
	}

		ApplicationData* procdata = new ApplicationData();

		procdata->setType(MPI_send);
		procdata->setDestId(dest);
		procdata->setSrcId(id);
	if(size<=0)
	{
		std::cout<<"Your exponential distribution parameter is wrong, Cntl-C and get out!"<<endl;
	}
	/*
	int len = exponential(size);
	while(len <= 0)
	{
		len = exponential(size);
	}
	*/
	// Do static size
	int len = size;

	procdata->setPayloadSize(len);

	if(numMessages > 0)
	{
		numMessages--;
	}
	return procdata;


}

ApplicationData* App_MITUCB_Random::dataArrive(ApplicationData* pdata){
	msgsRx++;
	bytesTransferred += pdata->getPayloadSize();
	latencyTime += SIMTIME_DBL(currentTime - pdata->getCreationTime());
	return NULL;
}
