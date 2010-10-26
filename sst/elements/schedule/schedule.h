#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include <fstream>
#include <string>
#include <map>
#include <queue>
#include <set>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <vector>
#include <sst/core/simulation.h>\

#include "../resil/resil.h"

#define MAX_RESIL_LINKS 50
using namespace std;
using namespace SST;

extern std::ofstream myfile;

//enum link_type_t {DISCON,OUTGO};

class schedule : public Component {
 public:
  schedule              ( ComponentId_t id, Params_t& params );
  ~schedule             ()                                    ;

  bool clock       ( Cycle_t cycle                      );
  void processEvent( Event* event                       );
	void failEventHandle( Event* event                   );
	void finishEventHandle( Event* event                  );
	int Setup        (                                    );
	int Finish       (                                    );
	
	void issuejobs   (                                    );
	
	void node_state();
	bool nodetablehasroom(int nodes);


  ifstream tasks;
	string input_line;

 private:
  schedule() : Component(-1) {} // Serialization requires a default constructor.
  friend class boost::serialization::access;


  Link* linkToSelf;
	std::vector <Link*> link_array;

  Params_t    params   ;
  std::string frequency;
	std::vector<std::string> links;
	std::vector<link_type> link_state;    //DISCON,INCOME,OUTGO

	struct job_t{
		job_t(int j, int d, int n):job_id(j), dur(d), num_nodes(n){}
		job_t(int j): job_id(j){}
		job_t(): job_id(0), dur(1), num_nodes(1){}
		bool operator==(const job_t &t) const {return this->job_id==t.job_id; }
		bool operator<(const job_t &t) const  {return this->job_id<t.job_id;  }

		int job_id;
		int dur;
		int num_nodes;
		int start_t;
	};

	std::queue<job_t> job_list;
	std::set<job_t> running_job_list;   
	std::set<job_t> finished_job_list; 
	std::set<job_t> fail_job_list;  

  struct comp_stat_t{

		enum type_t{LEAF, RESIL} ;	

		comp_stat_t(type_t t , bool a, int j):type(t), active(a), jobid(j){}		
    comp_stat_t(): type(LEAF), active(false), jobid(0) {}

	  type_t type;
		bool active;
		int jobid;
	};

	std::map<std::string, comp_stat_t> comp_map;


};


#endif
