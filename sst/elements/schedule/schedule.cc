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
#include "schedEvent.h"
#include "../resil/resil.h"
#include "sst/core/compEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/event.h>

using namespace std;


schedule::schedule( ComponentId_t id, Params_t& params ) : 
  Component(id), params(params), frequency("1000.0GHz")
{

	int linki=0, num_links=0;
	std::ostringstream os;

	
	
	int num_tasks=15;
	

	tasks.open("/home/aswilli/trunk/sst/elements/job_list.txt");
	std::cout << "schedule constructor!\n";
  if ( params.find("clock") != params.end() ) {
    frequency = params["clock"];
  }
	os<<"link"<<linki;
	while (params.find(os.str()) != params.end() ){


		links.push_back(params[os.str()]);
		os.str("");
		linki++;
		os<<"link"<<linki;
		std::cout<<"The string was: "<<os.str()<<std::endl;
	}
		std::cout<< "In loop\n";

  TimeConverter *tc = registerClock(frequency,
                                    new Clock::Handler<schedule>
      																(this, &schedule::clock)
   																	);

  linkToSelf = configureSelfLink("linkToSelf",
																		new Event::Handler<schedule>
   																		(this, &schedule::finishEventHandle)
                               	 );

  linkToSelf->setDefaultTimeBase(tc);

	

	for(linki = 0; linki<MAX_RESIL_LINKS; linki++)
	{
		os.str("");
		os<<"link"<<linki;
		link_array.push_back(configureLink(os.str(), new Event::Handler<schedule>(this, &schedule::failEventHandle)));

		if (link_array[linki]==0)
		{
			link_state.push_back(DISCON);	
			 break;
		}
		else
		{
			link_state.push_back(OUTGO);
		}
	}
	num_links=linki;

	for(linki=0; linki<link_array.size(); linki++)
	{
    std::cout<<"Link "<< linki<< " has value "<< link_state[linki] << "\n";
	}

	for(linki=0; linki<link_array.size(); linki++) {
		if(link_array[linki]==0)  {std::cout << "link_array "<< linki << "NULL \n";}
		else {
      link_array[linki]->setDefaultTimeBase(tc); 
    }
	}


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
			std::cout<<"Job_ID: "<<tmp_jobid<<" Dur: "<<tmp_dur<<" nodes: "<<tmp_nodes<<"\n";		
			job_list.push(job_t(tmp_jobid, tmp_dur, tmp_nodes));
			std:cout<<"JOB_ID "<<job_list.back().job_id<<" DUR: "<<job_list.back().dur<<" NODES:"<<job_list.back().num_nodes<<"\n";
		}
	}
	else
	{
		std::cout<<"file not open"<<endl;
	}

	/*for (int i=0; i < job_id.size(); i++){
		std:cout<<"JOB_ID "<<job_id[i]<<" DUR: "<<dur[i]<<" NODES:"<<num_nodes[i]<<"\n";
	}*/


  //linkToSelf->Send(10, new CompEvent());
	//job_id=new int[num_tasks];
	//dur=new int[num_tasks];
	//num_nodes=new int[num_tasks];

	registerExit();
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
  
  //linkToSelf->Send(10, new CompEvent());

  return false;
}

/*void schedule::processEvent( Event* event )
{


  std::cout << "schedule: got event pointer " << event << "!\n";
  delete event; // I'm responsible for it.

  return;
}*/

void schedule::failEventHandle( Event* event )
{
	failEvent* temp_e=static_cast<failEvent*>(event);

	std::cout<<"A leaf failed";
	
	//only works for case of a node failure	
	if(comp_map[(string(temp_e->comp))].active==1)
	{
		//if(!job_list.empty())
		//{
			//if(nodetablehasroom(job_list.front().num_nodes))
			//{
				//std::cout<<"\nNumber of nodes needed " <<job_list.front().num_nodes<<"\n";
				//TO ADD:   Write the job failure record

				

				//Add the job to the fail list if it hasn't already been added 
				if(fail_job_list.find(job_t(comp_map[string(temp_e->comp)].jobid))==fail_job_list.end())
				{
					fail_job_list.insert(*running_job_list.find(job_t(comp_map[string(temp_e->comp)].jobid)));
					running_job_list.erase(job_t(job_t(comp_map[string(temp_e->comp)].jobid)));
				}
				//Delete job from job list
				
				//Mark nodes available for new jobs, aka as inactive
				comp_map[string(temp_e->comp)].active=0;
				comp_map[string(temp_e->comp)].jobid=0;
			//}
		//}

	//issue new jobs to slots that have opened up
	if(!job_list.empty())
		issuejobs();

	}

	if(running_job_list.empty())
	{
		std::cout<<"I JUST UNREGISTERED EXIT\n";
		unregisterExit();
	}
	
	//else there was no job running on the node that failed so do not record any stats

	return;
}

void schedule::finishEventHandle( Event* event )
{
	finishEvent* temp_e=static_cast<finishEvent*>(event);
	std::map<std::string, schedule::comp_stat_t>::iterator j;

	if(fail_job_list.find(job_t(temp_e->jobid))==fail_job_list.end())
	{
		std::cout<<"Job "<< temp_e->jobid << " finished\n"; 
		//Write a job sucess record
		
		//remove job from the job list
		running_job_list.erase(job_t(temp_e->jobid));
		//mark the nodes as available for new jobs
		for(j=comp_map.find("1.1"); !(j->first.compare(0, 2, "1.")); j++)
		{
			if(j->second.jobid==temp_e->jobid)		
			{
				j->second.jobid=0;
				j->second.active=0;
			}
		}
	}
	else
	{
		std::cout<<" Job "<< temp_e->jobid <<" has already failed\n";
		//remove this job_id from the fail list
		fail_job_list.erase(job_t(temp_e->jobid));
	}

	//issue new jobs to slots that have opened up
	if(!job_list.empty())
		issuejobs();
	
	if(running_job_list.empty())
	{
		std::cout<<"I JUST UNREGISTERED EXIT\n";
		unregisterExit();
	}
}


int schedule::Setup()
{

		CompMap_t::const_iterator i;

  	myfile.open("job_failures.txt");

		std::cout<<"Setup is running\n";
		for(i=Simulation::getSimulation()->getComponentMap().begin();   
					i != Simulation::getSimulation()->getComponentMap().end(); i++)
		{
			const Component *c=i->second;
			const resil *r=dynamic_cast<const resil*>(c);
	    std::cout << r << ": from component " << c << "\n";
      if (!r) continue;
  		if(r->id_str[0]=='1'){
				comp_map[i->first]=comp_stat_t( schedule::comp_stat_t::LEAF , 0, 0);
			}
			else{
				comp_map[i->first]=comp_stat_t( schedule::comp_stat_t::RESIL , 0, 0);
			}
		}
			
		node_state();

		//comp_map[string("1.1")].active=1;

		node_state();

		issuejobs();

    return 0;
}

int schedule::Finish()
{
		std::cout<<"Scheduler file closed\n";
		myfile.close();
		return 0; 
}

void schedule::node_state()
{
		std::map<std::string, schedule::comp_stat_t>::const_iterator i; 
		static std::string type_str[2];

		type_str[0]="LEAF";
		type_str[1]="RESIL";

		for(i=comp_map.begin();   
					i != comp_map.end(); i++)
		{
				std::cout<<"Component " << i->first <<" is " << type_str[i->second.type] << " Active? " << i->second.active <<"\n";
		}
}

/*Returns true if there are enuogh nodes available*/
bool schedule::nodetablehasroom(int nodes)
{
	int tmp_count=0;
	std::map<std::string, schedule::comp_stat_t>::const_iterator i;
		
	for(i=comp_map.find("1.1"); !(i->first.compare(0, 2, "1.")); i++)
	{
		if(i->second.active==0)
			tmp_count++;
	}
	return(tmp_count>=nodes);
}

void schedule::issuejobs()
{
	std::map<std::string, schedule::comp_stat_t>::iterator j;

	while(nodetablehasroom(job_list.front().num_nodes))
	{
		job_list.front().start_t=getCurrentSimTime();
		std::cout<<"Job "<< job_list.front().job_id<<" was scheduled on "<< job_list.front().num_nodes <<" nodes\n"; 

		//send self event to scheduler for finish
		linkToSelf->Send(job_list.front().dur, new finishEvent(job_list.front().job_id));

		for(int i=0; i<job_list.front().num_nodes; i++)
		{
			for(j=comp_map.find("1.1"); !(j->first.compare(0, 2, "1.")); j++)
			{
				if(j->second.active==0)
				{
					j->second.active=1;
					j->second.jobid=job_list.front().job_id;
					break;
				}
			}
		}
		running_job_list.insert(job_list.front());
		job_list.pop();
	}
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
