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

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include <assert.h>

#include "sst/core/element.h"

#include "parser.h"
#include <sstream>


using namespace SST;

parser::parser(ComponentId_t id, Params_t& params) :
  IntrospectedComponent(id),
  params (params)  //Genie for power
 {

  numStatsFile = 1;

  // get parameters
  if ( params.find( "machine_type" ) != params.end() ) {
        if ( params["machine_type"].compare("0") == 0 ) 
       	    machineType = 0 ;
	else
	    machineType = 1;
     	
  }

  frequency = "";
  if ( params.find( "samplingFreq" ) != params.end() ) {
	frequency = params["samplingFreq"];
  }

  mcpatXML = "";
  if ( params.find( "StatsFile" ) != params.end() ) {
      mcpatXML = params["StatsFile"];
  }
  if ( params.find( "number_of_stats_files" ) != params.end() ) {
      numStatsFile = strtol( params[ "number_of_stats_files" ].c_str(), NULL, 0 );
  }

  m_mcpat = new ParseXML();
  
  m_tc = registerTimeBase("1ps");
  assert( m_tc );

  // tell the simulator not to end without us
  registerExit();
  
  // for power introspection
  registerClock(frequency, new Clock::Handler<parser>
					(this, &parser::parseM5 ) );
}

parser::parser() :
    IntrospectedComponent(-1)
{
    // for serialization only
}



bool parser::parseM5( Cycle_t current) {
  std::string filename;
  
  std::cout << " It is time (" <<current << ") to push power " << std::endl;

  std::ostringstream step;
  step << current;

  assert (numStatsFile >= current);

  filename.append(mcpatXML);
  filename.append("_");
  filename.append(step.str());
  filename.append(".xml");
  std::cout << "Parsing McPAT xml" << filename << std::endl;

  //m_mcpat->parse("/home/myhsieh/Desktop/documents/DATE12/mcpat_bzip_2.xml");
  //m_mcpat->parse("/home/myhsieh/Desktop/documents/DATE12/Xeon.xml");
  m_mcpat->parse(&filename[0]);    
  //std::cout << "icache[0] = " << m_mcpat->sys.core[0].icache.read_accesses << std::endl;

  for (unsigned int i=0; i< m_mcpat->sys.number_of_cores; i++){
      //icache
      mycounts.il1_read[i] = m_mcpat->sys.core[i].icache.read_accesses - 
				m_mcpat->sys.core[i].icache.read_misses;
      mycounts.il1_readmiss[i] = m_mcpat->sys.core[i].icache.read_misses;
      std::cout << "il1_read[" << i << "] = " << mycounts.il1_read[i] << std::endl;
      //dcache
	  mycounts.dl1_read[i] = m_mcpat->sys.core[i].dcache.read_accesses - 
				m_mcpat->sys.core[i].dcache.read_misses;
	  mycounts.dl1_readmiss[i] = m_mcpat->sys.core[i].dcache.read_misses;
	  mycounts.dl1_write[i] = m_mcpat->sys.core[i].dcache.write_accesses - 
				m_mcpat->sys.core[i].dcache.write_misses;
	  mycounts.dl1_writemiss[i] = m_mcpat->sys.core[i].dcache.write_misses;
	  //RF
	  mycounts.int_regfile_reads[i] = m_mcpat->sys.core[i].int_regfile_reads;
    	  mycounts.int_regfile_writes[i] = m_mcpat->sys.core[i].int_regfile_writes; 
	  mycounts.float_regfile_reads[i] = m_mcpat->sys.core[i].float_regfile_writes;
	  mycounts.float_regfile_writes[i] = m_mcpat->sys.core[i].float_regfile_writes;
	  //ALU
	  mycounts.alu_access[i] = m_mcpat->sys.core[i].ialu_accesses;
    	  mycounts.fpu_access[i] = m_mcpat->sys.core[i].fpu_accesses;
	  //LSQ
	  mycounts.LSQ_read[i] = (m_mcpat->sys.core[i].load_instructions + m_mcpat->sys.core[i].store_instructions)*2;
	  mycounts.LSQ_write[i] = (m_mcpat->sys.core[i].load_instructions + m_mcpat->sys.core[i].store_instructions)*2;
	  //IB
	  mycounts.IB_read[i] = m_mcpat->sys.core[i].total_instructions;
	  mycounts.IB_write[i] = m_mcpat->sys.core[i].total_instructions;
	  //INST_DECODER
	  mycounts.ID_inst_read[i] = m_mcpat->sys.core[i].total_instructions;
	  mycounts.ID_operand_read[i] = m_mcpat->sys.core[i].total_instructions;
	  mycounts.ID_misc_read[i] = m_mcpat->sys.core[i].total_instructions; 
	/*Below is for O3*/
	 if (machineType == 0){
	  //LOAD_Q
	  mycounts.loadQ_read[i] = m_mcpat->sys.core[i].load_instructions + m_mcpat->sys.core[i].store_instructions;
	  mycounts.loadQ_write[i] = m_mcpat->sys.core[i].load_instructions + m_mcpat->sys.core[i].store_instructions;
	  //scheduler_u
	  mycounts.int_win_read[i] = m_mcpat->sys.core[i].inst_window_reads;
    	  mycounts.int_win_write[i] = m_mcpat->sys.core[i].inst_window_writes;
    	  mycounts.int_win_search[i] = m_mcpat->sys.core[i].inst_window_wakeup_accesses;
    	  mycounts.fp_win_read[i] = m_mcpat->sys.core[i].fp_inst_window_reads;
    	  mycounts.fp_win_write[i] = m_mcpat->sys.core[i].fp_inst_window_writes;
    	  mycounts.fp_win_search[i] = m_mcpat->sys.core[i].fp_inst_window_wakeup_accesses;
	  mycounts.ROB_read[i] = m_mcpat->sys.core[i].ROB_reads;
	  mycounts.ROB_write[i] = m_mcpat->sys.core[i].ROB_writes; 
	  //rename_u
	  mycounts.iFRAT_read[i] = m_mcpat->sys.core[i].rename_reads;
   	  mycounts.iFRAT_write[i] = 0; //rename_write
	  mycounts.iFRAT_search[i] = 0;  //N/A
	  mycounts.fFRAT_read[i] = m_mcpat->sys.core[i].fp_rename_reads;  
	  mycounts.fFRAT_write[i] = 0; //fp_rename_write
	  mycounts.fFRAT_search[i] = 0; //N/A
	  mycounts.iRRAT_read[i] = 16*(m_mcpat->sys.core[i].context_switches + m_mcpat->sys.core[i].branch_mispredictions);
	  mycounts.iRRAT_write[i] = 0; //rename_write
	  mycounts.fRRAT_read[i] = 16*(m_mcpat->sys.core[i].context_switches + m_mcpat->sys.core[i].branch_mispredictions);
	  mycounts.fRRAT_write[i] = 0; //rename_write
	  mycounts.ifreeL_read[i] =  m_mcpat->sys.core[i].rename_reads;
	  mycounts.ifreeL_write[i] = 0; //rename_write
	  mycounts.ffreeL_read[i] =  m_mcpat->sys.core[i].fp_rename_reads;
	  mycounts.ffreeL_write[i] = 0; //fp_rename_write
	  mycounts.idcl_read[i] = 3*4*4*m_mcpat->sys.core[i].rename_reads; //decodeW = 4 in Clovertown
	  mycounts.fdcl_read[i] = 3*4*4*m_mcpat->sys.core[i].fp_rename_reads; //fp_issueW = 4 in Clovertown
	  //BTB
	  mycounts.BTB_read[i] = m_mcpat->sys.core[i].BTB.read_accesses;
	  mycounts.BTB_write[i] = m_mcpat->sys.core[i].BTB.write_accesses;
	  //BPT
	  mycounts.branch_read[i] = m_mcpat->sys.core[i].branch_instructions;
	  mycounts.branch_write[i] = m_mcpat->sys.core[i].branch_mispredictions + 0.1*m_mcpat->sys.core[i].branch_instructions;  
	  } //end machineType	   
  }

  for (unsigned int i = 0; i < m_mcpat->sys.number_of_L2s; i++){
	  //L2
	  mycounts.L2_read[i] = m_mcpat->sys.L2[i].read_accesses;
	  mycounts.L2_readmiss[i] = m_mcpat->sys.L2[i].read_misses;
	  mycounts.L2_write[i] = m_mcpat->sys.L2[i].write_accesses;
	  mycounts.L2_writemiss[i] = m_mcpat->sys.L2[i].write_misses;
	  ////std:cout << "read_access = " << m_mcpat->sys.core[i].l2cache_read_accesses[i] << ", read_miss = " << 
	  ////			m_mcpat->sys.core[i].l2cache_read_misses[i] << ", temp read = " << tempcounts.L2_read[i] << endl;

	  std::cout << "mycounts.L2_read[" << i << "] = " << mycounts.L2_read[i] << std::endl;

  }

  pdata= power->getPower(this, CACHE_IL1, mycounts);
  	pdata= power->getPower(this, CACHE_DL1, mycounts);
	pdata= power->getPower(this, CACHE_L2, mycounts);
	pdata= power->getPower(this, RF, mycounts);
	pdata= power->getPower(this, EXEU_ALU, mycounts);
	pdata= power->getPower(this, LSQ, mycounts);
	pdata= power->getPower(this, IB, mycounts);
	pdata= power->getPower(this, INST_DECODER, mycounts);
	if (machineType == 0){ 
	    pdata= power->getPower(this, LOAD_Q, mycounts);
	    pdata= power->getPower(this, SCHEDULER_U, mycounts);
	    pdata= power->getPower(this, RENAME_U, mycounts);
	    pdata= power->getPower(this, BTB, mycounts);
	    pdata= power->getPower(this, BPRED, mycounts);
	}
  power->compute_temperature(getId());
  regPowerStats(pdata);

  //reset all counts to zero for next power query
  power->resetCounts(&mycounts);
	
#if 1
	using namespace io_interval; std::cout <<"ID " << getId() <<": current total power = " << pdata.currentPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": leakage power = " << pdata.leakagePower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": runtime power = " << pdata.runtimeDynamicPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": total energy = " << pdata.totalEnergy << " J" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": peak power = " << pdata.peak << " W" << std::endl;
#endif


  // return false so we keep going
  return false;
}



// Element Libarary / Serialization stuff   
BOOST_CLASS_EXPORT(parser)

static Component*
create_parser(SST::ComponentId_t id, 
                  SST::Component::Params_t& params)
{
    return new parser( id, params );
}

static const ElementInfoComponent components[] = {
    { "parser",
      "Parser for M5",
      NULL,
      create_parser
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo parser_eli = {
        "parser",
        "Parser for M5",
        components,
    };
}
