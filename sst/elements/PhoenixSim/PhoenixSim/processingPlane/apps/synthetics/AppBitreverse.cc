/*
 * AppBitreverse.cc
 *
 *  Created on: Feb 8, 2009
 *      Author: Gilbert
 */

#include "AppBitreverse.h"

AppBitreverse::AppBitreverse(int i, int n,  double arr, int s) : Application(i, n, NULL) {
	arrival = param1;

	assert(arrival > 0);

	size = param2;
	numMessages = param3;
	progress = numMessages / 10;

	int mask = 0;

	if(numOfProcs == 16){
		mask = 0xF;
	}else if(numOfProcs == 64){
		mask = 0x3F;
	}else if(numOfProcs == 256){
		mask = 0xFF;
	}else{
		opp_error("Unsupported number of procs for Bitreverse app");
	}

	myDest = id ^ mask;

}

AppBitreverse::~AppBitreverse() {
	// TODO Auto-generated destructor stub
}



simtime_t AppBitreverse::process(ApplicationData* pdata)
{
	return (simtime_t)exponential(arrival);
}


ApplicationData* AppBitreverse::getFirstMsg()
{
	return msgCreated(NULL);

}

ApplicationData* AppBitreverse::msgCreated(ApplicationData* pdata)
{


	if(numMessages==0)
	{
		return NULL;
	}
/*/
	if(this->id != 0)
	{
		return NULL;
	}//*/



	ApplicationData* procdata = new ApplicationData();

	procdata->setType(MPI_send);
	procdata->setDestId(myDest);
	//procdata->setDestId(15);
	procdata->setSrcId(id);

	procdata->setCreationTime(currentTime);

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
	int len = Application::sizeDist->getNewSize();

	procdata->setPayloadSize(len);

	if(numMessages > 0)
	{
		//std::cout<<numMessages<<endl;
		numMessages--;
	}




	return procdata;
}


ApplicationData* AppBitreverse::dataArrive(ApplicationData* pdata){
	msgsRx++;

	return NULL;
}
