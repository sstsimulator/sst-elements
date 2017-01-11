#include<iostream>
#include "NVM_DIMM.h"
#include<map>
#include<ctime>
#include <stdlib.h>

using namespace std;

int main()
{

	NVM_PARAMS x;


	x.size = 8*1024*1024;  // in KB, which mean 8GB

	x.write_buffer_size = 256;	

	x.max_outstanding = 4;

	x.max_current_weight = 20;

	x.write_weight = 10;

	x.read_weight = 4;

	x.max_requests = 1;

	//float clock; 

	//float memory_clock;

	//float io_clock;

	x.tCMD = 1;

	x.tCL = 60;

	x.tRCD = 400;

	x.tCL_W = 1000;

	x.tBURST = 8;

	x.device_width = 8;

	x.num_ranks = 8;

	x.num_devices = 8;

	x.num_banks = 32;
	x.row_buffer_size = 8192;
	x.flush_th = 50; // this means it starts flushing at 50% full write buffer to avoid throttling the controller

	NVM_DIMM * ptr = new NVM_DIMM(x);

	long long req_id = 0;

	map<NVM_Request *, long long int> submit_time;

	long long int last_add = 0;
	for(int i=0; i< 10000000; i++)
	{

		if(i%100 ==0)
		{

			//last_add +=64;
			NVM_Request * temp;
			if((rand()%100) > 10)
				temp = new NVM_Request(req_id++, true, 64, abs(rand()%(x.size * 1024))); 
			else
				temp = new NVM_Request(req_id++, false, 64, abs(rand()%(x.size * 1024))); 
			//NVM_Request * temp = new NVM_Request(req_id++, true, 64, abs(rand()%(x.size * 1024))); 

			submit_time[temp] = i;

			if(!ptr->push_request(temp))
				delete temp;
			else
				last_add += 64;

		}
		ptr->tick();
		//std::cout<<"This is stuck here"<<std::endl;

		//cout<<"We submitted next"<<endl;
		NVM_Request * ready= ptr->pop_request();

		if(ready!=NULL) // pull a ready a request
		{
			cout<<"The time spent was "<<i - submit_time[ready]<<endl;
			delete ready;
		}
	}


	return 0;
}
