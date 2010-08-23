#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include <fstream>
#include <string>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <vector>

using namespace std;
using namespace SST;

class schedule : public Component {
 public:
  schedule              ( ComponentId_t id, Params_t& params );
  ~schedule             ()                                    ;

  bool clock       ( Cycle_t cycle                      );
  void processEvent( Event* event                       );
  ifstream tasks;
	string input_line;

 private:
  schedule() : Component(-1) {} // Serialization requires a default constructor.
  friend class boost::serialization::access;

  Link* linkToSelf;

  Params_t    params   ;
  std::string frequency;

	std::vector<int> job_id;
	std::vector<int> dur;
	std::vector<int> num_nodes;
};

#endif
