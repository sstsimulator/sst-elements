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
#include <fstream>
#include <cstdlib>
#include <string>

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "schedule.h"
//#include "myEvent.h"
#include "sst/core/compEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/event.h>

using namespace std;


schedule::schedule( ComponentId_t id, Params_t& params ) : 
  Component(id), params(params), frequency("1.0GHz")
{
	int num_tasks=15;
	tasks.open("/home/aswilli/trunk/sst/elements/job_list.txt");
	std::cout << "schedule constructor!\n";
  if ( params.find("clock") != params.end() ) {
    frequency = params["clock"];
  }


  TimeConverter *tc = registerClock(frequency,
                                    new Clock::Handler<schedule>
      																(this, &schedule::clock)
   																	);

  linkToSelf = configureSelfLink("linkToSelf",
																		new Event::Handler<schedule>
   																		(this, &schedule::processEvent)
                               	 );

  linkToSelf->setDefaultTimeBase(tc);

  //linkToSelf->Send(10, new CompEvent());
	//job_id=new int[num_tasks];
	//dur=new int[num_tasks];
	//num_nodes=new int[num_tasks];
}

schedule::~schedule()
{
	tasks.close();
	/*delete [] job_id;
	delete [] dur;
	delete [] num_nodes;*/
}

bool schedule::clock( Cycle_t current ) 
{
  std::cout << "schedule: it's cycle " << current << "!\n";
  
  linkToSelf->Send(10, new CompEvent());

  return true;
}

void schedule::processEvent( Event* event )
{
	int ln_num=0;
	string temp;
	int j=0;
	int which_val=0;
	int tmp_jobid=333;
	int tmp_dur=444;
	int tmp_nodes=555;

	if(tasks.is_open()){
		getline(tasks,input_line);
		while(!tasks.eof()){
			getline(tasks,input_line);
			string temp;
			unsigned long start=0;
			unsigned long end=0;
			while(end<input_line.size())
			{
				end++;
				if(end<input_line.size() && input_line[end]!=',')
				{
					continue;
				}
				temp.assign(input_line,start,end-start);


				if(which_val==0){
					tmp_jobid=atoi(temp.c_str());
					which_val=1;
				}
				else if(which_val==1) {
					tmp_dur=atoi(temp.c_str());
					which_val=2;
				}
				else if (which_val==2) {
					tmp_nodes=atoi(temp.c_str());
					which_val=0;
				}
				end++;
				start=end;
			}
			//std::cout<<"Job_ID: "<<tmp_jobid<<" Dur: "<<tmp_dur<<" nodes: "<<tmp_nodes<<"\n";		
			job_id.push_back(tmp_jobid);
			dur.push_back(tmp_dur);
			num_nodes.push_back(tmp_nodes);
		}
	}
	else
	{
		std::cout<<"file not open"<<endl;
	}
  std::cout << "schedule: got event pointer " << event << "!\n";
  delete event; // I'm responsible for it.

	for (int i=0; i < job_id.size(); i++){
		//std:cout<<"JOB_ID "<<job_id[i]<<" DUR: "<<dur[i]<<" NODES:"<<num_nodes[i]<<"\n";
	}

  return;
}


extern "C" {
  schedule* scheduleAllocComponent( SST::ComponentId_t id,
                          SST::Component::Params_t& params )
  {
    std::cout << "scheduleAllocComponent factory-ing a new schedule!\n";
    return new schedule( id, params );
  }
};

BOOST_CLASS_EXPORT(schedule);
