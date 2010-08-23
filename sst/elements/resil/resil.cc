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
#include <math.h>

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "resil.h"
//#include "myEvent.h"
#include "sst/core/compEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>


resil::resil( ComponentId_t id, Params_t& params ) :
  Component(id), params(params), frequency("1.0GHz")
{
  std::cout << "resil constructor!\n";
  if ( params.find("clock") != params.end() ) {
    frequency = params["clock"];
  }
	if (params.find("lambda") != params.end() ) {
		lambda = strtod(params["lambda"].c_str(), NULL);
	}

  //for (Params_t::iterator i = params.begin(); i != params.end(); i++)
  //  std::cout << id << ": " << i->first << "->" << i->second << '\n';

  /*if ( params.find("count_to") == params.end() ) {
        _abort(event_test,"couldn't find count_to\n");
  }
  count_to = strtol( params[ "count_to" ].c_str(), NULL, 0 );*/

  std::cout<<lambda;

  TimeConverter *tc = registerClock(frequency,
                                    new Clock::Handler<resil>
      																(this, &resil::clock)
   																	);

  //linkToSelf = configureSelfLink("linkToSelf",
																		new Event::Handler<resil>
   																		(this, &resil::processfailEvent)
                               	 );

	link0 = configureLink("link0", new Event::Handler<resil>(this, &resil::processEvent));
	//link_array[0]=configureLink("link0", new Event::Handler<resil>(this, &resil::processEvent));
	link1 = configureLink("link1", new Event::Handler<resil>(this, &resil::processEvent));
	//link_array[1]=configureLink("link1", new Event::Handler<resil>(this, &resil::processEvent));

	//sched_link=configureLink();

  if (link0 == 0) { std::cerr << "link0 NULL\n"; }
	//if(link_array[0] == 0) {std::cerr << "link0 NULL\n"; }
  if (link1 == 0) { std::cerr << "link1 NULL!\n"; }
	//if(link_array[1] == 0) {std::cerr << "link1 NULL\n"; }
  
	if (sched_link == 0) { std::cerr << "link1 NULL!\n"; }

  linkToSelf->setDefaultTimeBase(tc);
	link0->setDefaultTimeBase(tc);
	link1->setDefaultTimeBase(tc);
	//link_array[0]->setDefaultTimeBase(tc);
	//link_array[1]->setDefaultTimeBase(tc);
	sched_link->setDefaultTimeBase(tc);
  registerTimeBase("1ps");

}


resil::~resil()
{

}

bool resil::clock( Cycle_t current ) 
{
  std::cout << "resil: it's cycle " << current << "!\n";
  
  //link_array[0]->Send(10, new CompEvent());
	link1->Send(10, new CompEvent());

  return true;
}

void resil::processEvent( Event* event )
{
	//double threshold =  RAND_MAX*(1-exp(-lambda*getCurrentSimTime()));
	double my_num=rand();
	counter=0;
	count_to=1;
	int dur_scale=1;
	fail_assigned=0;
	int fail_time=dur_scale*genexp(lambda);

	if(fail_assigned==0){
		linkToSelf->Send(fail_time,new CompEvent());
		std::cout<<"Issue time:"<<getCurrentSimTime()<<" Fail cycle:"<<fail_time<<"\n";
		fail_assigned=1;
	}

  delete event;
	/*counter=0;
	count_to=10;
	/*while(counter<count_to)
	{
		std::cout<<counter;
		counter++;
	}
	std::cout<<"\n"*/
  return;
}

void resil::processfailEvent( Event* event )
{
	std::cout << "resil:failed at " <<getCurrentSimTime()<<"!\n";
  delete event; // I'm responsible for it.
  return;
}

float urand()
{
	return((float) rand()/RAND_MAX);
}

float genexp(float lambda)
{
	float u,x;
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
