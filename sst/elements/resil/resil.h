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


#ifndef _RESIL_H
#define _RESIL_H

#include <fstream>
#include <sstream>

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>


using namespace SST;

extern std::ofstream myfile;

enum comp_type {LEAF,SCHEDULER,REL_COMP,INFREQ};
enum fail_dist {GAUSS,EXP};
enum link_type {DISCON,INCOME,OUTGO};



class resil : public Component {
 public:
  resil              ( ComponentId_t id, Params_t& params );
  ~resil             ()                                    ;

  void processEvent( Event* event                       );
  void processfailEvent( Event* event                   );
	void schedEventHandle(Event* event                    );
	int Setup();
	int Finish();

  int my_id;
	std::string id_str;

  static bool file_open;

	/*	void set_values(comp_type type, fail_dist f_dist, float fail_duration);
  	void set_gauss_params(float gauss_mean, float gauss_std_dev);
		void set_exp_params(float exp_lambda);
		
		void do_job(job_id);
		bool runningAjob();
    bool checkForFailure();
		double computeDowntime();
    bool tellScheduler(failed, t);
		bool jobDone();
		MyEvent GetNextEvent();
		void startJob();
		void informParentComponents();
		void killJob();*/

 private:
  resil() : Component(-1) {} // Serialization requires a default constructor.
  friend class boost::serialization::access;

	/*		comp_type c_type;   // Component type of the resil
			fail_dist dist;     // resil failure rate distribution
			float fail_dur;     // resil failure duration

			//For use with the Gaussian Distribution
			float mean;
			float std_dev;*/

			//For use with Exponentials Distribution
			float lambda;	

/*			bool busy;  
			bool fail;
			int current_job;

    int my_id;
    int latency;
    bool done;*/

  Link* linkToSelf;
  Link* link0;
	Link* link1;
  Link* link2;
	Link* link3;
  Link* link4;
	Link* sched_link;
	std::vector<Link*> link_array;
	Link* uplink;
	
	int counter;
	std::vector<link_type> link_state;    //DISCON,INCOME,OUTGO
	int count_to;
	bool fail_assigned;
	bool failknow;  //If true I already know someone above me failed.

  Params_t    params   ;
  std::string frequency;
	std::vector<std::string> links;
};

struct node_id_t{
	node_id_t(std::string &s) {
		std::istringstream iss(s);
		char dot;
		iss>>major>>dot>>minor;
	}
	unsigned major, minor;
};


bool operator<(const node_id_t &l, const node_id_t &r) {return (l.major<r.major) || ((l.major==r.major) && (l.minor<r.minor));  }
bool operator==(const node_id_t &l, const node_id_t &r) {return (l.major==r.major) && (l.minor==r.minor);  }

unsigned genexp(double lambda);

#endif
