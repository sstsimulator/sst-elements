// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sstream>
#include <string>
#include <fstream>

using std::ofstream;

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "resil.h"
//#include "myEvent.h"
//#include "sst/core/CompEvent.h"
#include "sst/elements/schedule/schedEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>


bool resil::file_open=false;

/**/
ofstream myfile;

resil::resil( ComponentId_t id, Params_t& params ) :
  Component(id), params(params), frequency("1000GHz")
{
		int dur_scale=1;
		int linki=0, num_links=0;

		std::ostringstream os;

	/*for(linki=0; linki<MAX_LINKS; linki++)
	{ 
		link_array[linki]=0;
		link_state[linki]=DISCON; 
	}*/

	linki=0;	
	std::cout << "------------------------------------------------------\n";
  std::cout << "resil constructor!\n";

	if(params.find("id") != params.end()) {
		id_str = params["id"];
	}
	if (params.find("lambda") != params.end() ) {
		lambda = strtod(params["lambda"].c_str(), NULL);
	}
	os<<"link"<<linki;
	while (params.find(os.str()) != params.end() ){
		links.push_back(params[os.str()]);
		os.str("");
		linki++;
		os<<"link"<<linki;
		std::cout<<"The string was: "<<os.str()<<std::endl;
	}

	for(int i=0; i < links.size(); i++)
	{
		std::cout<<links[i]<<"\n";
	}

	my_id=id;
	
  std::cout<<"\nLambda: "<<lambda<<"\n";

	TimeConverter *tc=registerTimeBase(frequency);

  linkToSelf = configureSelfLink("linkToSelf",
																		new Event::Handler<resil>
   																		(this, &resil::processfailEvent)
                               	 );

	setDefaultTimeBase(tc);

	for(linki = 0;linki<links.size(); linki++)
	{
		os.str("");
		os<<"link"<<linki;
		link_array.push_back(configureLink(os.str(), new Event::Handler<resil>(this, &resil::processEvent)));
		//std::cout<< links[linki]<<" Comapre to " << id_str<< " returns " << links[linki].compare(0, 3, id_str, 0, 3) << "\n";
		node_id_t id_nums(id_str);
		node_id_t link_id_nums(links[linki]);


		/*if (link_array[linki]==0)
		{
			link_state[linki]=DISCON;	
			 break;
		}
		else if(links[linki].compare(0,3,id_str)==0)
		{
			link_state[linki]=INCOME;
		}
		else
		{
			link_state[linki]=OUTGO;
		}*/

		if (link_array[linki]==0)
		{
			link_state.push_back(DISCON);	
			 break;
		}
		else if(id_nums==link_id_nums)
		{
			link_state.push_back(INCOME);
		}
		else
		{
			link_state.push_back(OUTGO);
		}

	}
	num_links=linki;

	for(linki=0;linki<link_state.size(); linki++)
	{
    std::cout<<"Link "<< linki<< " has value "<< link_state[linki] << "\n";
	}

	
	//link_array[1]=configureLink("link1", new Event::Handler<resil>(this, &resil::processEvent));
	//uplink = configureLink("uplink", new Event::Handler<resil>(this, &resil::processEvent));

	sched_link=configureLink("schedlink0", new Event::Handler<resil>(this, &resil::schedEventHandle));

	for(linki=0; linki<link_array.size(); linki++) {
		if(link_array[linki]==0)  {std::cout << "link_array "<< linki << "NULL \n";}
		else {
      link_array[linki]->setDefaultTimeBase(tc); 
    }
	}
  
	if (sched_link == 0) { std::cout << "sched_link NULL!\n"; }
	else {sched_link->setDefaultTimeBase(tc);}

  linkToSelf->setDefaultTimeBase(tc);
	
	
	//link_array[0]->setDefaultTimeBase(tc);
	//link_array[1]->setDefaultTimeBase(tc);
	//sched_link->setDefaultTimeBase(tc);
	



	
	//sched_link->send(25, new endEvent());
	//registerExit();

}


resil::~resil()
{

}


void resil::processEvent( Event* event )
{
	//double threshold =  RAND_MAX*(1-exp(-lambda*getCurrentSimTime()));
	/*double my_num=rand();
	counter=0;
	count_to=1;
	int dur_scale=1;
	fail_assigned=0;
	int fail_time=dur_scale*genexp(lambda);

	if(fail_assigned==0){
		linkToSelf->Send(fail_time,new ComppEvent());
		std::cout<<"Issue time:"<<getCurrentSimTime()<<" Fail cycle:"<<fail_time<<"\n";
		fail_assigned=1;
	}*/


//Add a portion to only end event if we don't already know of a fail. 
	std::cout<<my_id<<": Someone above me failed and it is cycle " << getCurrentSimTime()<<"\n";
	
	if(sched_link!=0)  sched_link->Send(0, new failEvent(id_str, getCurrentSimTime()));

	for(int i=0; i < link_array.size(); i ++)
	{
		if((link_array[i]!=NULL)&&(link_state[i]==OUTGO))
		{
			link_array[i]->Send(0,new ComppEvent());
		}
	}
	/*if(link0!=0)
		link0->Send(0,new ComppEvent());
	if(link1!=0)
		link1->Send(0,new ComppEvent());
	if(link2!=0)
		link0->Send(0,new ComppEvent());
	if(link3!=0)
		link1->Send(0,new ComppEvent());
	if(link4!=0)
		link0->Send(0,new ComppEvent());*/

  delete event;
	/*counter=0;
	count_to=10;
	while(counter<count_to)
	{
		std::cout<<counter;
		counter++;
	}
	std::cout<<"\n"*/
}

void resil::processfailEvent( Event* event )
{
	std::cout << my_id << ": resil:failed at " <<getCurrentSimTime()<<" event " << static_cast<ComppEvent*>(event) << "!\n";
	
	//Send out notice of fail on lower links
	/*if(link0!=0)
		link0->Send(0, new ComppEvent());
	if(link1!=0)
		link1->Send(0, new ComppEvent());
	if(link2!=0)
		link0->Send(0, new ComppEvent());
	if(link3!=0)
		link1->Send(0, new ComppEvent());
	if(link4!=0)
		link0->Send(0, new ComppEvent());*/

	for(int i=0; i < link_array.size(); i ++)
	{
		if((link_array[i]!=NULL)&&(link_state[i]==OUTGO))
		{
			link_array[i]->Send(0,new ComppEvent());
		}
	}

	if(sched_link!=0)  sched_link->Send(0, new failEvent(id_str, getCurrentSimTime()));

	myfile << floor(getCurrentSimTime()/100)<< "," << id_str << "\n";  

	//
	unsigned fail_time=genexp((1.0/31556926.0)*lambda);  //number of seconds in a year
  std::cout << "Next failure in " << fail_time << " cycles. (lambda=" << lambda << "\n";
	linkToSelf->Send(fail_time*100,new ComppEvent());
  delete event; // I'm responsible for it.
  return;
}

void resil::schedEventHandle( Event* event )
{

	delete event;  //I'm responsible for it.
	return;
}

int resil::Setup()
{
		if(file_open==0)
		{
			myfile.open("failures.log");
			file_open=1;
		 myfile<<"TIME, NODE\n";
		}

	//if(id==1)
	//{
		unsigned fail_time=genexp((1.0/31556926.0)*lambda);  //number of seconds in a year

			linkToSelf->Send(fail_time*100,new ComppEvent());
			std::cout<<"Issue time:"<<getCurrentSimTime()<<" Fail cycle:"<<floor(fail_time)<<"\n";
		
	//}

		return 0;
}

int resil::Finish()
{
	if(file_open==1)
	{
		myfile.close();
		file_open=0;
	}
	return 0;
}

double urand()
{
	return(rand() + 1.0)/(RAND_MAX + 1.0);
}

unsigned genexp(double lambda)
{
	double u,x;
	u=urand();
	x=(-1/lambda)*log(u);
	return(x);
}

/***************************resil**************************************/
/*void resil::set_values (comp_type type, fail_dist f_dist, float fail_duration){

			c_type=type;
			dist=f_dist;
			fail_dur=fail_duration;
}*/

/*void resil::set_gauss_params (float gauss_mean, float gauss_std_dev){
		mean=gauss_mean;
		std_dev=gauss_std_dev;
}*/


/*void resil::set_exp_params (float exp_lambda){
	lambda=exp_lambda;
}*/

/*void resil::do_job(job_id){
	busy=1;
}

bool resil::runningAjob(){
	return busy;
}

bool resil::checkForFailure(){
	
}

double resil::computeDowntime(){

}

bool resil::tellScheduler(failed, t){
	
}

bool resil::jobDone(){
	return busy;
}
		
MyEvent resil::GetNextEvent(){

}

void resil::startJob(){

}

void resil::informParentComponents(){

}

void resil::killJob(){

}*/


extern "C" {
  resil* resilAllocComponent( SST::ComponentId_t id,
                          SST::Component::Params_t& params )
  {
    std::cout << "resilAllocComponent factory-ing a new resil!\n";
    return new resil( id, params );
  }
};

BOOST_CLASS_EXPORT(resil);
