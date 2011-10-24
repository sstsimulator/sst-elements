// Copyright 2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _PARSER_H
#define _PARSER_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/introspectedComponent.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "../power/power.h"

//using namespace SST;

// Notice: there is no using namespace SST, 
// so we should use SST::Power
bool SST::Power::p_hasUpdatedTemp __attribute__((weak));

class parser : public SST::IntrospectedComponent {
public:

  parser(SST::ComponentId_t id, SST::Component::Params_t& params);
  int Setup() 
  {
        power = new SST::Power(getId());

	//set up floorplan and thermal tiles
	power->setChip(params);

	//set up architecture parameters
	power->setTech(getId(), params, SST::CACHE_IL1, SST::McPAT);
	power->setTech(getId(), params, SST::CACHE_DL1, SST::McPAT);
	power->setTech(getId(), params, SST::CACHE_L2, SST::McPAT);
	power->setTech(getId(), params, SST::RF, SST::McPAT);
	power->setTech(getId(), params, SST::EXEU_ALU, SST::McPAT);
	power->setTech(getId(), params, SST::LSQ, SST::McPAT);
	power->setTech(getId(), params, SST::IB, SST::McPAT);
	power->setTech(getId(), params, SST::INST_DECODER, SST::McPAT);
	if (machineType == 0){
	    power->setTech(getId(), params, SST::LOAD_Q, SST::McPAT);
	    power->setTech(getId(), params, SST::RENAME_U, SST::McPAT);
	    power->setTech(getId(), params, SST::SCHEDULER_U, SST::McPAT);
	    power->setTech(getId(), params, SST::BTB, SST::McPAT);
	    power->setTech(getId(), params, SST::BPRED, SST::McPAT);
	}


	//reset all counts to zero
	power->resetCounts(&mycounts);  
      return 0;
  }
  int Finish() {
    power->compute_MTTF();
    printf("parser Finished\n");   
    return 0;
  }

private:
  parser();  // for serialization only
  parser(const parser&); // do not implement
  void operator=(const parser&); // do not implement

  bool parseM5( SST::Cycle_t );
  
  SST::TimeConverter *m_tc;
  // parameters for power modeling
  Params_t params;   
  // For power & introspection
  SST::Pdissipation_t pdata, pstats;
  SST::Power *power;
  std::string frequency;
  unsigned int machineType; //0: OOO, 1: inorder
  // Over-specified struct that holds usage counts of its sub-components
  SST::usagecounts_t mycounts;
  ParseXML *m_mcpat;
  std::string mcpatXML; //This is the mcpat xml file generates from the stats.txt of M5 using the python parser.
			//Each mcpat xml file contains statistics at a timestep
  unsigned int numStatsFile; //Number of mcpat xml files (# timesteps that generates statistics outputs)
			//If mcpatXML = mcpat, and numStatsFile = 4, this SST parser reads in mcpat_1.xml, mcpat_2.xml
			//mcpat_3.xml and mcpat_4.xml.




  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
  }
    
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

#endif /* _PARSER_H */
