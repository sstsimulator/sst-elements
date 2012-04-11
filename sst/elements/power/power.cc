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

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include "power.h"
#include "interface.h"
#include "HotSpot-interface.h"
#include "reliability.h"
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include <boost/mpi.hpp> 

namespace SST {

std::multimap<ptype,int> Power::subcompList;
parameters_chip_t Power::chip;
chip_t Power::p_chip;
int Power::p_NumCompNeedPower;
int Power::p_SumNumCompNeedPower;
int Power::p_TempSumNumCompNeedPower;
bool Power::p_hasUpdatedTemp;
double Power::p_TotalFailureRate;
unsigned int Power::p_NumSamples;
std::vector<double> Power::p_minTemperature;
std::vector<double> Power::p_maxTemperature;
std::vector<std::vector<double> > Power::p_thermalFreq; 

/*********************
* Power:: constructor*
**********************/
Power::Power()
{
}


void Power::test()
{
	     opt_for_clk	=true;
	     p_Mp1->parse("/home/myhsieh/Desktop/latest/trunk/sst/core/techModels/libMcPATbeta/Niagara1.xml");
	     ////McPATSetup();	    
	     p_Mproc.initialize(p_Mp1); //unit energy is computed by McPAT in this step
	    
	     p_Mcore = p_Mproc.SSTreturnCore(0); //return core
	     ifu = p_Mcore->SSTreturnIFU();
	     lsu = p_Mcore->SSTreturnLSU();
	     mmu = p_Mcore->SSTreturnMMU();
	     exu = p_Mcore->SSTreturnEXU();
	     rnu = p_Mcore->SSTreturnRNU();
}

/************************************************************************
* Decouple the power-related parameters from Component::Params_t params *
*************************************************************************/
void Power::setTech(ComponentId_t compID, Component::Params_t params, ptype power_type, pmodel power_model)
{
    #ifdef PANALYZER_H
    #ifdef LV2_PANALYZER_H
    double tdarea = 0;
    double tcnodeCeff = 0;
    #endif
    #endif

    char chtmp[60];
    char chtmp1[60];
    chtmp1[0]='\0';
    unsigned int i, n;
    boost::mpi::communicator world;


  //number of (sub)components on the chip per rank; we only care about this when thermal modeling is needed
  if (p_powerMonitor == true && p_tempMonitor == true){
      chip.num_comps +=1;
      //p_NumCompNeedPower = chip.num_comps;
  }
  if (world.size() == 1)
      p_NumCompNeedPower = chip.num_comps; 
  else {
      // get total number of (sub)components on the chip over ranks
      all_reduce(world, chip.num_comps, chip.sumnum_comps, std::plus<int>());
      p_TempSumNumCompNeedPower = chip.sumnum_comps;    
      //std::cout << "chip.sumnum_comps = " << chip.sumnum_comps << std::endl;
  }


  // read values from xml
  if(p_ifReadEntireXML == false){
    //Save computational time for calls to McPAT. For McPAT's case, XML is read in once and all the params
    // are set up during the 1st setTech call. So there is no need to read the XML again if it has done so.
 
    //set up architecture parameter values
    Component::Params_t::iterator it= params.begin();

    while (it != params.end()){
	//NOTE: params are NOT read in order as they apprears in XML
        if (!it->first.compare("power_level")){ //lv2
	        sscanf(it->second.c_str(), "%d", &p_powerLevel);
	}
	else if (!it->first.compare("McPAT_XMLfile")){ //Mc
	        p_McPATxmlpath = &it->second[0];
		//sscanf(it->second.c_str(), "%s", p_McPATxmlpath);
	}
	else if (!it->first.compare("if_parser")){ //Mc
		if (!it->second.compare("YES"))
		   p_ifParser = true;
	        else
		   p_ifParser = false;
	}
	else {
	    if (power_model == McPAT || power_model == McPAT05){
		//if it's the case of McPAT, read in all tech param at once to 
		//reduce #calls to McPAT power estimation functions/computational time 

	        //cache_il1		                       
                        if (!it->first.compare("cache_il1_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_il1_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.unit_icap);
			}
			else if (!it->first.compare("cache_il1_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){ //lv2
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){  //lv2 
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.op_freq);
			}
                        else if (!it->first.compare("cache_il1_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_rports);
			}
			else if (!it->first.compare("cache_il1_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_wports);
			}
			else if(!it->first.compare("cache_il1_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_il1_number_sets")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_sets);
			}
			else if(!it->first.compare("cache_il1_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_il1_number_bitlines")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_il1_number_wordlines")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_il1_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_il1_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_il1_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_il1_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_il1_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if(!it->first.compare("cache_il1_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_il1_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_il1_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_il1_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if (!it->first.compare("cache_l1dir_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l1dir_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l1dir_tech.op_freq);
			}
			else if(!it->first.compare("cache_l1dir_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l1dir_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l1dir_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l1dir_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_l1dir_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l1dir_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l1dir_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l1dir_tech.device_type);
			}
			else if(!it->first.compare("cache_l1dir_directory_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l1dir_tech.directory_type);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
       			//cache_dl1		
                        else if (!it->first.compare("cache_dl1_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_dl1_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.unit_icap);
			}
			else if (!it->first.compare("cache_dl1_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.op_freq);
			}
                        else if (!it->first.compare("cache_dl1_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_rports);
			}
			else if (!it->first.compare("cache_dl1_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_wports);
			}
			else if(!it->first.compare("cache_dl1_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_dl1_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_sets);
			}
			else if(!it->first.compare("cache_dl1_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_dl1_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_dl1_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_dl1_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_dl1_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_dl1_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		 
			else if(!it->first.compare("cache_dl1_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_dl1_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_dl1_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			//cache_itlb		
                        else if (!it->first.compare("cache_itlb_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_itlb_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.unit_icap);
			}
			else if (!it->first.compare("cache_itlb_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.op_freq);
			}
                        else if (!it->first.compare("cache_itlb_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_rports);
			}
			else if (!it->first.compare("cache_itlb_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_wports);
			}
			else if(!it->first.compare("cache_itlb_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_itlb_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_sets);
			}
			else if(!it->first.compare("cache_itlb_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_itlb_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_itlb_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_itlb_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if(!it->first.compare("core_virtual_memory_page_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_memory_page_size);
			}
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}			
			else if(!it->first.compare("cache_itlb_number_entries")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.number_entries);
			} 
			else if(!it->first.compare("core_number_instruction_fetch_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_instruction_fetch_ports);
			} 
			//cache_dtlb		
                        else if (!it->first.compare("cache_dtlb_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_dtlb_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.unit_icap);
			}
			else if (!it->first.compare("cache_dtlb_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.op_freq);
			}
                        else if (!it->first.compare("cache_dtlb_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_rports);
			}
			else if (!it->first.compare("cache_dtlb_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_wports);
			}
			else if(!it->first.compare("cache_dtlb_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_dtlb_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_sets);
			}
			else if(!it->first.compare("cache_dtlb_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_dtlb_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_dtlb_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_dtlb_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_dtlb_number_entries")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.number_entries);
			} 
			//bpred			    		
			else if (!it->first.compare("bpred_iC")){
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_icap);
			}
			else if (!it->first.compare("bpred_eC")){
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_ecap);
			}
			else if (!it->first.compare("bpred_sC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_scap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.vss);
			}
			else if (!it->first.compare("bpred_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.op_freq);
			}
			else if (!it->first.compare("bpred_number_rows")){  //lv2
			    sscanf(it->second.c_str(), "%d", &bpred_tech.nrows);
			}
			else if (!it->first.compare("bpred_number_cols")){  //lv2
			    sscanf(it->second.c_str(), "%d", &bpred_tech.ncols);
			}
			else if (!it->first.compare("bpred_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_rports);
			}
			else if (!it->first.compare("bpred_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_wports);
			}
			else if(!it->first.compare("bpred_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_rwports);
			} 
			else if (!it->first.compare("bpred_global_predictor_bits")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.global_predictor_bits);
			}
			else if (!it->first.compare("bpred_global_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.global_predictor_entries);
			}
			else if (!it->first.compare("bpred_prediction_width")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.prediction_width);
			}
			else if (!it->first.compare("bpred_local_predictor_size")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.local_predictor_size);
			}
			else if (!it->first.compare("bpred_local_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.local_predictor_entries);
			}
			else if (!it->first.compare("bpred_chooser_predictor_bits")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.chooser_predictor_bits);
			}
			else if (!it->first.compare("bpred_chooser_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.chooser_predictor_entries);
			}
			else if(!it->first.compare("archi_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			} 			
			else if(!it->first.compare("core_RAS_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_RAS_size);
			} 	    	
			//rf
			else if (!it->first.compare("rf_iC")){
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_icap);
			}
			else if (!it->first.compare("rf_eC")){
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_ecap);
			}
			else if (!it->first.compare("rf_sC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_scap);  
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.vss);
			}
			else if (!it->first.compare("rf_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.op_freq);
			}
			else if (!it->first.compare("rf_number_rows")){  //lv2
			    sscanf(it->second.c_str(), "%d", &rf_tech.nrows);
			}
			else if (!it->first.compare("rf_number_cols")){  //lv2
			    sscanf(it->second.c_str(), "%d", &rf_tech.ncols);
			}
			else if (!it->first.compare("rf_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_rports);
			}
			else if (!it->first.compare("rf_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_wports);
			}
			else if(!it->first.compare("rf_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_rwports);
			}
			else if(!it->first.compare("machine_bits")){  //Mc   
				sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			} 
			else if(!it->first.compare("archi_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			} 
			else if(!it->first.compare("core_phy_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			} 
			else if(!it->first.compare("core_phy_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			} 
			else if(!it->first.compare("core_issue_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			} 
			else if(!it->first.compare("core_peak_issue_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_peak_issue_width);
			} 
			else if(!it->first.compare("core_register_windows_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_register_windows_size);
			} 
			else if(!it->first.compare("core_opcode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			} 
			else if(!it->first.compare("core_micro_opcode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_micro_opcode_width);
			} 	
			//logic
			else if (!it->first.compare("logic_sC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_scap);
			}
			else if (!it->first.compare("logic_iC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_icap);
			}
			else if (!it->first.compare("logic_lC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_lcap);
			}
			else if (!it->first.compare("logic_eC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &logic_tech.vss);
			}
			else if (!it->first.compare("logic_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &logic_tech.op_freq);
			}
			else if(!it->first.compare("logic_style")) {  //lv2
			    if (!it->second.compare("STATIC"))
				logic_tech.lgc_style = STATIC;
			    else if (!it->second.compare("DYNAMIC"))
				logic_tech.lgc_style = DYNAMIC;
			}
			else if(!it->first.compare("logic_num_gates")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_gates);
			}
			else if(!it->first.compare("logic_num_functions")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_functions);
			}
			else if(!it->first.compare("logic_num_fan_in")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_fan_in);
			}
			else if(!it->first.compare("logic_num_fan_out")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_fan_out);
			}
			else if(!it->first.compare("core_instruction_window_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_window_size);
			}
			else if(!it->first.compare("core_decode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_decode_width);
			} 
			// ALU	
			else if (!it->first.compare("alu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_scap);
			}
			else if (!it->first.compare("alu_iC")){
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_icap);
			}
			else if (!it->first.compare("alu_lC")){
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_lcap);
			}
			else if (!it->first.compare("alu_eC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.vss);
			}
			else if (!it->first.compare("alu_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.op_freq);
			}
			//FPU
			else if (!it->first.compare("fpu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_scap);
			}
			else if (!it->first.compare("fpu_iC")){
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_icap);
			}
			else if (!it->first.compare("fpu_lC")){
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_lcap);
			}
			else if (!it->first.compare("fpu_eC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.vss);
			}
			else if (!it->first.compare("fpu_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.op_freq);
			}
			//IB	
			else if (!it->first.compare("core_instruction_length")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_length);
			}
			else if (!it->first.compare("core_instruction_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_buffer_size);
			}
			else if(!it->first.compare("core_longer_channel_device")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_long_channel);
			} 
			else if (!it->first.compare("ib_number_readwrite_ports")){
			    sscanf(it->second.c_str(), "%d", &ib_tech.num_rwports);
			}
			//ISSUE_Q
			//INST DECODER	
			//BYPASS	
			else if (!it->first.compare("ALU_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.ALU_per_core);
			}
			else if (!it->first.compare("FPU_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.FPU_per_core);
			}
			else if (!it->first.compare("MUL_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.MUL_per_core);
			}
			else if (!it->first.compare("core_store_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_store_buffer_size);
			}
			else if (!it->first.compare("core_memory_ports")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_memory_ports);
			}
			 //EXEU	
			else if (!it->first.compare("exeu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &C_EXEU);
			}
			//PIPELINE	
			else if (!it->first.compare("core_fetch_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_fetch_width);
			}
			else if (!it->first.compare("core_commit_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_commit_width);
			}
			else if (!it->first.compare("core_instruction_length")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_length);
			}
			else if (!it->first.compare("core_int_pipeline_depth")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_int_pipeline_depth);
			}
			//LSQ & LOAD_Q			
			else if (!it->first.compare("core_load_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_load_buffer_size);
			}
			//RAT
			else if (!it->first.compare("core_ROB_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_ROB_size);
			}
			//ROB
			//BTB
			else if (!it->first.compare("btb_sC")){
			    sscanf(it->second.c_str(), "%lf", &btb_tech.unit_scap);
			}
			else if(!it->first.compare("btb_associativity")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.assoc);
			}  
			else if(!it->first.compare("btb_throughput")){  //Mc
				sscanf(it->second.c_str(), "%lf", &btb_tech.throughput);
			} 
			else if(!it->first.compare("btb_latency")){  //Mc
				sscanf(it->second.c_str(), "%lf", &btb_tech.latency);
			} 
			else if(!it->first.compare("btb_number_banks")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.num_banks);
			} 
			else if(!it->first.compare("btb_line_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.line_size);
			} 
			 //cache_l2 & l2dir		                        
                        else if (!it->first.compare("cache_l2_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';

			        //sscanf(it->second.c_str(), "%lf", &cache_l2_tech.unit_scap);
			}
			else if (!it->first.compare("cache_l2_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l2_tech.op_freq);
			}
			else if(!it->first.compare("cache_l2_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';

				//sscanf(it->second.c_str(), "%d", &cache_l2_tech.line_size);
			}
			else if(!it->first.compare("cache_l2_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
				//sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_banks);
			} 
			else if(!it->first.compare("cache_l2_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l2_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}		
                        else if (!it->first.compare("cache_l2_number_read_ports")){ 
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_rports);
			}
			else if (!it->first.compare("cache_l2_number_write_ports")){  
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_wports);
			}
			else if(!it->first.compare("cache_l2_number_readwrite_ports")){ 
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_rwports);
			}  			
			else if(!it->first.compare("cache_l2_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}   
			else if(!it->first.compare("cache_l2_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.device_type);
			}
			else if (!it->first.compare("cache_l2dir_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l2dir_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l2dir_tech.op_freq);
			}
			else if(!it->first.compare("cache_l2dir_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l2dir_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_l2dir_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2dir_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2dir_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2dir_tech.device_type);
			}
			else if(!it->first.compare("cache_l2dir_directory_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2dir_tech.directory_type);
			}
			//MC
			else if (!it->first.compare("mc_clock_rate")){  //Mc  
			    sscanf(it->second.c_str(), "%lf", &mc_tech.mc_clock);
			}
			else if (!it->first.compare("mc_llc_line_length")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.llc_line_length);
			}
			else if (!it->first.compare("mc_databus_width")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.databus_width);
			}
			else if (!it->first.compare("mc_addressbus_width")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.addressbus_width);
			}
			else if(!it->first.compare("mc_req_window_size_per_channel")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.req_window_size_per_channel);
			}  
			else if(!it->first.compare("mc_memory_channels_per_mc")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_channels_per_mc);
			} 
			else if(!it->first.compare("mc_IO_buffer_size_per_channel")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.IO_buffer_size_per_channel);
			} 
			else if(!it->first.compare("mainmemory_number_ranks")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_number_ranks);
			} 
			else if(!it->first.compare("mainmemory_peak_transfer_rate")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_peak_transfer_rate);
			} 
			//ROUTER
			else if (!it->first.compare("router_clock_rate")){  //Mc  
			    sscanf(it->second.c_str(), "%lf", &router_tech.clockrate);
			}
			else if (!it->first.compare("router_has_global_link")){
			    sscanf(it->second.c_str(), "%d", &router_tech.has_global_link);
			}
			else if (!it->first.compare("router_link_length")){
			    sscanf(it->second.c_str(), "%lf", &router_tech.link_length);
			}
			else if (!it->first.compare("router_flit_bits")){
			    sscanf(it->second.c_str(), "%d", &router_tech.flit_bits);
			}
			else if (!it->first.compare("router_input_buffer_entries_per_vc")){
			    sscanf(it->second.c_str(), "%d", &router_tech.input_buffer_entries_per_vc);
			}
			else if(!it->first.compare("router_virtual_channel_per_port")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.virtual_channel_per_port);
			}  
			else if(!it->first.compare("router_input_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.input_ports);
			} 
			else if(!it->first.compare("router_output_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.output_ports);
			} 
			else if(!it->first.compare("router_link_throughput")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.link_throughput);
			} 
			else if(!it->first.compare("router_link_latency")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.link_latency);
			} 
			else if(!it->first.compare("router_horizontal_nodes")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.horizontal_nodes);
			} 
			else if (!it->first.compare("router_vertical_nodes")){
			    sscanf(it->second.c_str(), "%d", &router_tech.vertical_nodes);
			}
			else if(!it->first.compare("router_topology")) {  //Mc
			    if (!it->second.compare("2DMESH"))
				router_tech.topology = TWODMESH;
			    else if (!it->second.compare("RING"))
				router_tech.topology = RING;
			    else if (!it->second.compare("CROSSBAR"))
				router_tech.topology = CROSSBAR;
			}
			else if(!it->first.compare("core_number_of_NoCs")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_of_NoCs);
			} 
			//RENAME_U	
			else if (!it->first.compare("core_fp_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_fp_issue_width);
			}	
			//SCHEDULER_U	
			else if(!it->first.compare("core_fp_instruction_window_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_fp_instruction_window_size);
			}
			//CACHE_L3	
			else if (!it->first.compare("cache_l3_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l3_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.unit_icap);
			}
			else if (!it->first.compare("cache_l3_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.vss);
			}
                        else if (!it->first.compare("cache_l3_clockrate")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.op_freq);
			}
                        else if (!it->first.compare("cache_l3_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_rports);
			}
			else if (!it->first.compare("cache_l3_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_wports);
			}
			else if(!it->first.compare("cache_l3_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_l3_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_sets);
			}
			else if(!it->first.compare("cache_l3_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l3_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_l3_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_l3_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l3_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l3_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l3_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_l3_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l3_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l3_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}   
			else if(!it->first.compare("cache_l3_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.device_type);
			}
	    } 
	    else{
	      switch(power_type)
	      {
	        case 0:  //cache_il1		
                        
                        if (!it->first.compare("cache_il1_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_il1_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.unit_icap);
			}
			else if (!it->first.compare("cache_il1_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){ //lv2
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){  //lv2 
			    sscanf(it->second.c_str(), "%lf", &cache_il1_tech.op_freq);
			}
                        else if (!it->first.compare("cache_il1_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_rports);
			}
			else if (!it->first.compare("cache_il1_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_wports);
			}
			else if(!it->first.compare("cache_il1_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_il1_number_sets")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_sets);
			}
			else if(!it->first.compare("cache_il1_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_il1_number_bitlines")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_il1_number_wordlines")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il1_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_il1_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_il1_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_il1_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_il1_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_il1_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if(!it->first.compare("cache_il1_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_il1_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_il1_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_il1_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_il1_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_il1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_il1_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_il1_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_il1_tech.area);
			}
			else if(!it->first.compare("cache_il1_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_il1_tech.num_transistors);
			}
			else if (!it->first.compare("cache_l1dir_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l1dir_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l1dir_tech.op_freq);
			}
			else if(!it->first.compare("cache_l1dir_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l1dir_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l1dir_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l1dir_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_l1dir_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l1dir_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l1dir_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l1dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L1dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l1dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l1dir_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l1dir_tech.device_type);
			}
			else if(!it->first.compare("cache_l1dir_directory_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l1dir_tech.directory_type);
			}
			else if(!it->first.compare("cache_l1dir_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l1dir_tech.area);
			}
			else if(!it->first.compare("cache_l1dir_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l1dir_tech.num_transistors);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
	        break;
	        case 1:  //cache_il2		
                        if (!it->first.compare("cache_il2_sC")){  //Mc
				cache_il2_tech.unit_scap.at(0) = atoi(it->second.c_str());
			}
			else if (!it->first.compare("cache_il2_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il2_tech.unit_icap);
			}
			else if (!it->first.compare("cache_il2_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_il2_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_il2_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_il2_tech.op_freq);
			}
                        else if (!it->first.compare("cache_il2_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_rports);
			}
			else if (!it->first.compare("cache_il2_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_wports);
			}
			else if(!it->first.compare("cache_il2_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_il2_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_sets);
			}
			else if (!it->first.compare("cache_il2_line_size")){  //Mc
				cache_il2_tech.line_size.at(0) = atoi(it->second.c_str());
			}
			else if(!it->first.compare("cache_il2_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_il2_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_il2_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_il2_associativity")){
				cache_il2_tech.assoc.at(0) = atoi(it->second.c_str());
			} 
			else if(!it->first.compare("cache_il2_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_il2_tech.area);
			}
			else if(!it->first.compare("cache_il2_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_il2_tech.num_transistors);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			} 
	        break;
	        case 2:  //cache_dl1		
                        if (!it->first.compare("cache_dl1_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_dl1_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.unit_icap);
			}
			else if (!it->first.compare("cache_dl1_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.op_freq);
			}
                        else if (!it->first.compare("cache_dl1_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_rports);
			}
			else if (!it->first.compare("cache_dl1_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_wports);
			}
			else if(!it->first.compare("cache_dl1_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_dl1_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_sets);
			}
			else if(!it->first.compare("cache_dl1_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_dl1_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_dl1_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl1_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_dl1_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_dl1_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_dl1_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if(!it->first.compare("cache_dl1_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}   
			else if(!it->first.compare("cache_dl1_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dl1_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dl1_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dl1);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dl1_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_dl1_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.area);
			}
			else if(!it->first.compare("cache_dl1_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dl1_tech.num_transistors);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			}  
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
	        break;
	        case 3:  //cache_dl2		
                        if (!it->first.compare("cache_dl2_sC")){  //Mc			 
				cache_dl2_tech.unit_scap.at(0) = atoi(it->second.c_str());
			}
			else if (!it->first.compare("cache_dl2_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.unit_icap);
			}
			else if (!it->first.compare("cache_dl2_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.op_freq);
			}
                        else if (!it->first.compare("cache_dl2_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_rports);
			}
			else if (!it->first.compare("cache_dl2_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_wports);
			}
			else if(!it->first.compare("cache_dl2_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_dl2_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_sets);
			}
			else if (!it->first.compare("cache_dl2_line_size")){  //Mc			 
				cache_dl2_tech.line_size.at(0) = atoi(it->second.c_str());
			}
			else if(!it->first.compare("cache_dl2_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_dl2_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_dl2_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_dl2_associativity")){
				cache_dl2_tech.assoc.at(0) = atoi(it->second.c_str());
			}  	
			else if(!it->first.compare("cache_dl2_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.area);
			}
			else if(!it->first.compare("cache_dl2_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dl2_tech.num_transistors);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}		    
	        break;
	        case 4:  //cache_itlb		
                        if (!it->first.compare("cache_itlb_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_itlb_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.unit_icap);
			}
			else if (!it->first.compare("cache_itlb_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.op_freq);
			}
                        else if (!it->first.compare("cache_itlb_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_rports);
			}
			else if (!it->first.compare("cache_itlb_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_wports);
			}
			else if(!it->first.compare("cache_itlb_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_itlb_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_sets);
			}
			else if(!it->first.compare("cache_itlb_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_itlb_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_itlb_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_itlb_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_itlb_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_itlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_itlb_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_itlb_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.area);
			}
			else if(!it->first.compare("cache_itlb_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_itlb_tech.num_transistors);
			}
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if(!it->first.compare("core_virtual_memory_page_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_memory_page_size);
			}
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			}
			else if(!it->first.compare("cache_itlb_number_entries")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_itlb_tech.number_entries);
			} 
			else if(!it->first.compare("core_number_instruction_fetch_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_instruction_fetch_ports);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 	
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}    
	        break;
	        case 5:  //cache_dtlb		
                        if (!it->first.compare("cache_dtlb_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_dtlb_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.unit_icap);
			}
			else if (!it->first.compare("cache_dtlb_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.vss);
			}
                        else if (!it->first.compare("cache_freq")){
			    sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.op_freq);
			}
                        else if (!it->first.compare("cache_dtlb_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_rports);
			}
			else if (!it->first.compare("cache_dtlb_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_wports);
			}
			else if(!it->first.compare("cache_dtlb_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_dtlb_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_sets);
			}
			else if(!it->first.compare("cache_dtlb_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_dtlb_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_dtlb_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_dtlb_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_dtlb_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_dtlb);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_dtlb_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_dtlb_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.area);
			}
			else if(!it->first.compare("cache_dtlb_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_dtlb_tech.num_transistors);
			}
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if(!it->first.compare("core_virtual_memory_page_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_memory_page_size);
			}
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			}
			else if(!it->first.compare("cache_dtlb_number_entries")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_dtlb_tech.number_entries);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}	    		    
	        break;	        
	        case 6:   //clock
			if (!it->first.compare("clock_sC")){
			    sscanf(it->second.c_str(), "%lf", &clock_tech.unit_scap);
			}
			else if (!it->first.compare("clock_iC")){
			    sscanf(it->second.c_str(), "%lf", &clock_tech.unit_icap);
			}
			else if (!it->first.compare("clock_lC")){
			    sscanf(it->second.c_str(), "%lf", &clock_tech.unit_lcap);
			}
			else if (!it->first.compare("clock_eC")){
			    sscanf(it->second.c_str(), "%lf", &clock_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &clock_tech.vss);
			}
			else if (!it->first.compare("clock_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &clock_tech.op_freq);
			}            
			else if(!it->first.compare("clock_style")) {  //lv2
			    if (!it->second.compare("NORM_H"))
				clock_tech.clk_style = NORM_H;
			    else if (!it->second.compare("BALANCED_H"))
				clock_tech.clk_style = BALANCED_H;				
			}
			else if(!it->first.compare("clock_option")) {  //intsim
			    if (!it->second.compare("GLOBAL_CLOCK"))
				clock_tech.clock_option = GLOBAL_CLOCK;
			    else if (!it->second.compare("LOCAL_CLOCK"))
				clock_tech.clock_option = LOCAL_CLOCK;
			    else if (!it->second.compare("TOTAL_CLOCK"))
				clock_tech.clock_option = TOTAL_CLOCK;				
			}
			else if(!it->first.compare("clock_skew")){ //lv2
				sscanf(it->second.c_str(), "%lf", &clock_tech.skew);
			}
			else if(!it->first.compare("clock_chip_area")){
				sscanf(it->second.c_str(), "%d", &clock_tech.chip_area);
			}
			else if(!it->first.compare("clock_node_cap")){
				sscanf(it->second.c_str(), "%lf", &clock_tech.node_cap);
			}
			else if(!it->first.compare("clock_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &clock_tech.area);
			}
			else if(!it->first.compare("clock_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &clock_tech.num_transistors);
			}
			else if(!it->first.compare("opt_clock_buffer_num")){  //lv2
				sscanf(it->second.c_str(), "%d", &clock_tech.opt_clock_buffer_num);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
		break;
		case 7: //bpred
				    		
			if (!it->first.compare("bpred_iC")){
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_icap);
			}
			else if (!it->first.compare("bpred_eC")){
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_ecap);
			}
			else if (!it->first.compare("bpred_sC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.unit_scap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.vss);
			}
			else if (!it->first.compare("bpred_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &bpred_tech.op_freq);
			}
			else if (!it->first.compare("bpred_number_rows")){  //lv2
			    sscanf(it->second.c_str(), "%d", &bpred_tech.nrows);
			}
			else if (!it->first.compare("bpred_number_cols")){  //lv2
			    sscanf(it->second.c_str(), "%d", &bpred_tech.ncols);
			}
			else if (!it->first.compare("bpred_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_rports);
			}
			else if (!it->first.compare("bpred_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_wports);
			}
			else if(!it->first.compare("bpred_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &bpred_tech.num_rwports);
			} 
			else if (!it->first.compare("bpred_global_predictor_bits")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.global_predictor_bits);
			}
			else if (!it->first.compare("bpred_global_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.global_predictor_entries);
			}
			else if (!it->first.compare("bpred_prediction_width")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.prediction_width);
			}
			else if (!it->first.compare("bpred_local_predictor_size")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.local_predictor_size);
			}
			else if (!it->first.compare("bpred_local_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.local_predictor_entries);
			}
			else if (!it->first.compare("bpred_chooser_predictor_bits")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.chooser_predictor_bits);
			}
			else if (!it->first.compare("bpred_chooser_predictor_entries")){  //Mc
			    sscanf(it->second.c_str(), "%d", &bpred_tech.chooser_predictor_entries);
			}
			else if(!it->first.compare("bpred_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &bpred_tech.area);
			}
			else if(!it->first.compare("bpred_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &bpred_tech.num_transistors);
			}
			else if(!it->first.compare("archi_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			} 
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			} 
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}  
			else if(!it->first.compare("core_RAS_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_RAS_size);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			} 
                break;
		case 8:  //rf
			if (!it->first.compare("rf_iC")){
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_icap);
			}
			else if (!it->first.compare("rf_eC")){
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_ecap);
			}
			else if (!it->first.compare("rf_sC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.unit_scap);  
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.vss);
			}
			else if (!it->first.compare("rf_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &rf_tech.op_freq);
			}
			else if (!it->first.compare("rf_number_rows")){  //lv2
			    sscanf(it->second.c_str(), "%d", &rf_tech.nrows);
			}
			else if (!it->first.compare("rf_number_cols")){  //lv2
			    sscanf(it->second.c_str(), "%d", &rf_tech.ncols);
			}
			else if (!it->first.compare("rf_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_rports);
			}
			else if (!it->first.compare("rf_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_wports);
			}
			else if(!it->first.compare("rf_number_readwrite_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &rf_tech.num_rwports);
			}
			else if(!it->first.compare("rf_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &rf_tech.area);
			}
			else if(!it->first.compare("rf_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &rf_tech.num_transistors);
			}
			else if(!it->first.compare("machine_bits")){  //Mc   
				sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			} 
			else if(!it->first.compare("archi_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			} 
			else if(!it->first.compare("archi_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			} 
			else if(!it->first.compare("core_phy_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			} 
			else if(!it->first.compare("core_phy_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			} 
			else if(!it->first.compare("core_issue_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			} 
			else if(!it->first.compare("core_peak_issue_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_peak_issue_width);
			} 
			else if(!it->first.compare("core_register_windows_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_register_windows_size);
			} 
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if(!it->first.compare("core_opcode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			} 
			else if(!it->first.compare("core_micro_opcode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_micro_opcode_width);
			} 
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}  
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}   
		break;		
		case 9:  //io	    		               
			if (!it->first.compare("io_sC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &io_tech.unit_scap);
			}
			else if (!it->first.compare("io_iC")){
			    sscanf(it->second.c_str(), "%lf", &io_tech.unit_icap);
			}
			else if (!it->first.compare("io_lC")){
			    sscanf(it->second.c_str(), "%lf", &io_tech.unit_lcap);
			}
			else if (!it->first.compare("io_eC")){
			    sscanf(it->second.c_str(), "%lf", &io_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &io_tech.vss);
			}
			else if (!it->first.compare("io_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &io_tech.op_freq);
			}            
			else if(!it->first.compare("io_style")) {  //lv2
			    if (!it->second.compare("IN"))
				io_tech.i_o_style = IN;
			    else if (!it->second.compare("OUT"))
				io_tech.i_o_style = OUT;
			    else if (!it->second.compare("BI"))
				io_tech.i_o_style = BI;
			}
			else if(!it->first.compare("opt_io_buffer_num")){  //lv2
				sscanf(it->second.c_str(), "%d", &io_tech.opt_io_buffer_num);
			}
			else if(!it->first.compare("io_ustrip_len")){  //lv2
				sscanf(it->second.c_str(), "%lf", &io_tech.ustrip_len);
			}
			else if(!it->first.compare("io_bus_width")){  //lv2
				sscanf(it->second.c_str(), "%d", &io_tech.bus_width);
			}
			else if(!it->first.compare("io_transaction_size")){  //lv2
				sscanf(it->second.c_str(), "%d", &io_tech.bus_size);
			}
			else if(!it->first.compare("io_access_time")){  //lv2
				sscanf(it->second.c_str(), "%d", &io_tech.io_access_time);
			}
			else if(!it->first.compare("io_cycle_time")){  //lv2
				sscanf(it->second.c_str(), "%d", &io_tech.io_cycle_time);
			}
			else if(!it->first.compare("io_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &io_tech.area);
			}
			else if(!it->first.compare("io_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &io_tech.num_transistors);
			}
			break;	
		case 10:  //logic
			if (!it->first.compare("logic_sC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_scap);
			}
			else if (!it->first.compare("logic_iC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_icap);
			}
			else if (!it->first.compare("logic_lC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_lcap);
			}
			else if (!it->first.compare("logic_eC")){
			    sscanf(it->second.c_str(), "%lf", &logic_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &logic_tech.vss);
			}
			else if (!it->first.compare("logic_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &logic_tech.op_freq);
			}
			else if(!it->first.compare("logic_style")) {  //lv2
			    if (!it->second.compare("STATIC"))
				logic_tech.lgc_style = STATIC;
			    else if (!it->second.compare("DYNAMIC"))
				logic_tech.lgc_style = DYNAMIC;
			}
			else if(!it->first.compare("logic_num_gates")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_gates);
			}
			else if(!it->first.compare("logic_num_functions")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_functions);
			}
			else if(!it->first.compare("logic_num_fan_in")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_fan_in);
			}
			else if(!it->first.compare("logic_num_fan_out")){  //lv2
				sscanf(it->second.c_str(), "%d", &logic_tech.num_fan_out);
			}
			else if(!it->first.compare("logic_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &logic_tech.area);
			}
			else if(!it->first.compare("logic_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &logic_tech.num_transistors);
			}
			else if(!it->first.compare("core_instruction_window_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_window_size);
			}
			else if(!it->first.compare("core_issue_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if(!it->first.compare("core_number_hardware_threads")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if(!it->first.compare("core_decode_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_decode_width);
			}
			else if(!it->first.compare("archi_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if(!it->first.compare("archi_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
		break;
		case 11:  // ALU	
			if (!it->first.compare("alu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_scap);
			}
			else if (!it->first.compare("alu_iC")){
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_icap);
			}
			else if (!it->first.compare("alu_lC")){
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_lcap);
			}
			else if (!it->first.compare("alu_eC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.vss);
			}
			else if (!it->first.compare("alu_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &alu_tech.op_freq);
			}
			else if(!it->first.compare("alu_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &alu_tech.area);
			}
			else if(!it->first.compare("alu_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &alu_tech.num_transistors);
			}
			break;
			
		case 12:  //FPU
			if (!it->first.compare("fpu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_scap);
			}
			else if (!it->first.compare("fpu_iC")){
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_icap);
			}
			else if (!it->first.compare("fpu_lC")){
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_lcap);
			}
			else if (!it->first.compare("fpu_eC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.vss);
			}
			else if (!it->first.compare("fpu_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &fpu_tech.op_freq);
			}
			else if(!it->first.compare("fpu_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &fpu_tech.area);
			}
			else if(!it->first.compare("fpu_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &fpu_tech.num_transistors);
			}
			break;
		case 13:  //MULT	
			if (!it->first.compare("mult_sC")){
			    sscanf(it->second.c_str(), "%lf", &mult_tech.unit_scap);
			}
			else if (!it->first.compare("mult_iC")){
			    sscanf(it->second.c_str(), "%lf", &mult_tech.unit_icap);
			}
			else if (!it->first.compare("mult_lC")){
			    sscanf(it->second.c_str(), "%lf", &mult_tech.unit_lcap);
			}
			else if (!it->first.compare("mult_eC")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &mult_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &mult_tech.vss);
			}
			else if (!it->first.compare("mult_freq")){  //lv2
			    sscanf(it->second.c_str(), "%lf", &mult_tech.op_freq);
			}
			else if(!it->first.compare("mult_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &mult_tech.area);
			}
			else if(!it->first.compare("mult_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &mult_tech.num_transistors);
			}
			break;
		case 14:  //IB	
			if (!it->first.compare("core_instruction_length")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_length);
			}
			else if (!it->first.compare("core_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("core_instruction_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_buffer_size);
			}
			else if(!it->first.compare("core_longer_channel_device")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_long_channel);
			} 
			else if (!it->first.compare("ib_number_readwrite_ports")){
			    sscanf(it->second.c_str(), "%d", &ib_tech.num_rwports);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if(!it->first.compare("core_virtual_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			} 
			else if(!it->first.compare("core_virtual_memory_page_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_memory_page_size);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if(!it->first.compare("ib_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &ib_tech.area);
			}
			else if(!it->first.compare("ib_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &ib_tech.num_transistors);
			}
			break;
		case 15:  //ISSUE_Q
			if (!it->first.compare("core_number_hardware_threads")){  //Mc   
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("core_instruction_length")){  //Mc
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_length);
			}
			else if (!it->first.compare("core_instruction_window_size")){  //Mc
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_window_size);
			}
			else if (!it->first.compare("core_issue_width")){  //Mc
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if(!it->first.compare("archi_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if(!it->first.compare("archi_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			}
			else if(!it->first.compare("core_phy_Regs_IRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			}
			else if(!it->first.compare("core_phy_Regs_FRF_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if(!it->first.compare("irs_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &irs_tech.area);
			}
			else if(!it->first.compare("irs_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &irs_tech.num_transistors);
			}
			break;
		case 16:  //INST DECODER	
			if (!it->first.compare("core_opcode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if(!it->first.compare("decoder_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &decoder_tech.area);
			}
			else if(!it->first.compare("decoder_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &decoder_tech.num_transistors);
			}
			break;
		case 17:  //BYPASS	
			if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("ALU_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.ALU_per_core);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("FPU_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.FPU_per_core);
			}
			else if (!it->first.compare("MUL_per_core")){
			    sscanf(it->second.c_str(), "%d", &core_tech.MUL_per_core);
			}
			else if (!it->first.compare("core_opcode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			}
			else if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("core_store_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_store_buffer_size);
			}
			else if (!it->first.compare("core_memory_ports")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_memory_ports);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if (!it->first.compare("core_phy_Regs_FRF_size")){  //Mc  
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			}
			else if(!it->first.compare("bypass_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &bypass_tech.area);
			}
			else if(!it->first.compare("bypass_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &bypass_tech.num_transistors);
			}
			break;
		case 18:  //EXEU	
			if (!it->first.compare("exeu_sC")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &C_EXEU);
			}	
			break;
		case 19:  //PIPELINE	
			if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("core_fetch_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_fetch_width);
			}
			else if (!it->first.compare("core_decode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_decode_width);
			}
			else if (!it->first.compare("core_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if (!it->first.compare("core_commit_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_commit_width);
			}
			else if (!it->first.compare("core_instruction_length")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_length);
			}
			else if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("core_opcode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			}
			else if (!it->first.compare("core_int_pipeline_depth")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_int_pipeline_depth);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("archi_Regs_IRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if(!it->first.compare("pipeline_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &pipeline_tech.area);
			}
			else if(!it->first.compare("pipeline_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &pipeline_tech.num_transistors);
			}
			break;
		case 20: case 27: //LSQ & LOAD_Q
			if (!it->first.compare("core_opcode_width")){  //Mc
			    sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			}
			else if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("core_store_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_store_buffer_size);
			}
			else if (!it->first.compare("core_load_buffer_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_load_buffer_size);
			}
			else if (!it->first.compare("core_memory_ports")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_memory_ports);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 21: //RAT
			if (!it->first.compare("archi_Regs_IRF_size")){  //Mc    
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if (!it->first.compare("core_phy_Regs_IRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			}
			else if (!it->first.compare("archi_Regs_FRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			}
			else if (!it->first.compare("core_phy_Regs_FRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			}
			else if (!it->first.compare("core_ROB_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_ROB_size);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("core_decode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_decode_width);
			}
			else if (!it->first.compare("core_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if (!it->first.compare("core_commit_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_commit_width);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 22: //ROB
			if (!it->first.compare("core_phy_Regs_FRF_size")){  //Mc  
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			}
			else if (!it->first.compare("core_phy_Regs_IRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			}
			else if (!it->first.compare("archi_Regs_IRF_size")){ 
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if (!it->first.compare("archi_Regs_FRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			}
			else if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("core_ROB_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_ROB_size);
			}
			else if (!it->first.compare("core_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 23: //BTB
			if (!it->first.compare("bpred_prediction_width")){  //Mc  
			    sscanf(it->second.c_str(), "%d", &bpred_tech.prediction_width);
			}
			else if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("btb_sC")){
			    sscanf(it->second.c_str(), "%lf", &btb_tech.unit_scap);
			}
			else if(!it->first.compare("btb_associativity")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.assoc);
			}  
			else if(!it->first.compare("btb_throughput")){  //Mc
				sscanf(it->second.c_str(), "%lf", &btb_tech.throughput);
			} 
			else if(!it->first.compare("btb_latency")){  //Mc
				sscanf(it->second.c_str(), "%lf", &btb_tech.latency);
			} 
			else if(!it->first.compare("btb_number_banks")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.num_banks);
			} 
			else if(!it->first.compare("btb_line_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &btb_tech.line_size);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			else if(!it->first.compare("btb_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &btb_tech.area);
			}
			else if(!it->first.compare("btb_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &btb_tech.num_transistors);
			}
			break;
		case 24:  //cache_l2 & l2dir		                        
                        if (!it->first.compare("cache_l2_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l2_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l2_tech.op_freq);
			}
			else if(!it->first.compare("cache_l2_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l2_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
                        else if (!it->first.compare("cache_l2_number_read_ports")){ 
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_rports);
			}
			else if (!it->first.compare("cache_l2_number_write_ports")){  
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_wports);
			}
			else if(!it->first.compare("cache_l2_number_readwrite_ports")){ 
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.num_rwports);
			}  			
			else if(!it->first.compare("cache_l2_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2_tech.device_type);
			}
			else if(!it->first.compare("cache_l2_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l2_tech.area);
			}
			else if(!it->first.compare("cache_l2_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l2_tech.num_transistors);
			}
			if (!it->first.compare("cache_l2dir_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l2dir_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%lf", &cache_l2dir_tech.op_freq);
			}
			else if(!it->first.compare("cache_l2dir_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l2dir_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2dir_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l2dir_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("cache_l2dir_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l2dir_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}   
			else if(!it->first.compare("cache_l2dir_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l2dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L2dir);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l2dir_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l2dir_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2dir_tech.device_type);
			}
			else if(!it->first.compare("cache_l2dir_directory_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l2dir_tech.directory_type);
			}
			else if(!it->first.compare("cache_l2dir_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l2dir_tech.area);
			}
			else if(!it->first.compare("cache_l2dir_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l2dir_tech.num_transistors);
			}
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
	        break;
		case 25: //MC
			if (!it->first.compare("mc_clock_rate")){  //Mc  
			    sscanf(it->second.c_str(), "%lf", &mc_tech.mc_clock);
			}
			else if (!it->first.compare("mc_llc_line_length")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.llc_line_length);
			}
			else if (!it->first.compare("mc_databus_width")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.databus_width);
			}
			else if (!it->first.compare("mc_addressbus_width")){
			    sscanf(it->second.c_str(), "%d", &mc_tech.addressbus_width);
			}
			else if(!it->first.compare("mc_req_window_size_per_channel")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.req_window_size_per_channel);
			}  
			else if(!it->first.compare("mc_memory_channels_per_mc")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_channels_per_mc);
			} 
			else if(!it->first.compare("mc_IO_buffer_size_per_channel")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.IO_buffer_size_per_channel);
			} 
			else if(!it->first.compare("mainmemory_peak_transfer_rate")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_peak_transfer_rate);
			} 
			else if(!it->first.compare("mainmemory_number_ranks")){  //Mc
				sscanf(it->second.c_str(), "%d", &mc_tech.memory_number_ranks);
			} 
			else if(!it->first.compare("mc_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &mc_tech.area);
			}
			else if(!it->first.compare("mc_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &mc_tech.num_transistors);
			}
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if (!it->first.compare("core_opcode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_opcode_width);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 26: //ROUTER
			if (!it->first.compare("router_clock_rate")){  //Mc  
			    sscanf(it->second.c_str(), "%lf", &router_tech.clockrate);
			}
			if (!it->first.compare("router_voltage")){  //Mc  
			    sscanf(it->second.c_str(), "%lf", &router_tech.vdd);
			}
			else if (!it->first.compare("router_has_global_link")){
			    sscanf(it->second.c_str(), "%d", &router_tech.has_global_link);
			}
			else if (!it->first.compare("router_link_length")){
			    sscanf(it->second.c_str(), "%lf", &router_tech.link_length);
			}
			else if (!it->first.compare("router_flit_bits")){
			    sscanf(it->second.c_str(), "%d", &router_tech.flit_bits);
			}
			else if (!it->first.compare("router_input_buffer_entries_per_vc")){
			    sscanf(it->second.c_str(), "%d", &router_tech.input_buffer_entries_per_vc);
			}
			else if(!it->first.compare("router_virtual_channel_per_port")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.virtual_channel_per_port);
			}  
			else if(!it->first.compare("router_input_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.input_ports);
			} 
			else if(!it->first.compare("router_output_ports")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.output_ports);
			} 
			else if(!it->first.compare("router_link_throughput")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.link_throughput);
			} 
			else if(!it->first.compare("router_link_latency")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.link_latency);
			} 
			else if(!it->first.compare("router_horizontal_nodes")){  //Mc
				sscanf(it->second.c_str(), "%d", &router_tech.horizontal_nodes);
			} 
			else if (!it->first.compare("router_vertical_nodes")){
			    sscanf(it->second.c_str(), "%d", &router_tech.vertical_nodes);
			}
			else if(!it->first.compare("router_topology")) {  //Mc
			    if (!it->second.compare("2DMESH"))
				router_tech.topology = TWODMESH;
			    else if (!it->second.compare("RING"))
				router_tech.topology = RING;
			    else if (!it->second.compare("CROSSBAR"))
				router_tech.topology = CROSSBAR;
			}
			else if(!it->first.compare("router_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &router_tech.area);
			}
			else if(!it->first.compare("router_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &router_tech.num_transistors);
			}
			else if(!it->first.compare("core_number_of_NoCs")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_number_of_NoCs);
			} 
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 28:  //RENAME_U	
			if (!it->first.compare("core_phy_Regs_FRF_size")){  //Mc  
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_FRF_size);
			}
			else if (!it->first.compare("core_phy_Regs_IRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_phy_Regs_IRF_size);
			}
			else if (!it->first.compare("archi_Regs_IRF_size")){ 
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_IRF_size);
			}
			else if (!it->first.compare("archi_Regs_FRF_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.archi_Regs_FRF_size);
			}			
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}			
			else if (!it->first.compare("core_ROB_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_ROB_size);
			}	
			else if (!it->first.compare("core_decode_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_decode_width);
			}
			else if (!it->first.compare("core_commit_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_commit_width);
			}
			else if (!it->first.compare("core_fp_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_fp_issue_width);
			}		
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 29:  //SCHEDULER_U	
			if (!it->first.compare("core_virtual_address_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_virtual_address_width);
			}
			else if (!it->first.compare("core_number_hardware_threads")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_number_hardware_threads);
			}
			else if (!it->first.compare("machine_bits")){
			    sscanf(it->second.c_str(), "%d", &core_tech.machine_bits);
			}
			else if (!it->first.compare("core_ROB_size")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_ROB_size);
			}
			else if (!it->first.compare("core_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_issue_width);
			}
			else if (!it->first.compare("core_fp_issue_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_fp_issue_width);
			}
			else if (!it->first.compare("core_commit_width")){
			    sscanf(it->second.c_str(), "%d", &core_tech.core_commit_width);
			}
			else if(!it->first.compare("core_instruction_window_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_instruction_window_size);
			}
			else if(!it->first.compare("core_fp_instruction_window_size")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_fp_instruction_window_size);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			} 
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}
			break;
		case 30:  //CACHE_L3	
			if (!it->first.compare("cache_l3_sC")){  //Mc
			        i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.unit_scap.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.unit_scap.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if (!it->first.compare("cache_l3_iC")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.unit_icap);
			}
			else if (!it->first.compare("cache_l3_eC")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.unit_ecap);
			}
			else if (!it->first.compare("supply_voltage")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.vss);
			}
                        else if (!it->first.compare("cache_l3_clockrate")){
			    sscanf(it->second.c_str(), "%lf", &cache_l3_tech.op_freq);
			}
                        else if (!it->first.compare("cache_l3_number_read_ports")){ //lv2
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_rports);
			}
			else if (!it->first.compare("cache_l3_number_write_ports")){  //lv2
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_wports);
			}
			else if(!it->first.compare("cache_l3_number_readwrite_ports")){  //lv2 
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_rwports);
			} 
			else if (!it->first.compare("cache_l3_number_sets")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_sets);
			}
			else if(!it->first.compare("cache_l3_line_size")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.line_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.line_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}
			else if(!it->first.compare("cache_l3_number_bitlines")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_bitlines);
			}
			else if(!it->first.compare("cache_l3_number_wordlines")){
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.num_wordlines);
			}
			else if(!it->first.compare("cache_l3_associativity")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.assoc.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.assoc.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l3_throughput")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.throughput.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.throughput.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_latency")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.latency.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.latency.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l3_output_width")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.output_width.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.output_width.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 	
			else if(!it->first.compare("cache_l3_cache_policy")){   //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.cache_policy.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.cache_policy.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 		
			else if(!it->first.compare("core_physical_address_width")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_physical_address_width);
			} 
			else if(!it->first.compare("cache_l3_miss_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.miss_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.miss_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_fill_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.fill_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.fill_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_prefetch_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.prefetch_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}   
			else if(!it->first.compare("cache_l3_number_banks")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.num_banks.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.num_banks.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			} 
			else if(!it->first.compare("cache_l3_wbb_buffer_size")){  //Mc
				i=0;
				for(n=0; n < it->second.length(); n++)
				{
				    if (it->second[n]!=',')
				    {
					sprintf(chtmp,"%c",it->second[n]);
					strcat(chtmp1,chtmp);
				    }
				    else{
					cache_l3_tech.wbb_buf_size.at(i) = atoi(chtmp1);
					assert((i+1) < device_tech.number_L3);
					i = i + 1;
					chtmp1[0]='\0';
				    }
				}
				cache_l3_tech.wbb_buf_size.at(i) = atoi(chtmp1);
				chtmp1[0]='\0';
			}  
			else if(!it->first.compare("cache_l3_device_type")){  //Mc
				sscanf(it->second.c_str(), "%d", &cache_l3_tech.device_type);
			}
			else if(!it->first.compare("cache_l3_area")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l3_tech.area);
			}
			else if(!it->first.compare("cache_l3_num_transistors")){  //intsim
				sscanf(it->second.c_str(), "%lf", &cache_l3_tech.num_transistors);
			}
			else if(!it->first.compare("core_temperature")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
			} 
			else if(!it->first.compare("core_tech_node")){  //Mc
				sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
			}  
			else if (!it->first.compare("core_clock_rate")){  //Mc
			    sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
			}	
			break;
		case 31:case 32:case 33:  //L1dir, L2dir UARCH
			
			break;	
	      } // end switch ptype
	   }//end power models other than McPAT
        }
	++it;
    } // end for all params
  } //end !readEntireXML


  if(power_model == McPAT || power_model == McPAT05)
	p_ifReadEntireXML = true;

  std::map<int,floorplan_t>::iterator fit;

  if (p_powerMonitor == true){
      //initialize tech params in the selected power model
      switch(power_model) 
      {
	case 0:
	/* McPAT*/
	  #ifdef McPAT07_H
	  //initialize all the McPAT params from McPAT xml; some tech params will be over written later by SST xml	    
          if(p_ifGetMcPATUnitP == false){
	     //ensure that the following will only be called once to reduce computational time
	     p_Mp1->parse(p_McPATxmlpath);
	     if (p_ifParser == false){
	         McPATSetup();	//Component is not a parser; re-setup McPAT    
	         p_Mproc.initialize(p_Mp1); //unit energy is computed by McPAT in this step
		 p_ifGetMcPATUnitP = true;
	         p_Mcore = p_Mproc.SSTreturnCore(0); //return core
	         ifu = p_Mcore->SSTreturnIFU();
	         lsu = p_Mcore->SSTreturnLSU();
	         mmu = p_Mcore->SSTreturnMMU();
	         exu = p_Mcore->SSTreturnEXU();
	         rnu = p_Mcore->SSTreturnRNU();
	     }
	     else{
		p_Mproc.initialize(p_Mp1); //final power values computed by McPAT in this step
		p_ifGetMcPATUnitP = true;
	     }
	     
	  }
	  if (p_ifParser == false){
	      getUnitPower(power_type, -1, McPAT); //read
	  }
	  #endif /*McPAT07_H*/   
                    
	break;
	case 1:
	/*TODO SimPanalyzer*/
	/*level 1-high level */
	if (p_powerLevel == 1) {
	#ifdef LV1_PANALYZER_H
          SSTsim_lv1_panalyzer_check_options(
	  cache_il1_tech.vss, // TODO is there a generic supply voltage? Or diff units have their own vss?
			    // If it's the latter case, panalyzer needs to be re-formated.
	  cache_il1_tech.op_freq,
	  alu_tech.unit_ecap, //alu_Ceff,
	  fpu_tech.unit_ecap,//fpu_Ceff,
	  mult_tech.unit_ecap,//mult_Ceff,
	  rf_tech.unit_ecap,//rf_Ceff,
	  bpred_tech.unit_ecap,//bpred_Ceff,
	  clock_tech.unit_ecap,//clock_Ceff,
	  //io_tech.vss, // 3.3 from s-p
	  //io_tech.i_o_style, // out from s-p
	  //io_tech.bus_width, // 8 from s-p
	  //io_tech.bus_size, // 64 from s-p
	  //6,//unsigned io_access_lat, 6 got from sim-panalyzer.c (lat to first chunk)
	  //2,//unsigned io_burst_lat, 2 got from sim-panalizer.c (lat between remaining chunks)
	  //io_tech.unit_icap, io_tech.unit_ecap, //io_Ceff,
	  cache_il1_tech.unit_icap, cache_il1_tech.unit_ecap,//il1_Ceff,
	  cache_il2_tech.unit_icap, cache_il2_tech.unit_ecap,//il2_Ceff,
	  cache_dl1_tech.unit_icap, cache_dl1_tech.unit_ecap,//dl1_Ceff,
	  cache_dl2_tech.unit_icap, cache_dl2_tech.unit_ecap,//dl2_Ceff,
	  cache_itlb_tech.unit_icap, cache_itlb_tech.unit_ecap,//itlb_Ceff,
	  cache_dtlb_tech.unit_icap, cache_dtlb_tech.unit_ecap);//dtlb_Ceff

	  /* lv1_io is handled by lv2 model */
	  #ifdef IO_PANALYZER_H
	  aio_pspec = create_io_panalyzer(
		      "aio", /* io name */
		      Analytical, /* io power model mode */
		      io_tech.op_freq, /* operating frequency in Hz */
		      io_tech.vss,
		      ((io_tech.i_o_style == OUT) ? odirBuffer: (io_tech.i_o_style == IN) ? idirBuffer : bidirBuffer),
		      /* io style */
		      io_tech.opt_io_buffer_num, /* optimial number of io buffer stages (5)*/
		      io_tech.ustrip_len, /* micro strip length (2)*/
		      io_tech.bus_width, /* io bus width */
		      io_tech.io_access_time, io_tech.io_cycle_time, /* io access/cycle time in cycles */
		      io_tech.bus_size, /* io size in bytes: sizeof(fu_address_t)=32*/
		      /* switching/internal/lekage  effective capacitances */
		      io_tech.unit_scap * 1E-12, 0, 0); /*(25)The latter two are "don't care" in Analytical*/
	  dio_pspec = create_io_panalyzer(
		      "aio", /* io name */
		      Analytical, /* io power model mode */
		      io_tech.op_freq, /* operating frequency in Hz */
		      io_tech.vss,
		      ((io_tech.i_o_style == OUT) ? odirBuffer: (io_tech.i_o_style == IN) ? idirBuffer : bidirBuffer),
		      /* io style */
		      io_tech.opt_io_buffer_num, /* optimial number of io buffer stages (5)*/
		      io_tech.ustrip_len, /* micro strip length (2)*/
		      io_tech.bus_width, /* io bus width */
		      io_tech.io_access_time, io_tech.io_cycle_time, /* io access/cycle time in cycles */
		      io_tech.bus_size, /* io size in bytes: sizeof(word_t)=32*/
		      /* switching/internal/lekage  effective capacitances */
		      io_tech.unit_scap * 1E-12, 0, 0); /*(25)The latter two are "don't care" in Analytical*/
          #endif /* IO_PANALYZER_H */

	  //get unit power right after setTech b/c panalyzer doesn't use objects in lv1 model
	  getUnitPower(power_type, 0, power_model); //read
	  getUnitPower(power_type, 1, power_model); //write
	#endif
	}
	else if (p_powerLevel == 2){
	/*level 2-low level (Analytical) */
	#ifdef LV2_PANALYZER_H
	    switch(power_type)
	    {			
		case 0:  // IL1
			 il1_pspec = create_cache_panalyzer(
				      "il1", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_il1_tech.op_freq, cache_il1_tech.vss, /* operating frequency/supply voltage */
				      cache_il1_tech.num_sets,  cache_il1_tech.line_size.at(0), cache_il1_tech.assoc.at(0),
				      cache_il1_tech.num_bitlines, cache_il1_tech.num_wordlines, 
				      cache_il1_tech.num_rwports, cache_il1_tech.num_rports, cache_il1_tech.num_wports, // 1, 0, 0
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_il1_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;
		case 1:  // IL2
			il2_pspec = create_cache_panalyzer(
				      "il2", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_il2_tech.op_freq, cache_il2_tech.vss, /* operating frequency/supply voltage */
				      cache_il2_tech.num_sets,  cache_il2_tech.line_size.at(0), cache_il2_tech.assoc.at(0),
				      cache_il2_tech.num_bitlines, cache_il2_tech.num_wordlines, 
				      cache_il2_tech.num_rwports, cache_il2_tech.num_rports, cache_il2_tech.num_wports,
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_il2_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;
		case 2:  // DL1
			dl1_pspec = create_cache_panalyzer(
				      "dl1", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_dl1_tech.op_freq, cache_dl1_tech.vss, /* operating frequency/supply voltage */
				      cache_dl1_tech.num_sets,  cache_dl1_tech.line_size.at(0), cache_dl1_tech.assoc.at(0),
				      cache_dl1_tech.num_bitlines, cache_dl1_tech.num_wordlines, 
				      cache_dl1_tech.num_rwports, cache_dl1_tech.num_rports, cache_dl1_tech.num_wports,
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_dl1_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;
		case 3:  // DL2
			dl2_pspec = create_cache_panalyzer(
				      "dl2", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_dl2_tech.op_freq, cache_dl2_tech.vss, /* operating frequency/supply voltage */
				      cache_dl2_tech.num_sets,  cache_dl2_tech.line_size.at(0), cache_dl2_tech.assoc.at(0),
				      cache_dl2_tech.num_bitlines, cache_dl2_tech.num_wordlines, 
				      cache_dl2_tech.num_rwports, cache_dl2_tech.num_rports, cache_dl2_tech.num_wports,
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_dl2_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;
		case 4:  //ITLB
			itlb_pspec = create_cache_panalyzer(
				      "itlb", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_itlb_tech.op_freq, cache_itlb_tech.vss, /* operating frequency/supply voltage */
				      cache_itlb_tech.num_sets,  cache_itlb_tech.line_size.at(0), cache_itlb_tech.assoc.at(0),
				      cache_itlb_tech.num_bitlines, cache_itlb_tech.num_wordlines, 
				      cache_itlb_tech.num_rwports, cache_itlb_tech.num_rports, cache_itlb_tech.num_wports,
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_itlb_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;
		case 5:  //DTLB
			dtlb_pspec = create_cache_panalyzer(
				      "dtlb", /* cache name */
				      Analytical, /* cache power model mode */
				      cache_dtlb_tech.op_freq, cache_dtlb_tech.vss, /* operating frequency/supply voltage */
				      cache_dtlb_tech.num_sets,  cache_dtlb_tech.line_size.at(0), cache_dtlb_tech.assoc.at(0),
				      cache_dtlb_tech.num_bitlines, cache_dtlb_tech.num_wordlines, 
				      cache_dtlb_tech.num_rwports, cache_dtlb_tech.num_rports, cache_dtlb_tech.num_wports,
				      /* switching/internal/lekage  effective capacitances of the array */
				      cache_dtlb_tech.unit_scap.at(0) * 1E-12, 0, 0); /*The latter two are "don't care" in Analytical*/
		break;		
		case 6:  //clock
			tdarea = estimateClockDieAreaSimPan();
			tcnodeCeff = estimateClockNodeCapSimPan();
			//std::cout << "total die area: " << tdarea << ", Ceff: " << tcnodeCeff << std::endl;
			clock_pspec = create_clock_panalyzer(
   		  			"clock", /* clock tree name */
	      				Analytical, /* clock power model mode */
		  			clock_tech.op_freq, /* operating frequency in Hz */
		  			clock_tech.vss, /* operating voltage in V */
		  			/* clock specific parameters */
		  			tdarea, /* tdarea : 2cm x 2cm total die area in um^2 */
		  			tcnodeCeff, /* tcnodeCeff : total clocked node capacitance in F */
		
		  			/* clock tree style */
		  			((clock_tech.clk_style == NORM_H) ? Htree : balHtree),
		  			clock_tech.skew * 1E-12, /* in ps */
		  			clock_tech.opt_clock_buffer_num, /* optimial number of io buffer stages */
					/* switching/internal/lekage  effective capacitances */
		 			0, 0, 0); /*These are "don't care" in Analytical*/
		break;
		case 7: //bpred-bimod, lev1, lev2, ras
			bpred_pspec = create_sbank_panalyzer(
			      "bpred", /* memory name */
			      Analytical, /* bpred power model mode */
			      bpred_tech.op_freq, bpred_tech.vss, /* operating frequency/supply voltage */
					  
			      bpred_tech.nrows /* nrows(2048)*/, bpred_tech.ncols/* ncols(2) */,	  
			      bpred_tech.num_rwports, bpred_tech.num_rports, bpred_tech.num_wports, 
			      //0, fetch_speed(1), ruu_commit_width(4),
			      /* switching/internal/lekage  effective capacitances */
			      bpred_tech.unit_scap * 1E-12, 0, 0);  /*The latter two are "don't care" in Analytical*/
		break;
		case 8: //rf-irf,fprf
			rf_pspec = create_sbank_panalyzer(
				      "rf", /* memory name */
				      Analytical, /* bpred power model mode */
				      rf_tech.op_freq, rf_tech.vss, /* operating frequency/supply voltage */
				      rf_tech.nrows /* nrows=MD_NUM_IREGS(32:machine.h)+ MD_NUM_PIREGS(64:S-P.c)*/,
				      rf_tech.ncols/* ncols=sizeof(md_gpr_t) * 8 / MD_NUM_IREGS */,
				      rf_tech.num_rwports, rf_tech.num_rports, rf_tech.num_wports,
				      //0, ruu_issue_width(4) * 2, ruu_commit_width(4),
				      /* switching/internal/lekage  effective capacitances */
				      rf_tech.unit_scap * 1E-12, 0, 0);  /*The latter two are "don't care" in Analytical*/
		break;
		case 9: //io-aio,dio
			aio_pspec = create_io_panalyzer(
			      "aio", /* io name */
			      Analytical, /* io power model mode */
			      io_tech.op_freq, /* operating frequency in Hz */
			      io_tech.vss,
			      ((io_tech.i_o_style == OUT) ? odirBuffer: (io_tech.i_o_style == IN) ? idirBuffer : bidirBuffer),
			      /* io style */
			      io_tech.opt_io_buffer_num, /* optimial number of io buffer stages (5)*/
			      io_tech.ustrip_len, /* micro strip length (2)*/
			      io_tech.bus_width, /* io bus width */
			      io_tech.io_access_time, io_tech.io_cycle_time, /* io access/cycle time in cycles (6/2)*/
			      io_tech.bus_size, /* io size in bytes: sizeof(fu_address_t)=32*/
			      /* switching/internal/lekage  effective capacitances */
			      io_tech.unit_scap * 1E-12, 0, 0); /*(25)The latter two are "don't care" in Analytical*/
			dio_pspec = create_io_panalyzer(
			      "dio", /* io name */
			      Analytical, /* io power model mode */
			      io_tech.op_freq, /* operating frequency in Hz */
			      io_tech.vss,
			      ((io_tech.i_o_style == OUT) ? odirBuffer: (io_tech.i_o_style == IN) ? idirBuffer : bidirBuffer),
			      /* io style */
			      io_tech.opt_io_buffer_num, /* optimial number of io buffer stages (5)*/
			      io_tech.ustrip_len, /* micro strip length (2)*/
			      io_tech.bus_width, /* io bus width */
			      io_tech.io_access_time, io_tech.io_cycle_time, /* io access/cycle time in cycles */
			      io_tech.bus_size, /* io size in bytes: sizeof(word_t)=32*/
			      /* switching/internal/lekage  effective capacitances */
			      io_tech.unit_scap * 1E-12, 0, 0); /*(25)The latter two are "don't care" in Analytical*/
		break;
		case 10: //logic
			logic_pspec = create_logic_panalyzer(
					"logic", /* memory name */
					Analytical, /* power model mode */
					/* memory operating parameters: operating frequency/supply voltage */
					logic_tech.op_freq, logic_tech.vss, 
					((logic_tech.lgc_style == STATIC) ? Static : Dynamic), /* logic style */
					logic_tech.num_gates, logic_tech.num_functions, //30000, 4
					logic_tech.num_fan_in, logic_tech.num_fan_out,  //10, 10
					/* x/y switching/internal/lekage  effective capacitances */
					0, 0, 0, 0);  /*These are "don't care" in Analytical*/
		break;
		case 11: //alu
			alu_pspec = create_alu_panalyzer("alu",(int) (alu_tech.op_freq), alu_tech.vss, alu_tech.unit_ecap*1E-12);
		break;
		case 12: //fpu
			fpu_pspec = create_fpu_panalyzer("fpu",(int) (fpu_tech.op_freq), fpu_tech.vss, fpu_tech.unit_ecap*1E-12);
		break;
		case 13: //mult
			mult_pspec = create_mult_panalyzer("mult",(int) (mult_tech.op_freq), mult_tech.vss, mult_tech.unit_ecap*1E-12);
		break;
		
		case 20: //uarch
		break;
		
		default:
			break; 	
	    }
	    //for lv2, unit power is calculated in xxx_pspec already;
	    //these step is to set up p_powerModel.xxx only.
	    getUnitPower(power_type, 0, power_model);
	    #endif //lv2
	} // end level =2		
	        
	break;
	case 2:
	/*McPAT05*/ 
	  #ifdef McPAT05_H
	  //initialize all the McPAT params from McPAT xml; some tech params will be over written later by SST xml	    
          p_Mp1->parse(p_McPATxmlpath);
	  McPAT05Setup();	    
	  p_Mproc.initialize(p_Mp1);
	  getUnitPower(power_type, 0, power_model); //read
	  #endif /*McPAT05_H*/                 
	break;
	case 3:
	/*IntSim*/
	   #ifdef INTSIM_H
	   switch(power_type)
      	   {
	      case CACHE_IL1:
		fit = p_chip.floorplan.find(floorplan_id.il1.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	        intsim_il1 = new IntSim_library((*fit).second.device_tech,cache_il1_tech.area, cache_il1_tech.num_transistors);
	      break;	
	      case CACHE_DL1:
		fit = p_chip.floorplan.find(floorplan_id.dl1.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_dl1 = new IntSim_library((*fit).second.device_tech,cache_dl1_tech.area, cache_dl1_tech.num_transistors);
	      break;
	      case CACHE_ITLB:
		fit = p_chip.floorplan.find(floorplan_id.itlb.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_itlb = new IntSim_library((*fit).second.device_tech,cache_itlb_tech.area, cache_itlb_tech.num_transistors);
	      break;
	      case CACHE_DTLB:
		fit = p_chip.floorplan.find(floorplan_id.dtlb.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_dtlb = new IntSim_library((*fit).second.device_tech,cache_dtlb_tech.area, cache_dtlb_tech.num_transistors);
	      break;
	      case CLOCK:
		fit = p_chip.floorplan.find(floorplan_id.clock.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_clock = new IntSim_library((*fit).second.device_tech,(*fit).second.feature.area, 0.25*(*fit).second.feature.area/pow((*fit).second.device_tech.feature_size*1e-9,2)*1e-2);
	      break;
	      case BPRED:
		fit = p_chip.floorplan.find(floorplan_id.bpred.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_bpred = new IntSim_library((*fit).second.device_tech,bpred_tech.area, bpred_tech.num_transistors);
	      break;
	      case RF:
		fit = p_chip.floorplan.find(floorplan_id.rf.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_rf = new IntSim_library((*fit).second.device_tech,rf_tech.area, rf_tech.num_transistors);
	      break;
	      case IO:
		fit = p_chip.floorplan.find(floorplan_id.io.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_io = new IntSim_library((*fit).second.device_tech,io_tech.area, io_tech.num_transistors);
	      break;
	      case LOGIC:
		fit = p_chip.floorplan.find(floorplan_id.logic.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_logic = new IntSim_library((*fit).second.device_tech,logic_tech.area, logic_tech.num_transistors);
	      break;
	      case EXEU_ALU:	    
		fit = p_chip.floorplan.find(floorplan_id.alu.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_alu = new IntSim_library((*fit).second.device_tech,alu_tech.area, alu_tech.num_transistors);
	      break;
	      case EXEU_FPU:
		fit = p_chip.floorplan.find(floorplan_id.fpu.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_fpu = new IntSim_library((*fit).second.device_tech,fpu_tech.area, fpu_tech.num_transistors);
	      break;
	      case MULT:
		fit = p_chip.floorplan.find(floorplan_id.mult.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_mult = new IntSim_library((*fit).second.device_tech,mult_tech.area, mult_tech.num_transistors);
	      break;
	      case 14: //IB
		fit = p_chip.floorplan.find(floorplan_id.ib.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
	    	intsim_ib = new IntSim_library((*fit).second.device_tech,ib_tech.area, ib_tech.num_transistors);
	      break;
	      case ISSUE_Q:
		fit = p_chip.floorplan.find(floorplan_id.issueQ.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_issueQ = new IntSim_library((*fit).second.device_tech,irs_tech.area, irs_tech.num_transistors);
	      break;
	      case INST_DECODER:
		fit = p_chip.floorplan.find(floorplan_id.decoder.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_decoder = new IntSim_library((*fit).second.device_tech,decoder_tech.area, decoder_tech.num_transistors);
	      break;
	      case BYPASS:
		fit = p_chip.floorplan.find(floorplan_id.bypass.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_bypass = new IntSim_library((*fit).second.device_tech,bypass_tech.area, bypass_tech.num_transistors);
	      break;
	      //case EXEU:
	      //break;
	      case PIPELINE:
		fit = p_chip.floorplan.find(floorplan_id.pipeline.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_pipeline = new IntSim_library((*fit).second.device_tech,pipeline_tech.area, pipeline_tech.num_transistors);
	      break;
	      //case 20:  //LSQ
	      //break;
	      //case RAT:
	      //break;
	      //case 22: //ROB	   
	      //break;
	      case 23:  //BTB
		fit = p_chip.floorplan.find(floorplan_id.btb.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_btb = new IntSim_library((*fit).second.device_tech,btb_tech.area, btb_tech.num_transistors);
	      break;
	      case CACHE_L2:
		fit = p_chip.floorplan.find(floorplan_id.L2.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_L2 = new IntSim_library((*fit).second.device_tech,cache_l2_tech.area, cache_l2_tech.num_transistors);
	      break;
	      case MEM_CTRL:
		fit = p_chip.floorplan.find(floorplan_id.mc.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_mc = new IntSim_library((*fit).second.device_tech,mc_tech.area, mc_tech.num_transistors);
	      break;
	      case ROUTER:
		fit = p_chip.floorplan.find(floorplan_id.router.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_router = new IntSim_library((*fit).second.device_tech,router_tech.area, router_tech.num_transistors);
	      break;
	      //case LOAD_Q:
	      //break;
	      //case RENAME_U:
	      //break;
	      //case SCHEDULER_U:
	      //break;
	      case CACHE_L3:
		fit = p_chip.floorplan.find(floorplan_id.L3.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_L3 = new IntSim_library((*fit).second.device_tech,cache_l3_tech.area, cache_l3_tech.num_transistors);
	      break;
	      case CACHE_L1DIR:
		fit = p_chip.floorplan.find(floorplan_id.L1dir.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_L1dir = new IntSim_library((*fit).second.device_tech,cache_l1dir_tech.area, cache_l1dir_tech.num_transistors);
	      break;
	      case CACHE_L2DIR:
		fit = p_chip.floorplan.find(floorplan_id.L2dir.at(0));
        	if(fit == p_chip.floorplan.end())
            	    cout << "ERROR: No matching floorplan is found" << endl;
		intsim_L2dir = new IntSim_library((*fit).second.device_tech,cache_l2dir_tech.area, cache_l2dir_tech.num_transistors);
	      break;
	      default: break;
	      
          } // end switch ptype
	  #endif //intsim_H
	  getUnitPower(power_type, 0, power_model); 
	break;

   case 4:
   /*ORION*/
      #ifdef ORION_H
      SST_SIM_router_init(&GLOB(router_info), &GLOB(router_power), NULL,
                          router_tech.input_ports,
                          router_tech.output_ports,
                          router_tech.flit_bits,
                          router_tech.virtual_channel_per_port);
      getUnitPower(power_type, 0, power_model);
      #endif //ORION_H
   break;
      } // end switch p_model
    }	//end model power = yes

}

/******************************************************
* Estimate power dissipation of a component per usage *
*******************************************************/
void Power::getUnitPower(ptype power_type, int user_data, pmodel power_model)
{
	
	unsigned int i=0;

	#ifdef ORION_H
	    char name[64] = "ORION";
    	    char *nameptr = name;
    	#endif /*ORION*/

	switch(power_type)
	{
	    case 0:
	    //cache_il1	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			icache = ifu->SSTreturnIcache();
			for(i = 0; i < device_tech.number_il1; i++){
			    p_areaMcPAT = p_areaMcPAT + icache.area.get_area();
			    #ifdef MESMTHI_H 
			        updateFloorplanAreaInfo(i /*floorplan id*/, icache.area.get_area());
			    #else
				updateFloorplanAreaInfo(floorplan_id.il1.at(i), icache.area.get_area());
			    #endif
			}
		    #endif                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		      #ifdef LV1_PANALYZER_H		    
		      lv1_panalyzer(lv1_il1,(fu_mcommand_t) user_data); //0:Read; 1:Write
		      if (user_data == 0) // unit read power
		        p_unitPower.il1_read = SSTlv1_panalyzerReadCurPower(lv1_il1);
		      else // unit write power
		        p_unitPower.il1_write = SSTlv1_panalyzerReadCurPower(lv1_il1);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)		      
  		          icache = p_Mproc.SSTInorderReturnICACHE();
		      else if (device_tech.machineType == 0)
			  icache = p_Mproc.SSToooReturnICACHE();
		      
		      p_areaMcPAT = p_areaMcPAT + icache.caches.local_result.area 
				+ icache.missb.local_result.area + icache.ifb.local_result.area + icache.prefetchb.local_result.area;	
   
		      #endif                        
		    break;
	            case 3:
		    /*IntSim*/ 
			#ifdef INTSIM_H
                      p_unitPower.il1_read = intsim_il1->chip->dyn_power_logic_gates+intsim_il1->chip->dyn_power_repeaters+intsim_il1->chip->power_wires;
		      p_unitPower.il1_write = p_unitPower.il1_read;	    
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.il1 = power_model;
	   break;
	   case 1:
	    //cache_il2	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    #ifdef LV1_PANALYZER_H	    
		    lv1_panalyzer(lv1_il2,(fu_mcommand_t) user_data); //0:Read; 1:Write
		    if (user_data == 0) // unit read power
		        p_unitPower.il2_read = SSTlv1_panalyzerReadCurPower(lv1_il2);
		    else // unit write power
		        p_unitPower.il2_write = SSTlv1_panalyzerReadCurPower(lv1_il2);
		    #endif
		    break;
                    case 2:
		    /*McPAT05*/
                    // catogorized in cache_L2 instead
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.il2_read = intsim_il2->chip->dyn_power_logic_gates+intsim_il2->chip->dyn_power_repeaters+intsim_il2->chip->power_wires;
		      p_unitPower.il2_write = p_unitPower.il2_read;		   		
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.il2 = power_model;
	   break;
	   case 2:
	    //cache_dl1	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			dcache = lsu->SSTreturnDcache();
			for(i = 0; i < device_tech.number_dl1; i++){
			    p_areaMcPAT = p_areaMcPAT + dcache.area.get_area();
			    #ifdef MESMTHI_H 
			        updateFloorplanAreaInfo(i /*floorplan id*/, dcache.area.get_area());
			    #else
				updateFloorplanAreaInfo(floorplan_id.dl1.at(i), dcache.area.get_area());
			    #endif
			}
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_dl1,(fu_mcommand_t) user_data); //0:Read; 1:Write
		      if (user_data == 0) // unit read power
		        p_unitPower.dl1_read = SSTlv1_panalyzerReadCurPower(lv1_dl1);
		      else // unit write power
		        p_unitPower.dl1_write = SSTlv1_panalyzerReadCurPower(lv1_dl1);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          dcache = p_Mproc.SSTInorderReturnDCACHE();
		      else if (device_tech.machineType == 0)
			  dcache = p_Mproc.SSToooReturnDCACHE();
		      
		      p_areaMcPAT = p_areaMcPAT + dcache.caches.local_result.area + dcache.wbb.local_result.area
				+ dcache.missb.local_result.area + dcache.ifb.local_result.area + dcache.prefetchb.local_result.area;
		      
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.dl1_read = intsim_dl1->chip->dyn_power_logic_gates+intsim_dl1->chip->dyn_power_repeaters+intsim_dl1->chip->power_wires;
		      p_unitPower.dl1_write = p_unitPower.dl1_read;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.dl1 = power_model;
	   break;
	   case 3:
	    //cache_dl2	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    #ifdef LV1_PANALYZER_H	    
		    lv1_panalyzer(lv1_dl2,(fu_mcommand_t) user_data); //0:Read; 1:Write
		    if (user_data == 0) // unit read power
		        p_unitPower.dl2_read = SSTlv1_panalyzerReadCurPower(lv1_dl2);
		    else // unit write power
		        p_unitPower.dl2_write = SSTlv1_panalyzerReadCurPower(lv1_dl2);
		    #endif
		    break;
                    case 2:
		    /*McPAT05*/
                    // catogorized in cache_L2 instead
		    break;
	            case 3:
		    /*IntSim*/
		 	#ifdef INTSIM_H
                         p_unitPower.dl2_read = intsim_dl2->chip->dyn_power_logic_gates+intsim_dl2->chip->dyn_power_repeaters+intsim_dl2->chip->power_wires;
		      p_unitPower.dl2_write = p_unitPower.dl2_read;		    		    
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.dl2 = power_model;
	   break;
	   case 4:
	    //cache_itlb	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			itlb = mmu->SSTreturnITLB();
			p_areaMcPAT = p_areaMcPAT + itlb->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.itlb.at(0), itlb->area.get_area());
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/
		      #ifdef LV1_PANALYZER_H		    
		      lv1_panalyzer(lv1_itlb,(fu_mcommand_t) user_data); //0:Read; 1:Write
		      if (user_data == 0) // unit read power
		        p_unitPower.itlb_read = SSTlv1_panalyzerReadCurPower(lv1_itlb);
		      else // unit write power
		        p_unitPower.itlb_write = SSTlv1_panalyzerReadCurPower(lv1_itlb);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          itlb = p_Mproc.SSTInorderReturnITLB();
		      else if (device_tech.machineType == 0)
			  itlb = p_Mproc.SSToooReturnITLB();                 
  		      p_areaMcPAT += itlb.tlb.local_result.area;
  		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                    	p_unitPower.itlb_read = intsim_itlb->chip->dyn_power_logic_gates+intsim_itlb->chip->dyn_power_repeaters+intsim_itlb->chip->power_wires;
		      p_unitPower.itlb_write = p_unitPower.itlb_read;	    		    
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.itlb = power_model;
	   break;
	   case 5:
	    //cache_dtlb	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			dtlb = mmu->SSTreturnDTLB();
			p_areaMcPAT = p_areaMcPAT + dtlb->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.dtlb.at(0), dtlb->area.get_area());
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_dtlb,(fu_mcommand_t) user_data); //0:Read; 1:Write
		      if (user_data == 0) // unit read power
		        p_unitPower.dtlb_read = SSTlv1_panalyzerReadCurPower(lv1_dtlb);
		      else // unit write power
		        p_unitPower.dtlb_write = SSTlv1_panalyzerReadCurPower(lv1_dtlb);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          dtlb = p_Mproc.SSTInorderReturnDTLB();
		      else if (device_tech.machineType == 0)
			  dtlb = p_Mproc.SSToooReturnDTLB();                       
  		      p_areaMcPAT += dtlb.tlb.local_result.area;
  		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.dtlb_read = intsim_dtlb->chip->dyn_power_logic_gates+intsim_dtlb->chip->dyn_power_repeaters+intsim_dtlb->chip->power_wires;
		      p_unitPower.dtlb_write = p_unitPower.dtlb_read;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.dtlb = power_model;
	   break;
	   case 6:
	    //clock	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_clock,(fu_mcommand_t) user_data); //0
 		      p_unitPower.clock = SSTlv1_panalyzerReadCurPower(lv1_clock);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		    //go to GetPower directly	
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                     	if(clock_tech.clock_option == GLOBAL_CLOCK)
        		    p_unitPower.clock = intsim_clock->chip->clock_power_dynamic*(1-intsim_clock->param->clock_gating_factor);
      			else if(clock_tech.clock_option == LOCAL_CLOCK)
        		    p_unitPower.clock = intsim_clock->chip->clock_power_dynamic*(intsim_clock->param->clock_gating_factor);
      			else
        		    p_unitPower.clock = intsim_clock->chip->clock_power_dynamic;	   
			std::cout << "unit clock power = " << p_unitPower.clock << std::endl;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.clock = power_model;
	   break;
	   case 7:
	    //bpred	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
		    if (bpred_tech.prediction_width > 0){
			BPT = ifu->SSTreturnBPT();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + BPT->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.bpred.at(i), BPT->area.get_area());
			}
		    }
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_bpred,(fu_mcommand_t) user_data); //0
     	              p_unitPower.bpred = SSTlv1_panalyzerReadCurPower(lv1_bpred);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
                      if(device_tech.machineType == 0){
			predictor =  p_Mproc.SSToooReturnPREDICTOR();
			p_areaMcPAT += predictor.gpredictor.local_result.area;
			p_areaMcPAT += predictor.lpredictor.local_result.area;
			p_areaMcPAT += predictor.chooser.local_result.area;
			p_areaMcPAT += predictor.ras.local_result.area*core_tech.core_number_hardware_threads;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.bpred = intsim_bpred->chip->dyn_power_logic_gates+intsim_bpred->chip->dyn_power_repeaters+intsim_bpred->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.bpred = power_model;
	   break;
	   case 8:
	    //rf	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			rfu = exu->SSTreturnRFU();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + rfu->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.rf.at(i), rfu->area.get_area());
			}
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/
		      #ifdef LV1_PANALYZER_H		    
		      lv1_panalyzer(lv1_rf,(fu_mcommand_t) user_data); //0
		      p_unitPower.rf = SSTlv1_panalyzerReadCurPower(lv1_rf);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1){
                          IRF = p_Mproc.SSTInorderReturnIRF();
		          FRF = p_Mproc.SSTInorderReturnFRF();
		          RFWIN = p_Mproc.SSTInorderReturnRFWIN();
		      }else if (device_tech.machineType == 0){
			  IRF = p_Mproc.SSToooReturnIRF();
		          FRF = p_Mproc.SSToooReturnFRF();
		          RFWIN = p_Mproc.SSToooReturnRFWIN(); 
			  phyIRF = p_Mproc.SSToooReturnPHYIRF();
			  phyFRF = p_Mproc.SSToooReturnPHYFRF();
			  p_areaMcPAT += phyFRF.RF.local_result.area;
			  p_areaMcPAT += phyIRF.RF.local_result.area;
		      }
  		      p_areaMcPAT += IRF.RF.local_result.area*core_tech.core_number_hardware_threads;
  		      p_areaMcPAT += FRF.RF.local_result.area*core_tech.core_number_hardware_threads;
		      if (core_tech.core_register_windows_size>0){		          
	  	          p_areaMcPAT += RFWIN.RF.local_result.area;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.rf = intsim_rf->chip->dyn_power_logic_gates+intsim_rf->chip->dyn_power_repeaters+intsim_rf->chip->power_wires;
			#endif
		    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.rf = power_model;
	   break;
	   case 9:
	    //io	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    /* Handled by lv2 and thus by GetPower*/
		    break;
                    case 2:
		    /*TODO McPAT05*/
                    
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                         p_unitPower.aio = intsim_io->chip->dyn_power_logic_gates+intsim_io->chip->dyn_power_repeaters+intsim_io->chip->power_wires;
			p_unitPower.dio = intsim_io->chip->dyn_power_logic_gates+intsim_io->chip->dyn_power_repeaters+intsim_io->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.io = power_model;
	   break;
	   case 10:
	    //logic	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer lv1 doesn't support logic power*/
		    break;
                    case 2:
		    /*McPAT05*/
			#ifdef McPAT05_H
			if (device_tech.machineType == 1){
                          //selection logic
  			  instruction_selection = p_Mproc.SSTInorderReturnINSTSELEC();
  			  //integer dcl  
  			  idcl = p_Mproc.SSTInorderReturnIDCL();
 			  //fp dcl
  			  fdcl = p_Mproc.SSTInorderReturnFDCL();
		        }else if (device_tech.machineType == 0){
			  //selection logic
  			  instruction_selection = p_Mproc.SSToooReturnINSTSELEC();
  			  //integer dcl  
  			  idcl = p_Mproc.SSToooReturnIDCL();
 			  //fp dcl
  			  fdcl = p_Mproc.SSToooReturnFDCL();
			}  
			#endif                      
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.logic = intsim_logic->chip->dyn_power_logic_gates+intsim_logic->chip->dyn_power_repeaters+intsim_logic->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.logic = power_model;
	   break;
	   case 11:
	    //alu	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			exeu = exu->SSTreturnEXEU();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + exeu->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.alu.at(i), exeu->area.get_area());
			}
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_alu,(fu_mcommand_t) user_data); //0
		      p_unitPower.alu = SSTlv1_panalyzerReadCurPower(lv1_alu);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      //double C_ALU; 		    
		      //C_ALU  = 0.05e-9;//F		   
		      p_unitPower.alu  = alu_tech.unit_scap*1E-12*g_tp.peri_global.Vdd*g_tp.peri_global.Vdd;  //g_tp is extern in McPAT parameter.h
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                         p_unitPower.alu = intsim_alu->chip->dyn_power_logic_gates+intsim_alu->chip->dyn_power_repeaters+intsim_alu->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.alu = power_model;
	   break;
	   case 12:
	    //fpu	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			fp_u = exu->SSTreturnFPU();
			p_areaMcPAT = p_areaMcPAT + fp_u->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.fpu.at(0), fp_u->area.get_area());
		    #endif
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		      #ifdef LV1_PANALYZER_H	    
		      lv1_panalyzer(lv1_fpu,(fu_mcommand_t) user_data); //0		  
		      p_unitPower.fpu = SSTlv1_panalyzerReadCurPower(lv1_fpu);
		      #endif
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      //double C_FPU; 		    
		      //C_FPU  = 0.35e-9; //F		   
		      p_unitPower.fpu  = fpu_tech.unit_scap* 1E-12*g_tp.peri_global.Vdd*g_tp.peri_global.Vdd;
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.fpu = intsim_fpu->chip->dyn_power_logic_gates+intsim_fpu->chip->dyn_power_repeaters+intsim_fpu->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.fpu = power_model;
	   break;
	   case 13:
	    //mult	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		      #ifdef LV1_PANALYZER_H		    
		      lv1_panalyzer(lv1_mult,(fu_mcommand_t) user_data); //0		   
		      p_unitPower.mult = SSTlv1_panalyzerReadCurPower(lv1_mult);
		      #endif
		    break;
                    case 2:
		    /*TODO McPAT05*/
                    
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.mult = intsim_mult->chip->dyn_power_logic_gates+intsim_mult->chip->dyn_power_repeaters+intsim_mult->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.mult = power_model;
	   break;
	   case 14:  
	   //ib	
		switch(power_model)
		{
		    case 0:
		     /* McPAT*/
		    #ifdef McPAT07_H
			IB = ifu->SSTreturnIB();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + IB->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.ib.at(i), IB->area.get_area());
			}
		    #endif   
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          IB = p_Mproc.SSTInorderReturnIB();
		      else if (device_tech.machineType == 0)
			  IB = p_Mproc.SSToooReturnIB();
  		      p_areaMcPAT += IB.IB.local_result.area;
  		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.ib = intsim_ib->chip->dyn_power_logic_gates+intsim_ib->chip->dyn_power_repeaters+intsim_ib->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.ib = power_model;
		break;
	   case 15:  
	  //issue_q	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          iRS = p_Mproc.SSTInorderReturnIRS();
		      else if (device_tech.machineType == 0){
			  iRS = p_Mproc.SSToooReturnIRS();
			  iISQ = p_Mproc.SSToooReturnIISQ();
			  fISQ = p_Mproc.SSToooReturnFISQ();
			  p_areaMcPAT += iISQ.RS.local_result.area;
			  p_areaMcPAT += fISQ.RS.local_result.area;
		      }		      
	  	      p_areaMcPAT += iRS.RS.local_result.area;
	  	      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.issueQ = intsim_issueQ->chip->dyn_power_logic_gates+intsim_issueQ->chip->dyn_power_repeaters+intsim_issueQ->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.issueQ = power_model;
		break;	
	   case 16:  
	  //inst decoder	
		switch(power_model)
		{
		    case 0:
		     /* McPAT*/
		    #ifdef McPAT07_H
			ID_inst = ifu->SSTreturnIDinst();
			ID_operand = ifu->SSTreturnIDoperand();
			ID_misc = ifu->SSTreturnIDmisc();
			for(i = 0; i < device_tech.number_dl1; i++){
			    p_areaMcPAT = p_areaMcPAT + (ID_inst->area.get_area() +
				ID_operand->area.get_area() +
				ID_misc->area.get_area())*core_tech.core_decode_width; //*10-6mm^2
			    updateFloorplanAreaInfo(floorplan_id.decoder.at(i), (ID_inst->area.get_area() +
				ID_operand->area.get_area() +
				ID_misc->area.get_area())*core_tech.core_decode_width);
			}
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
			#ifdef McPAT05_H
			if (device_tech.machineType == 1)
                          inst_decoder = p_Mproc.SSTInorderReturnDECODER();
		        else if (device_tech.machineType == 0)
			  inst_decoder = p_Mproc.SSToooReturnDECODER();                       
			#endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.decoder = intsim_decoder->chip->dyn_power_logic_gates+intsim_decoder->chip->dyn_power_repeaters+intsim_decoder->chip->power_wires;	   
			std::cout << "unit decoder power = " << p_unitPower.decoder << std::endl;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.decoder = power_model;
		break;	
	   case 17:  
	  //bypass	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			bypass = exu->SSTreturnBy();
			p_areaMcPAT = p_areaMcPAT + bypass.area.get_area();
			updateFloorplanAreaInfo(floorplan_id.bypass.at(0), bypass.area.get_area());
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
			#ifdef McPAT05_H
			p_areaMcPAT += LSQ.LSQ.local_result.area;
  			LSQ.area += LSQ.LSQ.local_result.area;

			if (device_tech.machineType == 1){
                          //int-broadcast 			
 			  int_bypass = p_Mproc.SSTInorderReturnINTBYPASS();
  			  //int_tag-broadcast 			
			  intTagBypass = p_Mproc.SSTInorderReturnINTTAGBYPASS();
  			  //fp-broadcast 			
                          fp_bypass = p_Mproc.SSTInorderReturnFPBYPASS();
		        }else if (device_tech.machineType == 0){
			  //int-broadcast 			
 			  int_bypass = p_Mproc.SSToooReturnINTBYPASS();
  			  //int_tag-broadcast 			
			  intTagBypass = p_Mproc.SSToooReturnINTTAGBYPASS();
  			  //fp-broadcast 			
                          fp_bypass = p_Mproc.SSToooReturnFPBYPASS();
			  fpTagBypass = p_Mproc.SSToooReturnFPTAGBYPASS(); 
			}
			#endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.bypass = intsim_bpred->chip->dyn_power_logic_gates+intsim_bpred->chip->dyn_power_repeaters+intsim_bpred->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.bypass = power_model;
		break;	
	   case 18:  
	  //exeu	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H		      	   
		      p_unitPower.exeu  = C_EXEU*1e-12*g_tp.peri_global.Vdd*g_tp.peri_global.Vdd; //pF->F                   
		      #endif		   
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.exeu = intsim_exeu->chip->dyn_power_logic_gates+intsim_exeu->chip->dyn_power_repeaters+intsim_exeu->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.exeu = power_model;
		break;
	   case 19:  
	  //pipeline	
		switch(power_model)
		{
		    case 0:
		    /*McPAT*/
		    corepipe = p_Mcore->SSTreturnPIPE();	
                    p_areaMcPAT = p_areaMcPAT + corepipe->area.get_area();
		    updateFloorplanAreaInfo(floorplan_id.pipeline.at(0), corepipe->area.get_area());
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1){
                          corepipe = p_Mproc.SSTInorderReturnPIPELINE();
		          undifferentiatedCore = p_Mproc.SSTInorderReturnUNCORE();
		      }else if (device_tech.machineType == 0){
			  corepipe = p_Mproc.SSToooReturnPIPELINE();
		          undifferentiatedCore = p_Mproc.SSToooReturnUNCORE(); 
                      }
                      p_areaMcPAT += undifferentiatedCore.areaPower.first;
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.pipeline = intsim_pipeline->chip->dyn_power_logic_gates+intsim_pipeline->chip->dyn_power_repeaters+intsim_pipeline->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.pipeline = power_model;
		break;
	   case 20:  
	   //lsq
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			LSQ = lsu->SSTreturnLSQ();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + LSQ->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.lsq.at(i), LSQ->area.get_area());
			}
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 1)
                          LSQ = p_Mproc.SSTInorderReturnLSQ();
		      else if (device_tech.machineType == 0){
			  LSQ = p_Mproc.SSToooReturnLSQ();
			  loadQ = p_Mproc.SSToooReturnLOADQ();
			  p_areaMcPAT += loadQ.LSQ.local_result.area;
		      }                     
                      p_areaMcPAT += LSQ.LSQ.local_result.area;
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.lsq = intsim_lsq->chip->dyn_power_logic_gates+intsim_lsq->chip->dyn_power_repeaters+intsim_lsq->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.lsq = power_model;
		break;	
	   case 21:  
	   //rat
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 0){	
                      	iRRAT = p_Mproc.SSToooReturnIRRAT();   
			fRRAT = p_Mproc.SSToooReturnFRRAT();
			iFRAT = p_Mproc.SSToooReturnIFRAT();
			fFRAT = p_Mproc.SSToooReturnFFRAT();
			iFRATCG = p_Mproc.SSToooReturnIFRATCG();
			fFRATCG = p_Mproc.SSToooReturnFFRATCG();
                        p_areaMcPAT = p_areaMcPAT + iFRAT.area + iFRATCG.area + iRRAT.area + fFRAT.area + fFRATCG.area + fRRAT.area;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.rat = intsim_rat->chip->dyn_power_logic_gates+intsim_rat->chip->dyn_power_repeaters+intsim_rat->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.rat = power_model;
		break;
	   case 22:  
	   //rob
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 0){
                        ROB = p_Mproc.SSToooReturnROB();
                        p_areaMcPAT += ROB.ROB.local_result.area;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.rob = intsim_rob->chip->dyn_power_logic_gates+intsim_rob->chip->dyn_power_repeaters+intsim_rob->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.rob = power_model;
		break;
	   case 23:  
	   //btb
		switch(power_model)
		{
		    case 0:
		     /* McPAT*/
		    #ifdef McPAT07_H
			if (bpred_tech.prediction_width > 0){
			    BTB = ifu->SSTreturnBTB();
			    for(i = 0; i < device_tech.number_core; i++){
			        p_areaMcPAT = p_areaMcPAT + BTB->area.get_area();
			        updateFloorplanAreaInfo(floorplan_id.btb.at(i), BTB->area.get_area());
			    }
			}
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      if (device_tech.machineType == 0){
                        BTB = p_Mproc.SSToooReturnBTB();
                        p_areaMcPAT += BTB.btb.local_result.area;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.btb = intsim_btb->chip->dyn_power_logic_gates+intsim_btb->chip->dyn_power_repeaters+intsim_btb->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.btb = power_model;
		break;
	   case 24:  
	   //L2
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			for(i = 0; i < device_tech.number_L2; i++){
			    l2array = p_Mproc.SSTreturnL2(i);
			    p_areaMcPAT = p_areaMcPAT + l2array->area.get_area();
			    #ifdef MESMTHI_H 
			        updateFloorplanAreaInfo(i /*floorplan id*/, l2array->area.get_area());
			    #else
				updateFloorplanAreaInfo(floorplan_id.L2.at(i), l2array->area.get_area());
			    #endif
			}
		    #endif                        
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
		      #ifdef McPAT05_H
		      for(i = 0; i < device_tech.number_L2; i++){
                        llCache = p_Mproc.SSTReturnL2CACHE(i);
			directory = p_Mproc.SSTReturnL2DIRECTORY(i);
			pipeLogicCache = p_Mproc.SSTReturnL2PIPELOGICCACHE(i);
			pipeLogicDirectory = p_Mproc.SSTReturnL2PIPELOGICDIRECTORY(i);
			L2clockNetwork = p_Mproc.SSTReturnL2CLOCKNETWORK(i);
                        p_areaMcPAT = p_areaMcPAT + llCache.caches.local_result.area + llCache.missb.local_result.area + llCache.ifb.local_result.area 
				     + llCache.prefetchb.local_result.area + llCache.wbb.local_result.area + directory.caches.local_result.area;
		      }
		      #endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.l2 = intsim_L2->chip->dyn_power_logic_gates+intsim_L2->chip->dyn_power_repeaters+intsim_L2->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.L2 = power_model;
		break;
	   case 25:  
	   //MC
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			mc = p_Mproc.SSTreturnMC();
			p_areaMcPAT = p_areaMcPAT + mc->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.mc.at(0), mc->area.get_area());
		    #endif                    
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/		
			#ifdef McPAT05_H      
                        frontendBuffer = p_Mproc.SSTReturnMCFRONTBUF();
			readBuffer = p_Mproc.SSTReturnMCREADBUF();
			writeBuffer = p_Mproc.SSTReturnMCWRITEBUF();
			MC_arb =  p_Mproc.SSTReturnMCARB();
			MCpipeLogic = p_Mproc.SSTReturnMCPIPE();
			MCclockNetwork = p_Mproc.SSTReturnMCCLOCKNETWORK();
			transecEngine = p_Mproc.SSTReturnMCBACKEND(); 
			PHY = p_Mproc.SSTReturnMCPHY();
                        p_areaMcPAT = p_areaMcPAT + frontendBuffer.area + readBuffer.area + writeBuffer.area + transecEngine.area + PHY.area;		      
			#endif
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.mc = intsim_mc->chip->dyn_power_logic_gates+intsim_mc->chip->dyn_power_repeaters+intsim_mc->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}	
		p_powerModel.mc = power_model;
		break;
	   case 26:  
	   //router
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			nocs = p_Mproc.SSTreturnNOC();
			p_areaMcPAT = p_areaMcPAT + nocs->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.router.at(0), nocs->area.get_area());
		    #endif                      
		    break;
		    case 1:
		    /*SimPanalyzer*/
		    break;
                    case 2:
		    /*McPAT05*/
			#ifdef McPAT05_H
			if  ((core_tech.core_number_of_NoCs  <=2) && (core_tech.core_number_of_NoCs  > 0))
  			{  //global		      
                            inputBuffer =  p_Mproc.SSTglobalReturnINPUTBUF();
			    routingTable = p_Mproc.SSTglobalReturnRTABLE();
        		    xbar = p_Mproc.SSTglobalReturnXBAR();
        		    vcAllocatorStage1 = p_Mproc.SSTglobalReturnVC1(); 
			    vcAllocatorStage2 = p_Mproc.SSTglobalReturnVC2();
 			    switchAllocatorStage1 = p_Mproc.SSTglobalReturnSWITCH1();
			    switchAllocatorStage2 =  p_Mproc.SSTglobalReturnSWITCH2();
        		    globalInterconnect =  p_Mproc.SSTglobalReturnINTERCONN();
       			    RTpipeLogic = p_Mproc.SSTglobalReturnRTPIPE();
        		    RTclockNetwork = p_Mproc.SSTglobalReturnRTCLOCK();
			}
			if  (core_tech.core_number_of_NoCs  ==2)
  			{  //local
			    inputBuffer =  p_Mproc.SSTlocalReturnINPUTBUF();
			    routingTable = p_Mproc.SSTlocalReturnRTABLE();
        		    xbar = p_Mproc.SSTlocalReturnXBAR();
        		    vcAllocatorStage1 = p_Mproc.SSTlocalReturnVC1(); 
			    vcAllocatorStage2 = p_Mproc.SSTlocalReturnVC2();
 			    switchAllocatorStage1 = p_Mproc.SSTlocalReturnSWITCH1();
			    switchAllocatorStage2 =  p_Mproc.SSTlocalReturnSWITCH2();
        		    globalInterconnect =  p_Mproc.SSTlocalReturnINTERCONN();
       			    RTpipeLogic = p_Mproc.SSTlocalReturnRTPIPE();
        		    RTclockNetwork = p_Mproc.SSTlocalReturnRTCLOCK();
			}
                        p_areaMcPAT = p_areaMcPAT + (inputBuffer.area + xbar.area.get_area()*1e-6
				+ globalInterconnect.area*router_tech.input_ports*(router_tech.horizontal_nodes-1+ router_tech.vertical_nodes-1)*1e-6)
				* router_tech.horizontal_nodes * router_tech.vertical_nodes;	
			#endif	      
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.router = intsim_router->chip->dyn_power_logic_gates+intsim_router->chip->dyn_power_repeaters+intsim_router->chip->power_wires;
			#endif
                    break;

          	    case 4:
          	    /*ORION*/
			#ifdef ORION_H 
			//p_unitPower.router = SST_SIM_router_stat_power(&GLOB(router_info), &GLOB(router_power), 0, nameptr, 0/*max_avg*/, 0.5/*avg # flits per port per cycle*/, 1/*if 1, print subcomponent results*/, router_tech.clockrate, router_tech.vdd);
	 		p_unitPower.router = SST_SIM_router_stat_energy(&GLOB(router_info), &GLOB(router_power), 0, nameptr, 0/*max_avg*/, 0.5/*avg # flits per port per cycle*/, 1/*if 1, print subcomponent results*/, router_tech.clockrate, router_tech.vdd);  //here unit power is actually unit energy
			#endif
          	    break;
		}	
		p_powerModel.router = power_model;
		break;
	   case 27:
	    //load_q	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			LoadQ = lsu->SSTreturnLoadQ();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + LoadQ->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.loadQ.at(i), LoadQ->area.get_area());
			}
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*IntSim*/  
			#ifdef INTSIM_H 
			p_unitPower.loadQ = intsim_loadQ->chip->dyn_power_logic_gates+intsim_loadQ->chip->dyn_power_repeaters+intsim_loadQ->chip->power_wires;                 
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.loadQ = power_model;
	   break;
	   case 28:
	    //rename_U	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + rnu->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.rename.at(i), rnu->area.get_area());
			}
		    #endif
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.rename = intsim_rename->chip->dyn_power_logic_gates+intsim_rename->chip->dyn_power_repeaters+intsim_rename->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
	   p_powerModel.rename = power_model;
	   break;
	   case 29:
	    //scheduler_U	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
			scheu = exu->SSTreturnSCHEU();
			for(i = 0; i < device_tech.number_core; i++){
			    p_areaMcPAT = p_areaMcPAT + scheu->area.get_area();
			    updateFloorplanAreaInfo(floorplan_id.scheduler.at(i), scheu->area.get_area());
			}
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/			   
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.scheduler = intsim_scheduler->chip->dyn_power_logic_gates+intsim_scheduler->chip->dyn_power_repeaters+intsim_scheduler->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.scheduler = power_model;
	   break;	
	   case 30:
	    //cache_L3	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
		      for(i = 0; i < device_tech.number_L3; i++){
			l3array = p_Mproc.SSTreturnL3(i);
			p_areaMcPAT = p_areaMcPAT + l3array->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.L3.at(i), l3array->area.get_area());
		      }
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/			   
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
			p_unitPower.l3 = intsim_L3->chip->dyn_power_logic_gates+intsim_L3->chip->dyn_power_repeaters+intsim_L3->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.L3 = power_model;
	   break;								
	   case 31:
	    //l1dir	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
		      for(i = 0; i < device_tech.number_L1dir; i++){
			l1dirarray = p_Mproc.SSTreturnL1dir(i);
			p_areaMcPAT = p_areaMcPAT + l1dirarray->area.get_area();
			#ifdef MESMTHI_H 
			        updateFloorplanAreaInfo(i /*floorplan id*/, l1dirarray->area.get_area());
			#else
				updateFloorplanAreaInfo(floorplan_id.L1dir.at(i), l1dirarray->area.get_area());
			#endif
		      }
		    #endif                      
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05--not modelled*/                   
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.l1dir = intsim_L1dir->chip->dyn_power_logic_gates+intsim_L1dir->chip->dyn_power_repeaters+intsim_L1dir->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.L1dir = power_model;
	   break;
	   case 32:
	    //l2dir	
		switch(power_model)
		{
		    case 0:
		    /* McPAT*/
		    #ifdef McPAT07_H
		      for(i = 0; i < device_tech.number_L2dir; i++){	
			l2dirarray = p_Mproc.SSTreturnL2dir(i);
			p_areaMcPAT = p_areaMcPAT + l2dirarray->area.get_area();
			updateFloorplanAreaInfo(floorplan_id.L2dir.at(i), l2dirarray->area.get_area());
		      }
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05--see case l2*/                   
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                        p_unitPower.l2dir = intsim_L2dir->chip->dyn_power_logic_gates+intsim_L2dir->chip->dyn_power_repeaters+intsim_L2dir->chip->power_wires;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
		p_powerModel.L2dir = power_model;
	   break;
	   case 33:
	    //uarch	
		switch(power_model)
		{
		    case 0:
		    /* TODO Wattch*/
                    
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    #ifdef LV1_PANALYZER_H	    
		    lv1_panalyzer(lv1_uarch,(fu_mcommand_t) user_data); //0		    
		    p_unitPower.uarch = SSTlv1_panalyzerReadCurPower(lv1_uarch);
		    #endif
		    break;
                    case 2:
		    /*TODO McPAT05*/                   
		    break;
	            case 3:
		    /*IntSim*/
			#ifdef INTSIM_H
                    p_unitPower.uarch = 9.99;
			#endif
                    break;

          case 4:
          /*ORION*/
          break;
		}
	   break;
	} // end switch ptype
	
}

/**************************************************************
* Estimate power dissipation of a component/sub-component.    *
* Registers/updates power statistics (itemized and ALL)       *
* locally by updatePowUsage.				      *
* It is component writer's responsibility to decide how often *
* to generate usage counts and call getPower.                 *
***************************************************************/
Pdissipation_t& Power::getPower(IntrospectedComponent* c, ptype power_type, usagecounts_t counts/*char *user_parms*/)
{
    I totalPowerUsage=0.0;
    I dynamicPower = 0.0;
    I leakage = 0.0;
    I TDP = 0.0;


    #ifdef McPAT05_H
    unsigned read_hits, read_misses, miss_buffer_access, fill_buffer_access, prefetch_buffer_access, wbb_buffer_access, write_access, total_hits, total_misses; //McPAT05 user_params
    unsigned archi_int_regfile_reads, archi_int_regfile_writes, archi_float_regfile_reads, archi_float_regfile_writes, function_calls; //McPAT05 user_params
    unsigned phy_int_regfile_reads, phy_int_regfile_writes, phy_float_regfile_reads, phy_float_regfile_writes; //McPAT05 user_params
    unsigned instruction_buffer_reads, instruction_buffer_writes, instruction_window_reads, instruction_window_writes;  //McPAT05 user_params
    unsigned fp_instructions, total_instructions, bypassbus_access, int_instructions, lsq_access, commited_instructions;  //McPAT05 user_params
    unsigned ROB_reads, ROB_writes, branch_instructions, branch_mispredictions; //McPAT05 user_params
    unsigned load_buffer_reads, load_buffer_writes, store_buffer_reads, store_buffer_writes; //McPAT05 user_params
    unsigned read_accesses, write_accesses, miss_buffer_accesses, fill_buffer_accesses, prefetch_buffer_reads, prefetch_buffer_writes; //McPAT05 user_params
    unsigned wbb_reads, wbb_writes, L2directory_read_accesses, L2directory_write_accesse, mc_memory_reads, mc_memory_writes, total_router_accesses; //McPAT05 user_params
    #endif /*McPAT05_H*/
    I executionTime = 1.0;
    SimTime_t current;
    unsigned int i=0;
    boost::mpi::communicator world;
    map<int,floorplan_t>::iterator fit;
 


  /*if (world.rank() == 0) {
    value = 1000.01;
  }
  else {
    value = 123.05;
  }
  if (world.rank() == 0){
	    reduce( world, value, maxvalue, std::plus<double>(), 0); 
    	    std::cout << "Power::The sum value is " << maxvalue << std::endl;
  } else {
    	    reduce(world, value, std::plus<double>(), 0);
  } */

   
    //decrement number of components need to getpower
    if (p_powerMonitor ==true && p_tempMonitor == true){
        if (world.size() == 1){
            p_NumCompNeedPower -= 1;
            assert (p_NumCompNeedPower >=0 );
	    //std::cout <<"My rank " << world.rank() << ", NumCompNeedPower = " << p_NumCompNeedPower << std::endl;
	}
	else { 
	    all_reduce( world, &p_TempSumNumCompNeedPower, 1, &p_SumNumCompNeedPower, std::minus<int>() ); 
            assert (p_SumNumCompNeedPower >=0 );
	    p_TempSumNumCompNeedPower = p_SumNumCompNeedPower;
	    //std::cout <<"My rank " << world.rank() << ", SumNumCompNeedPower = " << p_SumNumCompNeedPower << std::endl;
	}
    }


    if(p_powerMonitor == false){
	memset(&p_usage_uarch,0,sizeof(Pdissipation_t));
	return p_usage_uarch;
    }
    else{
        //first, get period since power was last queried (=current time - last time)
        executionTime = getExecutionTime(c);
	//and get current cycle for S-P
	current = c->getCurrentSimTime();

	switch(power_type)
	{
	  case 0:
	  //cache_il1	
	    for(i = 0; i < device_tech.number_il1; i++){
	    switch(p_powerModel.il1)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		ifu->SSTcomputeEnergy(false, counts.il1_read[i], counts.il1_readmiss[i], counts.IB_read[i], counts.IB_write[i], counts.BTB_read[i], counts.BTB_write[i]);
		icache = ifu->SSTreturnIcache();
		leakage = (I)icache.power.readOp.leakage + (I)icache.power.readOp.gate_leakage;
		////using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", il1 leakage = " << leakage << "readOP = " << icache.power.readOp.leakage << " gate = " << icache.power.readOp.gate_leakage << std::endl;
		dynamicPower = (I)icache.rt_power.readOp.dynamic / executionTime;
		////using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", icache.rt_power = " << icache.rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		ifu->SSTcomputeEnergy(true, counts.il1_read[i], counts.il1_readmiss[i], counts.IB_read[i], counts.IB_write[i], counts.BTB_read[i], counts.BTB_write[i]);
		icache = ifu->SSTreturnIcache();
		TDP = (I)icache.power.readOp.dynamic * (I)device_tech.clockRate;
		#endif
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		#ifdef PANALYZER_H	    		   
	        if (counts.il1_ReadorWrite == 0){//0:Read
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.il1_access * (I)p_unitPower.il1_read; 
		    #ifdef LV2_PANALYZER_H 
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(il1_pspec, Read, counts.il1_accessaddress/*address*/, NULL/*buffer(actual data block)*/, (tick_t)current, counts.il1_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.il1_access * (I)p_unitPower.il1_write;  //1:write
		    #ifdef LV2_PANALYZER_H
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(il1_pspec, Write, counts.il1_accessaddress, NULL, (tick_t) current, counts.il1_latency);
		    #endif
	        }
		#endif	
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d:%d:%d:%d", &read_hits, &read_misses, &miss_buffer_access, &fill_buffer_access, &prefetch_buffer_access, &wbb_buffer_access) != 6) {
    		    fprintf(stderr, "getPower: bad cache params: <read hits>:<read misses>:<miss buf access>:<fill buf access>:<prefetch buf access>:<wbb buf access>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = ((I)icache.caches.local_result.power.readOp.dynamic * (I)icache.caches.l_ip.num_rw_ports * (I)read_hits
			+ (I)icache.caches.local_result.power.writeOp.dynamic * (I)read_misses) / executionTime
		        + ((I)icache.missb.local_result.power.readOp.dynamic * (I)miss_buffer_access) / executionTime	
		        + ((I)icache.ifb.local_result.power.readOp.dynamic * (I)fill_buffer_access) / executionTime	
		        + ((I)icache.prefetchb.local_result.power.readOp.dynamic * (I)prefetch_buffer_access) / executionTime;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.il1_access * (I)p_unitPower.il1_read;
		leakage = (I)intsim_il1->chip->leakage_power_logic_gates + intsim_il1->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    #ifdef MESMTHI_H
		    updatePowUsage(c, power_type, i /*floorplan id*/, &p_usage_cache_il1[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #else
	    	    updatePowUsage(c, power_type, floorplan_id.il1.at(i), &p_usage_cache_il1[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #endif
	  }
	  break;

	  case 1:
	  //cache_il2
	    switch(p_powerModel.il2)
	    {
	      case 0:
	      /*McPAT*/                  
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		#ifdef PANALYZER_H        	
	        if (counts.il2_ReadorWrite == 0){ //0:Read
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.il2_access * (I)p_unitPower.il2_read;
		    #ifdef LV2_PANALYZER_H 
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(il2_pspec, Read, counts.il2_accessaddress, NULL, (tick_t) current, counts.il2_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.il2_access * (I)p_unitPower.il2_write;  //1:write	
		    #ifdef LV2_PANALYZER_H	
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(il2_pspec, Write, counts.il2_accessaddress, NULL, (tick_t) current, counts.il2_latency);
		    #endif
		}
                #endif
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.il2_access * (I)p_unitPower.il2_read;
		leakage = (I)intsim_il2->chip->leakage_power_logic_gates + intsim_il2->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.il2.at(0), &p_usage_cache_il2, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	   
	  case 2:
	  //cache_dl1
	    for(i = 0; i < device_tech.number_dl1; i++){
	    switch(p_powerModel.dl1)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		lsu->SSTcomputeEnergy(false, counts.dl1_read[i], counts.dl1_readmiss[i], counts.dl1_write[i], counts.dl1_writemiss[i], counts.LSQ_read[i], counts.LSQ_write[i]);
		dcache = lsu->SSTreturnDcache();
		leakage = (I)dcache.power.readOp.leakage + (I)dcache.power.readOp.gate_leakage;
		dynamicPower = (I)dcache.rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", dl1 leakage = " << leakage << "readOP = " << dcache.power.readOp.leakage << " gate = " << dcache.power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", dcache.rt_power = " << dcache.rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		lsu->SSTcomputeEnergy(true, counts.dl1_read[i], counts.dl1_readmiss[i], counts.dl1_write[i], counts.dl1_writemiss[i], counts.LSQ_read[i], counts.LSQ_write[i], counts.loadQ_read[i], counts.loadQ_write[i]);
		dcache = lsu->SSTreturnDcache();
		TDP = (I)dcache.power.readOp.dynamic * (I)device_tech.clockRate;
		#endif
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H	
	        if (counts.dl1_ReadorWrite == 0){//0:Read
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.dl1_access * (I)p_unitPower.dl1_read;  
		    #ifdef LV2_PANALYZER_H
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(dl1_pspec, Read, counts.dl1_accessaddress, NULL, (tick_t) current, counts.dl1_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.dl1_access * (I)p_unitPower.dl1_write;  //1:write		
		    #ifdef LV2_PANALYZER_H
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(dl1_pspec, Write, counts.dl1_accessaddress, NULL, (tick_t) current, counts.dl1_latency);
		    #endif
		}
		#endif
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d:%d:%d:%d", &read_hits, &read_misses, &miss_buffer_access, &fill_buffer_access, &prefetch_buffer_access, &write_access) != 6) {
    		    fprintf(stderr, "getPower: bad cache params: <read hits>:<read misses>:<miss buf access>:<fill buf access>:<prefetch buf access>:<write_access>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = ((I)dcache.caches.local_result.power.readOp.dynamic * (I)dcache.caches.l_ip.num_rw_ports * (I)read_hits
			+ (I)dcache.caches.local_result.power.writeOp.dynamic * (I)read_misses) / executionTime
		        + ((I)dcache.missb.local_result.power.readOp.dynamic * (I)miss_buffer_access) / executionTime	
		        + ((I)dcache.ifb.local_result.power.readOp.dynamic * (I)fill_buffer_access) / executionTime	
		        + ((I)dcache.prefetchb.local_result.power.readOp.dynamic * (I)prefetch_buffer_access) / executionTime
			+ ((I)dcache.wbb.local_result.power.readOp.dynamic * (I)write_access) / executionTime;
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.dl1_access * (I)p_unitPower.dl1_read;
		leakage = (I)intsim_dl1->chip->leakage_power_logic_gates + intsim_dl1->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	   
	    #ifdef MESMTHI_H
		    updatePowUsage(c, power_type, i /*floorplan id*/, &p_usage_cache_dl1[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #else
	    	    updatePowUsage(c, power_type, floorplan_id.dl1.at(i), &p_usage_cache_dl1[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #endif
	    }
	  break;		
	   
	  case 3:
	  //cache_dl2	
	    switch(p_powerModel.dl2)
	    {
	      case 0:
	      /*McPAT*/                 
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		#ifdef PANALYZER_H  
	        if (counts.dl2_ReadorWrite == 0){//0:Read
		    if (p_powerLevel == 1)
 		        totalPowerUsage = (I)counts.dl2_access * (I)p_unitPower.dl2_read; 
		    #ifdef LV2_PANALYZER_H 
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(dl2_pspec, Read, counts.dl2_accessaddress, NULL, (tick_t) current, counts.dl2_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.dl2_access * (I)p_unitPower.dl2_write;  //1:write
		    #ifdef LV2_PANALYZER_H		
		    else 
			totalPowerUsage = (I)SSTcache_panalyzer(dl2_pspec, Write, counts.dl2_accessaddress, NULL, (tick_t) current, counts.dl2_latency);	
		    #endif
		}
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.dl2_access * (I)p_unitPower.dl2_read;
		leakage = (I)intsim_dl2->chip->leakage_power_logic_gates + intsim_dl2->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.dl2.at(0), &p_usage_cache_dl2, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  
	  case 4:
	  //cache_itlb
	    for(i = 0; i < device_tech.number_itlb; i++){
	    switch(p_powerModel.itlb)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		mmu->SSTcomputeEnergy(false, counts.itlb_read[i], counts.itlb_readmiss[i], counts.dtlb_read[i], counts.dtlb_readmiss[i]);
		itlb = mmu->SSTreturnITLB();
		leakage = (I)itlb->power.readOp.leakage + (I)itlb->power.readOp.gate_leakage;
		dynamicPower = (I)itlb->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", itlb leakage = " << leakage << "readOP = " << itlb->power.readOp.leakage << " gate = " << itlb->power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", itlb->rt_power = " << itlb->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		mmu->SSTcomputeEnergy(true, counts.itlb_read[i], counts.itlb_readmiss[i], counts.dtlb_read[i], counts.dtlb_readmiss[i]);
		itlb = mmu->SSTreturnITLB();
		TDP = (I)itlb->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H   
	        if (counts.itlb_ReadorWrite == 0){//0:Read
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.itlb_access * (I)p_unitPower.itlb_read;  
		    #ifdef LV2_PANALYZER_H
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(itlb_pspec, Read, counts.itlb_accessaddress, NULL, (tick_t) current, counts.itlb_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.itlb_access * (I)p_unitPower.itlb_write;  //1:write
		    #ifdef LV2_PANALYZER_H		
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(itlb_pspec, Write, counts.itlb_accessaddress, NULL, (tick_t) current, counts.itlb_latency);	
		    #endif
		}
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &total_hits, &total_misses) != 2) {
    		    fprintf(stderr, "getPower: bad cache params: <total hits>:<total misses>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = ((I)itlb.tlb.local_result.power.readOp.dynamic * (I)total_hits
			+ (I)itlb.tlb.local_result.power.writeOp.dynamic * (I)total_misses) / executionTime;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.itlb_access * (I)p_unitPower.itlb_read;
		leakage = (I)intsim_itlb->chip->leakage_power_logic_gates + intsim_itlb->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.itlb.at(0), &p_usage_cache_itlb, totalPowerUsage, dynamicPower, leakage, TDP);
	    }
	  break;		
	   
	  case 5:
	  //cache_dtlb
	    for(i = 0; i < device_tech.number_dtlb; i++){
	    switch(p_powerModel.dtlb)
	    {
	      case 0:
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		mmu->SSTcomputeEnergy(false, counts.itlb_read[i], counts.itlb_readmiss[i], counts.dtlb_read[i], counts.dtlb_readmiss[i]);
		dtlb = mmu->SSTreturnDTLB();
		leakage = (I)dtlb->power.readOp.leakage + (I)dtlb->power.readOp.gate_leakage;
		dynamicPower = (I)dtlb->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", dtlb leakage = " << leakage << "readOP = " << dtlb->power.readOp.leakage << " gate = " << dtlb->power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", dtlb->rt_power = " << dtlb->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		mmu->SSTcomputeEnergy(true, counts.itlb_read[i], counts.itlb_readmiss[i], counts.dtlb_read[i], counts.dtlb_readmiss[i]);
		dtlb = mmu->SSTreturnDTLB();
		TDP = (I)dtlb->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H  
	        if (counts.dtlb_ReadorWrite == 0){//0:Read
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.dtlb_access * (I)p_unitPower.dtlb_read; 
		    #ifdef LV2_PANALYZER_H 
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(dtlb_pspec, Read, counts.dtlb_accessaddress, NULL, (tick_t) current, counts.dtlb_latency);
		    #endif
		}else{ // write
		    if (p_powerLevel == 1)
		        totalPowerUsage = (I)counts.dtlb_access * (I)p_unitPower.dtlb_write;  //1:write		
		    #ifdef LV2_PANALYZER_H
		    else
			totalPowerUsage = (I)SSTcache_panalyzer(dtlb_pspec, Write, counts.dtlb_accessaddress, NULL, (tick_t) current, counts.dtlb_latency);
		    #endif
		}
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &total_hits, &total_misses) != 2) {
    		    fprintf(stderr, "getPower: bad cache params: <total hits>:<total misses>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = ((I)dtlb.tlb.local_result.power.readOp.dynamic * (I)total_hits
			+ (I)dtlb.tlb.local_result.power.writeOp.dynamic * (I)total_misses) / executionTime;  
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.dtlb_access * (I)p_unitPower.dtlb_read;
		leakage = (I)intsim_dtlb->chip->leakage_power_logic_gates + intsim_dtlb->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.dtlb.at(0), &p_usage_cache_dtlb, totalPowerUsage, dynamicPower, leakage, TDP);
	    }
	  break;		

	  case 6:
	  //clock
	    switch(p_powerModel.clock)
	    {
	      case 0:
	        /*McPAT*/
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.clock_access * (I)p_unitPower.clock;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTclock_panalyzer(clock_pspec, (tick_t) current);
		#endif
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(device_tech.machineType == 1)
		    clockNetwork = p_Mproc.SSTInorderReturnCLOCK();
		else if (device_tech.machineType == 0)
		    clockNetwork = p_Mproc.SSToooReturnCLOCK();		
                totalPowerUsage = (I)clockNetwork.total_power.readOp.dynamic * (I)device_tech.clockRate;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.clock_access * (I)p_unitPower.clock;
		if(clock_tech.clock_option == GLOBAL_CLOCK)
        	    leakage = (I)intsim_clock->chip->clock_power_leakage*(1-intsim_clock->param->clock_gating_factor);     
      		else if(clock_tech.clock_option == LOCAL_CLOCK)     
        	    leakage = (I)intsim_clock->chip->clock_power_leakage*(intsim_clock->param->clock_gating_factor);      
      		else     
        	    leakage = (I)intsim_clock->chip->clock_power_leakage;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.clock.at(0), &p_usage_clock, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 7:
	  //bpred
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.bpred)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		if (bpred_tech.prediction_width > 0){
		  //executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		  BPT->SSTcomputeEnergy(false, counts.branch_read[i], counts.branch_write[i], counts.RAS_read[0], counts.RAS_write[0]);		
		  leakage = (I)BPT->power.readOp.leakage + (I)BPT->power.readOp.gate_leakage;
		  dynamicPower = (I)BPT->rt_power.readOp.dynamic / executionTime;
		  totalPowerUsage = leakage + dynamicPower;
		  BPT->SSTcomputeEnergy(true, counts.branch_read[i], counts.branch_write[i], counts.RAS_read[0], counts.RAS_write[0]);
		  TDP = (I)BPT->power.readOp.dynamic * (I)device_tech.clockRate;
		}
		#endif   
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.bpred_access * (I)p_unitPower.bpred;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTsbank_panalyzer(bpred_pspec, NULL /*bus = create_buffer_t(&target, xx_pspec->bsize)*/, (tick_t) current);
		#endif
                #endif
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &branch_instructions, &branch_mispredictions) != 2) {
    		    fprintf(stderr, "getPower: bad branch predictor params: <branch_instructions>:<branch_mispredictions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage =  (((I)predictor.gpredictor.local_result.power.readOp.dynamic + (I)predictor.lpredictor.local_result.power.readOp.dynamic
				+ (I)predictor.chooser.local_result.power.readOp.dynamic + (I)predictor.ras.local_result.power.readOp.dynamic
				+ (I)predictor.ras.local_result.power.writeOp.dynamic) * (I)branch_instructions
				+ ((I)predictor.gpredictor.local_result.power.writeOp.dynamic + (I)predictor.lpredictor.local_result.power.writeOp.dynamic
				+ (I)predictor.chooser.local_result.power.writeOp.dynamic + (I)predictor.ras.local_result.power.writeOp.dynamic) * (I)branch_mispredictions) / executionTime;  
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.bpred_access * (I)p_unitPower.bpred;
		leakage = (I)intsim_bpred->chip->leakage_power_logic_gates + intsim_bpred->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.bpred.at(i), &p_usage_bpred[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  
	  case 8:
	  //rf
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.rf)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		rfu->SSTcomputeEnergy(false, counts.int_regfile_reads[i], counts.int_regfile_writes[i], counts.float_regfile_reads[i], counts.float_regfile_writes[i], counts.RFWIN_read[0], counts.RFWIN_write[0]);		
		leakage = (I)rfu->power.readOp.leakage + (I)rfu->power.readOp.gate_leakage;
		dynamicPower = (I)rfu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		rfu->SSTcomputeEnergy(true, counts.int_regfile_reads[i], counts.int_regfile_writes[i], counts.float_regfile_reads[i], counts.float_regfile_writes[i], counts.RFWIN_read[0], counts.RFWIN_write[0]);
		TDP = (I)rfu->power.readOp.dynamic * (I)device_tech.clockRate;
		//using namespace io_interval;  std::cout << "CompID " << c->Id() << ", rf leakage = " << leakage << std::endl;
		#endif   
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.rf_access * (I)p_unitPower.rf;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTsbank_panalyzer(rf_pspec, NULL, (tick_t) current);
		#endif
                #endif
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d:%d:%d:%d:%d:%d:%d", &archi_int_regfile_reads, &archi_int_regfile_writes, &archi_float_regfile_reads, &archi_float_regfile_writes, &function_calls, &phy_int_regfile_reads, &phy_int_regfile_writes, &phy_float_regfile_reads, &phy_float_regfile_writes) != 9) {
    		    fprintf(stderr, "getPower: bad RF params: <int_regfile_reads>:<int_regfile_writes>:<float_regfile_reads>:<float_regfile_writes>:<function_calls>:<phy_int_regfile_reads>:<phy_int_regfile_writes>:<phy_float_regfile_reads>:<phy_float_regfile_writes>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = ((I)IRF.RF.local_result.power.readOp.dynamic * (I)archi_int_regfile_reads
			+ (I)IRF.RF.local_result.power.writeOp.dynamic * (I)archi_int_regfile_writes) / executionTime	
		        + ((I)FRF.RF.local_result.power.readOp.dynamic * (I)archi_float_regfile_reads
			+ (I)FRF.RF.local_result.power.writeOp.dynamic * (I)archi_float_regfile_writes) / executionTime;
		if (core_tech.core_register_windows_size > 0)
		    totalPowerUsage = totalPowerUsage + ((I)RFWIN.RF.local_result.power.readOp.dynamic + (I)RFWIN.RF.local_result.power.writeOp.dynamic)*12.0*2.0 * (I)function_calls;
		if(device_tech.machineType == 0){
		    totalPowerUsage = totalPowerUsage + ((I)phyIRF.RF.local_result.power.readOp.dynamic * (I)phy_int_regfile_reads
					                          + (I)phyIRF.RF.local_result.power.writeOp.dynamic * (I)phy_int_regfile_writes) / executionTime
				+ ((I)phyFRF.RF.local_result.power.readOp.dynamic * (I)phy_float_regfile_reads
					                          + (I)phyFRF.RF.local_result.power.writeOp.dynamic * (I)phy_float_regfile_writes) / executionTime;

		}
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.rf_access * (I)p_unitPower.rf;
		leakage = (I)intsim_rf->chip->leakage_power_logic_gates + intsim_rf->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.rf.at(i), &p_usage_rf[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 9:
	  //io
	    switch(p_powerModel.io)
	    {
	      case 0:
	      /* TODO Wattch*/
                  
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      #ifdef PANALYZER_H
	      #ifdef IO_PANALYZER_H 
		// io always handled by lv2
		if (counts.io_ReadorWrite == 0){//0:Read
		    totalPowerUsage = (I)SSTaio_panalyzer(aio_pspec, Read, counts.io_accessaddress/*address*/, NULL/*buffer*/, (tick_t) current, counts.io_latency/*lat*/) +
	  		         (I)SSTdio_panalyzer(dio_pspec, Read, counts.io_accessaddress, NULL, (tick_t) current, counts.io_latency);
		}else{  //write
		    totalPowerUsage = (I)SSTaio_panalyzer(aio_pspec, Write, counts.io_accessaddress, NULL, (tick_t) current, counts.io_latency) +
	  		         (I)SSTdio_panalyzer(dio_pspec, Write, counts.io_accessaddress, NULL, (tick_t) current, counts.io_latency);
		}
              #endif //IO_PANALYZER_H 
	      #endif 	
	      break;
	      case 2:
	      /*TODO McPAT05*/
                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.io_access * (I)p_unitPower.aio;
		leakage = (I)intsim_io->chip->leakage_power_logic_gates + intsim_io->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.io.at(0), &p_usage_io, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 10:
	  //logic
	    switch(p_powerModel.logic)
	    {
	      case 0:
	      /* TODO Wattch*/
                  
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.logic_access * (I)p_unitPower.logic;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTlogic_panalyzer(logic_pspec, (tick_t) current);
		#endif
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d", &total_instructions, &int_instructions, &fp_instructions) != 3) {
    		    fprintf(stderr, "getPower: bad logic params: <total_instructions>:<int_instructions>:<fp_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = (I)instruction_selection.power.readOp.dynamic * (I)total_instructions / executionTime
			    + (I)idcl.power.readOp.dynamic * (I)int_instructions / executionTime
			    + (I)fdcl.power.readOp.dynamic * (I)fp_instructions / executionTime;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.logic_access * (I)p_unitPower.logic;
		leakage = (I)intsim_logic->chip->leakage_power_logic_gates + intsim_logic->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.logic.at(0), &p_usage_logic, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 11:
	  //alu
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.alu)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		exu->SSTcomputeEnergy(false, counts.alu_access[i], counts.mult_access[0], counts.fpu_access[i]);
		leakage = (I)exeu->power.readOp.leakage + (I)exeu->power.readOp.gate_leakage;
		dynamicPower = (I)exeu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		exu->SSTcomputeEnergy(true, counts.alu_access[i], counts.mult_access[0], counts.fpu_access[i]);
		TDP = (I)exeu->power.readOp.dynamic * (I)device_tech.clockRate;
		/*using namespace io_interval; std::cout <<"ALU leakage =" << leakage <<", orig dynamic = " << exeu->rt_power.readOp.dynamic << ",dynamic= " << dynamicPower << ", total=" << totalPowerUsage << ", TDP= " << TDP << "executionTime= " << executionTime << std::endl;*/
		#endif      
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.alu_access[i] * (I)p_unitPower.alu;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTalu_panalyzer(alu_pspec, (tick_t) current);
		#endif
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d", &int_instructions) != 1) {
    		    fprintf(stderr, "getPower: bad alu params: <int_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = (I)p_unitPower.alu * (I)int_instructions / executionTime;   
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.alu_access[i] * (I)p_unitPower.alu;
		leakage = (I)intsim_alu->chip->leakage_power_logic_gates + intsim_alu->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.alu.at(i), &p_usage_alu[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  }  //end i for loop
	  break;
	  case 12:
	  //fpu
	    switch(p_powerModel.fpu)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		exu->SSTcomputeEnergy(false, counts.alu_access[0], counts.mult_access[0], counts.fpu_access[0]);
		leakage = (I)fp_u->power.readOp.leakage + (I)fp_u->power.readOp.gate_leakage;
		dynamicPower = (I)fp_u->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		exu->SSTcomputeEnergy(true, counts.alu_access[0], counts.mult_access[0], counts.fpu_access[0]);
		TDP = (I)fp_u->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif    
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.fpu_access[0] * (I)p_unitPower.fpu;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTfpu_panalyzer(fpu_pspec, (tick_t) current);
		#endif
                #endif	
	      break;
	      case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
                if(sscanf(user_parms, "%d", &fp_instructions) != 1) {
    		    fprintf(stderr, "getPower: bad fpu params: <fp_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = (I)p_unitPower.fpu * (I)fp_instructions / executionTime;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.fpu_access[0] * (I)p_unitPower.fpu;
		leakage = (I)intsim_fpu->chip->leakage_power_logic_gates + intsim_fpu->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, floorplan_id.fpu.at(0), &p_usage_fpu, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 13:
	  //mult
	    switch(p_powerModel.mult)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		exu->SSTcomputeEnergy(false, counts.alu_access[0], counts.mult_access[0], counts.fpu_access[0]);
		leakage = (I)mul->power.readOp.leakage + (I)mul->power.readOp.gate_leakage;
		dynamicPower = (I)mul->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		exu->SSTcomputeEnergy(true, counts.alu_access[0], counts.mult_access[0], counts.fpu_access[0]);
		TDP = (I)mul->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif    
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.mult_access[0] * (I)p_unitPower.mult;
		#ifdef LV2_PANALYZER_H
		else
		    totalPowerUsage = (I)SSTmult_panalyzer(mult_pspec, (tick_t) current);
		#endif
                #endif	
	      break;
	      case 2:
	      /*TODO McPAT05*/
                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.mult_access[0] * (I)p_unitPower.mult;
		leakage = (I)intsim_mult->chip->leakage_power_logic_gates + intsim_mult->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.mult.at(0), &p_usage_mult, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 14:  
	  //ib	
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.ib)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[i], counts.IB_write[i], counts.BTB_read[0], counts.BTB_write[0]);
		//IB = ifu->SSTreturnIcache();
		leakage = (I)IB->power.readOp.leakage + (I)IB->power.readOp.gate_leakage;
		dynamicPower = (I)IB->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		ifu->SSTcomputeEnergy(true, 1, 0, 4, 4, 2, 2);
		//icache = ifu->SSTreturnIcache();
		TDP = (I)IB->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &instruction_buffer_reads, &instruction_buffer_writes) != 2) {
    		    fprintf(stderr, "getPower: bad Instruction Buffer params: <instruction_buffer_reads>:<instruction_buffer_writes>");
    		    exit(1);
  		}
		executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = ((I)IB.IB.local_result.power.readOp.dynamic * (I)instruction_buffer_reads
			+ (I)IB.IB.local_result.power.writeOp.dynamic * (I)instruction_buffer_writes) / executionTime;
                #endif    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.ib_access * (I)p_unitPower.ib;
		leakage = (I)intsim_ib->chip->leakage_power_logic_gates + intsim_ib->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	    
	    updatePowUsage(c, power_type, floorplan_id.ib.at(i), &p_usage_ib[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop	
	  break;			
	  case 15:  
	  //issue_q	
	    switch(p_powerModel.issueQ)
	    {
	      case 0:
	      /* TODO Wattch*/
                    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &instruction_window_reads, &instruction_window_writes) != 2) {
    		    fprintf(stderr, "getPower: bad issue_q(inst issue queue) params: <instruction_window_reads>:<instruction_window_writes>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		if (device_tech.machineType == 0){
		    totalPowerUsage = ((I)iRS.RS.local_result.power.readOp.dynamic * (I)instruction_window_reads
			    + (I)iRS.RS.local_result.power.writeOp.dynamic * (I)instruction_window_writes) / executionTime
			    + ((I)iISQ.RS.local_result.power.readOp.dynamic * (I)instruction_window_reads
					+ (I)iISQ.RS.local_result.power.writeOp.dynamic * (I)instruction_window_writes) / executionTime
			    + ((I)fISQ.RS.local_result.power.readOp.dynamic * (I)instruction_window_reads
					+ (I)fISQ.RS.local_result.power.writeOp.dynamic * (I)instruction_window_writes) / executionTime; 
		}
		else if(device_tech.machineType == 1){
		    totalPowerUsage = ((I)iRS.RS.local_result.power.readOp.dynamic * (I)instruction_window_reads
			    + (I)iRS.RS.local_result.power.writeOp.dynamic * (I)instruction_window_writes) / executionTime;
		}
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.issueQ_access * (I)p_unitPower.issueQ;
		leakage = (I)intsim_issueQ->chip->leakage_power_logic_gates + intsim_issueQ->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.issueQ.at(0), &p_usage_rs, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;					
	  case 16:  
	  //decoder
	   for(i = 0; i < device_tech.number_core; i++){	
	    switch(p_powerModel.decoder)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[0], counts.IB_write[0], counts.BTB_read[0], counts.BTB_write[0], counts.ID_inst_read[i], counts.ID_operand_read[i], counts.ID_misc_read[i]);
		leakage = (I) ((bool)core_tech.core_long_channel? 
				(ID_inst->power.readOp.longer_channel_leakage +
				  ID_operand->power.readOp.longer_channel_leakage +
				  ID_misc->power.readOp.longer_channel_leakage):
				(ID_inst->power.readOp.leakage +
				  ID_operand->power.readOp.leakage +
				  ID_misc->power.readOp.leakage))  + 
			   (I)(ID_inst->power.readOp.gate_leakage +
				ID_operand->power.readOp.gate_leakage +
				ID_misc->power.readOp.gate_leakage) ;
		dynamicPower = (I)(ID_inst->rt_power.readOp.dynamic +
				ID_operand->rt_power.readOp.dynamic +
				ID_misc->rt_power.readOp.dynamic) / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		ifu->SSTcomputeEnergy(true, 1, 0, 4, 4, 2, 2, 2, 2, 2);
		TDP = (I)(ID_inst->power.readOp.dynamic +
			  ID_operand->power.readOp.dynamic +
			  ID_misc->power.readOp.dynamic) * (I)device_tech.clockRate;
		#endif    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d", &total_instructions) != 1) {
    		    fprintf(stderr, "getPower: bad inst decoder params: <total_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = (I)inst_decoder.total_power.readOp.dynamic * (I)total_instructions / executionTime;   
                #endif    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.decoder_access * (I)p_unitPower.decoder;
		leakage = (I)intsim_decoder->chip->leakage_power_logic_gates+intsim_decoder->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	    
	    updatePowUsage(c, power_type, floorplan_id.decoder.at(i), &p_usage_decoder[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end if ofr loop	
	  break;				
	  case 17:  
	  //bypass	
	    switch(p_powerModel.bypass)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		exu->SSTcomputeEnergy(false, counts.bypass_access);	
		leakage = (I)bypass.power.readOp.leakage + (I)bypass.power.readOp.gate_leakage;
		dynamicPower = (I)bypass.rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		exu->SSTcomputeEnergy(true, counts.bypass_access);
		TDP = (I)bypass.power.readOp.dynamic * (I)device_tech.clockRate;
		#endif     
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
                if(sscanf(user_parms, "%d:%d:%d", &bypassbus_access, &int_instructions, &fp_instructions) != 3) {
    		    fprintf(stderr, "getPower: bad bypass params: <bypassbus_access>:<int_instructions>:<fp_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		totalPowerUsage = (I)int_bypass.wires.power_link.readOp.dynamic * (I)bypassbus_access / executionTime
			    + (I)intTagBypass.wires.power_link.readOp.dynamic * (I)int_instructions / executionTime
			    + (I)fp_bypass.wires.power_link.readOp.dynamic * (I)bypassbus_access / executionTime;
		if (device_tech.machineType == 0){
		    totalPowerUsage = totalPowerUsage + (I)fpTagBypass.wires.power_link.readOp.dynamic * (I)fp_instructions / executionTime;
		}
		#endif	    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.bypass_access * (I)p_unitPower.bypass;
		leakage = (I)intsim_bypass->chip->leakage_power_logic_gates + intsim_bypass->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.bypass.at(0), &p_usage_bypass, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;		
	  case 18:  
	  //exeu	
	    switch(p_powerModel.exeu)
	    {
	      case 0:
	      /* TODO Wattch*/
                    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
                if(sscanf(user_parms, "%d", &int_instructions) != 1) {
    		    fprintf(stderr, "getPower: bad exeu params: <int_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = (I)p_unitPower.exeu * (I)int_instructions / executionTime;    
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.exeu_access * (I)p_unitPower.exeu;
		leakage = (I)intsim_exeu->chip->leakage_power_logic_gates + intsim_exeu->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	    
	    updatePowUsage(c, power_type, floorplan_id.exeu.at(0), &p_usage_exeu, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 19:  
	  //pipeline	
	    switch(p_powerModel.pipeline)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		leakage = (I)corepipe->power.readOp.leakage + (I)corepipe->power.readOp.gate_leakage;
		dynamicPower = (I)corepipe->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)corepipe->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif                     
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
                totalPowerUsage = (I)corepipe.power.readOp.dynamic * (I)device_tech.clockRate; 
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.pipeline_access * (I)p_unitPower.pipeline;
		leakage = (I)intsim_pipeline->chip->leakage_power_logic_gates + intsim_pipeline->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	
	    updatePowUsage(c, power_type, floorplan_id.pipeline.at(0), &p_usage_pipeline, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  case 20:  
	  //lsq	
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.lsq)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		lsu->SSTcomputeEnergy(false, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[i], counts.LSQ_write[i], counts.loadQ_read[0], counts.loadQ_write[0]);
		
		leakage = (I)LSQ->power.readOp.leakage + (I)LSQ->power.readOp.gate_leakage;
		dynamicPower = (I)LSQ->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		lsu->SSTcomputeEnergy(true, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[i], counts.LSQ_write[i], counts.loadQ_read[0], counts.loadQ_write[0]);
		
		TDP = (I)LSQ->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif                    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(device_tech.machineType == 1){ 
		    if(sscanf(user_parms, "%d", &lsq_access) != 1) {
    		        fprintf(stderr, "getPower: bad lsq params: <lsq_access>");
    		        exit(1);
  		    }
                    totalPowerUsage = ((I)LSQ.LSQ.l_ip.num_rd_ports * (I)LSQ.LSQ.local_result.power.readOp.dynamic
			        + (I)LSQ.LSQ.l_ip.num_wr_ports * (I)LSQ.LSQ.local_result.power.writeOp.dynamic) * (I)device_tech.clockRate * (I)lsq_access;
		}
		else if (device_tech.machineType == 0){
		    if(sscanf(user_parms, "%d:%d:%d:%d", &load_buffer_reads, &load_buffer_writes, &store_buffer_reads, &store_buffer_writes) != 4) {
    		        fprintf(stderr, "getPower: bad lsq params: <load_buffer_reads>:<load_buffer_writes>:<store_buffer_reads>:<store_buffer_writes>");
    		        exit(1);
  		    }
		    //executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                    totalPowerUsage = ((I)loadQ.LSQ.local_result.power.readOp.dynamic * (I)load_buffer_reads
				  + (I)loadQ.LSQ.local_result.power.writeOp.dynamic * (I)load_buffer_writes) / executionTime
				  + ((I)LSQ.LSQ.local_result.power.readOp.dynamic * (I)store_buffer_reads
				  + (I)LSQ.LSQ.local_result.power.writeOp.dynamic * (I)store_buffer_writes) / executionTime;
		}
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.lsq_access * (I)p_unitPower.lsq;
		leakage = (I)intsim_lsq->chip->leakage_power_logic_gates + intsim_lsq->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.lsq.at(i), &p_usage_lsq[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 21:  
	  //rat
	    switch(p_powerModel.rat)
	    {
	      case 0:
	      /* TODO Wattch*/                    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d:%d:%d", &int_instructions, &branch_mispredictions, &branch_instructions, &commited_instructions, &fp_instructions) != 5) {
    		    fprintf(stderr, "getPower: bad RAT params: <int_instructions>,<branch_mispredictions>,<branch_instructions>,<commited_instructions>,<fp_instructions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = ((I)iFRAT.rat.local_result.power.readOp.dynamic * (I)int_instructions * 2.0
					                        + (I)iFRAT.rat.local_result.power.writeOp.dynamic * (I)int_instructions) / executionTime
			     + ((I)iFRATCG.rat.local_result.power.readOp.dynamic * (I)branch_mispredictions * 32.0
					                        + (I)iFRATCG.rat.local_result.power.writeOp.dynamic * (I)branch_instructions * 32.0)/executionTime
			     + ((I)iRRAT.rat.local_result.power.readOp.dynamic * (I)branch_mispredictions
					                        + (I)iRRAT.rat.local_result.power.writeOp.dynamic * (I)commited_instructions) / executionTime
			     + ((I)fFRAT.rat.local_result.power.readOp.dynamic * (I)fp_instructions * 2.0
					                        + (I)fFRAT.rat.local_result.power.writeOp.dynamic * (I)fp_instructions) / executionTime
			     + ((I)fFRATCG.rat.local_result.power.readOp.dynamic * (I)branch_mispredictions * 32.0
					+ (I)fFRATCG.rat.l_ip.num_wr_ports * (I)fFRATCG.rat.local_result.power.writeOp.dynamic * (I)branch_instructions * 32.0) / executionTime
			     + ((I)fRRAT.rat.local_result.power.readOp.dynamic * (I)branch_mispredictions
					                        + (I)fRRAT.rat.local_result.power.writeOp.dynamic * (I)commited_instructions) / executionTime; 
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.rat_access * (I)p_unitPower.rat;
		leakage = (I)intsim_rat->chip->leakage_power_logic_gates + intsim_rat->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.rat.at(0), &p_usage_rat, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  case 22:  
	  //rob
	    switch(p_powerModel.rob)
	    {
	      case 0:
	      /* TODO Wattch*/                    
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &ROB_reads, &ROB_writes) != 2) {
    		    fprintf(stderr, "getPower: bad rob params: <ROB_reads>:<ROB_writes>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = ((I)ROB.ROB.local_result.power.readOp.dynamic * (I)ROB_reads
					                     + (I)ROB.ROB.local_result.power.writeOp.dynamic * (I)ROB_writes) / executionTime; 
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.rob_access * (I)p_unitPower.rob;
		leakage = (I)intsim_rob->chip->leakage_power_logic_gates + intsim_rob->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	    
	    updatePowUsage(c, power_type, floorplan_id.rob.at(0), &p_usage_rob, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  case 23:  
	  //btb	
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.btb)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		if (bpred_tech.prediction_width > 0){
		  //executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		  ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[0], counts.IB_write[0], counts.BTB_read[i], counts.BTB_write[i]);		
		  leakage = (I)BTB->power.readOp.leakage + (I)BTB->power.readOp.gate_leakage;
		  dynamicPower = (I)BTB->rt_power.readOp.dynamic / executionTime;
		  totalPowerUsage = leakage + dynamicPower;
		  TDP = (I)BTB->power.readOp.dynamic * (I)device_tech.clockRate;
		}
		#endif                     
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &branch_instructions, &branch_mispredictions) != 2) {
    		    fprintf(stderr, "getPower: bad btb params: <branch_instructions>:<branch_mispredictions>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = ((I)BTB.btb.local_result.power.readOp.dynamic * (I)branch_instructions
				                      + (I)BTB.btb.local_result.power.writeOp.dynamic * (I)branch_mispredictions) / executionTime;
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.btb_access * (I)p_unitPower.btb;
		leakage = (I)intsim_btb->chip->leakage_power_logic_gates + intsim_btb->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.btb.at(i), &p_usage_btb[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 24:  
	  //L2	
	    for(i = 0; i < device_tech.number_L2; i++){
	    switch(p_powerModel.L2)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		l2array = p_Mproc.SSTreturnL2(i);
		l2array->SSTcomputeEnergy(false, counts.L2_read[i], counts.L2_readmiss[i], counts.L2_write[i], counts.L2_writemiss[i], counts.L3_read[i], counts.L3_readmiss[i], 
			counts.L3_write[i], counts.L3_writemiss[i], counts.L1Dir_read[i], counts.L1Dir_readmiss[i], counts.L1Dir_write[i], counts.L1Dir_writemiss[i], 
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i], counts.homeL2_read[i], counts.homeL2_readmiss[i], counts.homeL2_write[i], counts.homeL2_writemiss[i], counts.homeL3_read[i], counts.homeL3_readmiss[i], 
			counts.homeL3_write[i], counts.homeL3_writemiss[i]);
		leakage = (I)l2array->power.readOp.leakage + (I)l2array->power.readOp.gate_leakage;
		dynamicPower = (I)l2array->rt_power.readOp.dynamic / executionTime;
		////std::cout << "L2 id " << i << " on floorplan " << floorplan_id.L2.at(i) << ":" << std::endl;
		////using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", l2 leakage = " << leakage << "readOP = " << l2array->power.readOp.leakage << " gate = " << l2array->power.readOp.gate_leakage << std::endl;
		////using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", l2array->rt_power = " << l2array->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)l2array->power.readOp.dynamic * (I)cache_l2_tech.op_freq; //(cache_clockrate)
		#endif                        
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", &read_accesses, &write_accesses, &miss_buffer_accesses, &fill_buffer_accesses, &prefetch_buffer_reads, &prefetch_buffer_writes, &wbb_reads, &wbb_writes, &L2directory_read_accesses, &L2directory_write_accesse) != 10) {
    		    fprintf(stderr, "getPower: bad L2 params: <read_accesses>:<write_accesses>:<miss_buffer_accesses>:<fill_buffer_accesses>:<prefetch_buffer_reads>:<prefetch_buffer_writes>:<wbb_reads>:<wbb_writes>:<L2directory_read_accesses>:<L2directory_write_accesse>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)cache_l2_tech.op_freq * (I)total_cycles;
                totalPowerUsage = ((I)llCache.caches.local_result.power.readOp.dynamic * (I)read_accesses + (I)llCache.caches.local_result.power.writeOp.dynamic * (I)write_accesses) / executionTime + ((I)llCache.missb.local_result.power.readOp.dynamic * (I)miss_buffer_accesses + (I)llCache.missb.local_result.power.writeOp.dynamic * (I)miss_buffer_accesses) / executionTime + ((I)llCache.ifb.local_result.power.readOp.dynamic * (I)fill_buffer_accesses + (I)llCache.ifb.local_result.power.writeOp.dynamic * (I)fill_buffer_accesses) / executionTime 
+ ((I)llCache.prefetchb.local_result.power.readOp.dynamic * (I)prefetch_buffer_reads + (I)llCache.prefetchb.local_result.power.writeOp.dynamic * (I)prefetch_buffer_writes) /	executionTime + ((I)llCache.wbb.local_result.power.readOp.dynamic * (I)wbb_reads + (I)llCache.wbb.local_result.power.writeOp.dynamic * (I)wbb_writes) / executionTime + ((I)directory.caches.local_result.power.readOp.dynamic * (I)L2directory_read_accesses + (I)directory.caches.local_result.power.writeOp.dynamic * (I)L2directory_write_accesse) / executionTime + (I)pipeLogicCache.power.readOp.dynamic * (I)cache_l2_tech.op_freq + (I)pipeLogicDirectory.power.readOp.dynamic * (I)cache_l2_tech.op_freq + (I)L2clockNetwork.total_power.readOp.dynamic * (I)cache_l2_tech.op_freq;
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.l2_access * (I)p_unitPower.l2;
		leakage = (I)intsim_L2->chip->leakage_power_logic_gates + intsim_L2->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }	
	    #ifdef MESMTHI_H
		    updatePowUsage(c, power_type, i /*floorplan id*/, &p_usage_cache_l2[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #else
	    	    updatePowUsage(c, power_type, floorplan_id.L2.at(i), &p_usage_cache_l2[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #endif
	    }
	  break;
	  case 25:  
	  //MC	
	    switch(p_powerModel.mc)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		mc->SSTcomputeEnergy(false, counts.memctrl_read, counts.memctrl_write);
		leakage = (I)mc->power.readOp.leakage + (I)mc->power.readOp.gate_leakage;
		dynamicPower = (I)mc->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)mc->power.readOp.dynamic * (I)mc_tech.mc_clock * (I)2.0;  //*2 is from McPAT memoryctrl.cc set_param() 
		#endif                           
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d:%d", &mc_memory_reads, &mc_memory_writes) != 2) {
    		    fprintf(stderr, "getPower: bad MEM_CTRL params: <mc_memory_reads>:<mc_memory_writes>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)mc_tech.mc_clock * (I)total_cycles;
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
                totalPowerUsage = ((I)frontendBuffer.caches.local_result.power.readOp.dynamic + (I)frontendBuffer.caches.local_result.power.writeOp.dynamic) * ((I)mc_memory_reads + (I)mc_memory_writes) / executionTime + ((I)readBuffer.caches.local_result.power.readOp.dynamic+ (I)readBuffer.caches.local_result.power.writeOp.dynamic) * ((I)mc_memory_reads + (I)mc_memory_writes) * (I)mc_tech.llc_line_length * 8.0 / (I)mc_tech.databus_width / executionTime + ((I)writeBuffer.caches.local_result.power.readOp.dynamic + (I)writeBuffer.caches.local_result.power.writeOp.dynamic) * ((I)mc_memory_reads + (I)mc_memory_writes) * (I)mc_tech.llc_line_length * 8.0 / (I)mc_tech.databus_width / executionTime + (I)MC_arb.power.readOp.dynamic * ((I)mc_memory_reads + (I)mc_memory_writes) / executionTime + (I)transecEngine.power.readOp.dynamic * ((I)mc_memory_reads + (I)mc_memory_writes) / executionTime + (I)PHY.power.readOp.dynamic * (((I)mc_memory_reads + (I)mc_memory_writes) * ((I)mc_tech.llc_line_length * 8.0 + (I)core_tech.core_physical_address_width * 2.0) / executionTime) * 1e-9 + (I)MCpipeLogic.power.readOp.dynamic * (I)mc_tech.memory_channels_per_mc * (I)mc_tech.mc_clock + (I)MCclockNetwork.power_link.readOp.dynamic * (I)mc_tech.memory_channels_per_mc * (I)mc_tech.mc_clock;
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.mc_access * (I)p_unitPower.mc;
		leakage = (I)intsim_mc->chip->leakage_power_logic_gates + intsim_mc->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    }		    
	    updatePowUsage(c, power_type, floorplan_id.mc.at(0), &p_usage_mc, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 26:  
	  //router	
	    switch(p_powerModel.router)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		nocs->SSTcomputeEnergy(false, counts.router_access);
		if (p_hasUpdatedTemp == true){
		    fit = p_chip.floorplan.find(floorplan_id.router.at(0));
		    leakage_feedback(p_powerModel.router, (*fit).second.device_tech, power_type);
		}
		leakage = (I)nocs->power.readOp.leakage + (I)nocs->power.readOp.gate_leakage;
		//using namespace io_interval;  std::cout << "CompID " << c->getId() << ", noc leakage = " << leakage << " fid =  " << floorplan_id.router << std::endl;
		dynamicPower = (I)nocs->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "getPower::CompID " << c->getId() << ", noc unit energy = " << nocs->rt_power.readOp.dynamic << " and exe time = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)nocs->power.readOp.dynamic * (I)router_tech.clockrate; 
		#endif                       
	      break;
	      case 1:
	      /*SimPanalyzer*/
	      break;
              case 2:
	      /*McPAT05*/
		#ifdef McPAT05_H
		if(sscanf(user_parms, "%d", &total_router_accesses) != 1) {
    		    fprintf(stderr, "getPower: bad router params: <total_router_accesses>");
    		    exit(1);
  		}
		//executionTime = 1.0 / (I)router_tech.clockrate * (I)total_cycles;
                totalPowerUsage = (((I)inputBuffer.caches.local_result.power.readOp.dynamic + (I)inputBuffer.caches.local_result.power.writeOp.dynamic) 
				+ (I)xbar.total_power.readOp.dynamic + (I)vcAllocatorStage1.power.readOp.dynamic * (I)vcAllocatorStage1.numArbiters / 2.0
				+ (I)vcAllocatorStage2.power.readOp.dynamic * (I)vcAllocatorStage2.numArbiters / 2.0
				+ (I)switchAllocatorStage1.power.readOp.dynamic * (I)switchAllocatorStage1.numArbiters / 2.0
				+ (I)switchAllocatorStage2.power.readOp.dynamic * (I)switchAllocatorStage2.numArbiters / 2.0				
				+ (I)globalInterconnect.power_link.readOp.dynamic * 2.0) * (I)total_router_accesses / executionTime
				+ (I)RTclockNetwork.power_link.readOp.dynamic * (I)router_tech.clockrate + (I)RTpipeLogic.power.readOp.dynamic * (I)router_tech.clockrate; 
		#endif
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
		dynamicPower = (I)counts.router_access * (I)p_unitPower.router;
		leakage = (I)intsim_router->chip->leakage_power_logic_gates + intsim_router->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

         case 4:
         /*ORION*/
         #ifdef ORION_H
         //totalPowerUsage = (I)p_unitPower.router * (I)counts.router_access;
	 totalPowerUsage = (I)p_unitPower.router / executionTime * (I)counts.router_access;  //here unit power is actuall unit energy
         //dynamicPower = 0.25 * (I)LinkDynamicEnergyPerBitPerMeter(router_tech.link_length*1e-6, router_tech.vdd) * (I)router_tech.clockrate * (I)router_tech.link_length*1e-6 * (I)router_tech.flit_bits * (I)router_tech.input_ports * (I)counts.router_access;
	 dynamicPower = 0.25 * (I)LinkDynamicEnergyPerBitPerMeter(router_tech.link_length*1e-6, router_tech.vdd) * (I)router_tech.link_length*1e-6 * (I)router_tech.flit_bits * (I)router_tech.input_ports * (I)counts.router_access / executionTime;
         leakage = (I)LinkLeakagePowerPerMeter(router_tech.link_length*1e-6, router_tech.vdd) * (I)router_tech.link_length*1e-6 * (I)router_tech.flit_bits * (I)router_tech.input_ports;
         totalPowerUsage = dynamicPower + leakage + totalPowerUsage;
         #endif
         break;

	    }	
	   
	    updatePowUsage(c, power_type, floorplan_id.router.at(0), &p_usage_router, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
         
	  case 27:
	  //load_Q
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.loadQ)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		lsu->SSTcomputeEnergy(false, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[0], counts.LSQ_write[0], counts.loadQ_read[i], counts.loadQ_write[i]);		
		leakage = (I)LoadQ->power.readOp.leakage + (I)LoadQ->power.readOp.gate_leakage;
		dynamicPower = (I)LoadQ->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;		
		TDP = (I)LoadQ->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif      
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.loadQ_access * (I)p_unitPower.loadQ;
		leakage = (I)intsim_loadQ->chip->leakage_power_logic_gates + intsim_loadQ->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.loadQ.at(i), &p_usage_loadQ[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 28:
	  //rename_U
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.rename)
	    {
	      case 0:
	       /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		rnu->SSTcomputeEnergy(false, counts.iFRAT_read[i], counts.iFRAT_write[i], counts.iFRAT_search[i], counts.fFRAT_read[i], counts.fFRAT_write[i], counts.fFRAT_search[i], counts.iRRAT_read[i],
			counts.iRRAT_write[i], counts.fRRAT_read[i], counts.fRRAT_write[i],	counts.ifreeL_read[i], counts.ifreeL_write[i], counts.ffreeL_read[i], counts.ffreeL_write[i], counts.idcl_read[i], counts.fdcl_read[i]);
		leakage = (I)rnu->power.readOp.leakage + (I)rnu->power.readOp.gate_leakage;
		dynamicPower = (I)rnu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)rnu->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif   
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.rename_access * (I)p_unitPower.rename;
		leakage = (I)intsim_rename->chip->leakage_power_logic_gates + intsim_rename->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.rename.at(i), &p_usage_renameU[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 29:
	  //scheduler_U
	    for(i = 0; i < device_tech.number_core; i++){
	    switch(p_powerModel.scheduler)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		scheu->SSTcomputeEnergy(false, counts.int_win_read[i], counts.int_win_write[i], counts.int_win_search[i], counts.fp_win_read[i], counts.fp_win_write[i], counts.fp_win_search[i], counts.ROB_read[i], counts.ROB_write[i]);		
		leakage = (I)scheu->power.readOp.leakage + (I)scheu->power.readOp.gate_leakage;
		dynamicPower = (I)scheu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		scheu->SSTcomputeEnergy(true, 4, 4, 1, 1, 4, 4);
		TDP = (I)scheu->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif   
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.scheduler_access * (I)p_unitPower.scheduler;
		leakage = (I)intsim_scheduler->chip->leakage_power_logic_gates + intsim_scheduler->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.scheduler.at(i), &p_usage_schedulerU[i], totalPowerUsage, dynamicPower, leakage, TDP);
	  } //end i for loop
	  break;
	  case 30:
	  //cache_l3
	    for(i = 0; i < device_tech.number_L3; i++){
	    switch(p_powerModel.L3)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		l3array = p_Mproc.SSTreturnL3(i);
		l3array->SSTcomputeEnergy(false, counts.L2_read[i], counts.L2_readmiss[i], counts.L2_write[i], counts.L2_writemiss[i], counts.L3_read[i], counts.L3_readmiss[i], 
			counts.L3_write[i], counts.L3_writemiss[i], counts.L1Dir_read[i], counts.L1Dir_readmiss[i], counts.L1Dir_write[i], counts.L1Dir_writemiss[i], 
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i], counts.homeL2_read[i], counts.homeL2_readmiss[i], counts.homeL2_write[i], counts.homeL2_writemiss[i], counts.homeL3_read[i], counts.homeL3_readmiss[i], 
			counts.homeL3_write[i], counts.homeL3_writemiss[i]);
		leakage = (I)l3array->power.readOp.leakage + (I)l3array->power.readOp.gate_leakage;
		dynamicPower = (I)l3array->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", l3 leakage = " << leakage << "readOP = " << l3array->power.readOp.leakage << " gate = " << l3array->power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", l3array->rt_power = " << l3array->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)l3array->power.readOp.dynamic * (I)cache_l3_tech.op_freq; //(cache_clockrate)
		#endif     
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.l3_access * (I)p_unitPower.l3;
		leakage = (I)intsim_L3->chip->leakage_power_logic_gates + intsim_L3->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.L3.at(i), &p_usage_cache_l3[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    }
	  break;	
	  case 31:
	  //l1dir
	    for(i = 0; i < device_tech.number_L1dir; i++){
	    switch(p_powerModel.L1dir)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		l1dirarray = p_Mproc.SSTreturnL1dir(i);
		l1dirarray->SSTcomputeEnergy(false, counts.L2_read[i], counts.L2_readmiss[i], counts.L2_write[i], counts.L2_writemiss[i], counts.L3_read[i], counts.L3_readmiss[i], 
			counts.L3_write[i], counts.L3_writemiss[i], counts.L1Dir_read[i], counts.L1Dir_readmiss[i], counts.L1Dir_write[i], counts.L1Dir_writemiss[i], 
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i], counts.homeL2_read[i], counts.homeL2_readmiss[i], counts.homeL2_write[i], counts.homeL2_writemiss[i], counts.homeL3_read[i], counts.homeL3_readmiss[i], 
			counts.homeL3_write[i], counts.homeL3_writemiss[i]);
		leakage = (I)l1dirarray->power.readOp.leakage + (I)l1dirarray->power.readOp.gate_leakage;
		dynamicPower = (I)l1dirarray->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", l1dir leakage = " << leakage << "readOP = " << l1dirarray->power.readOp.leakage << " gate = " << l1dirarray->power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", l1dirarray->rt_power = " << l1dirarray->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)l1dirarray->power.readOp.dynamic * (I)cache_l1dir_tech.op_freq; //(cache_clockrate)
		#endif  
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.l1dir_access * (I)p_unitPower.l1dir;
		leakage = (I)intsim_L1dir->chip->leakage_power_logic_gates + intsim_L1dir->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    #ifdef MESMTHI_H
		    updatePowUsage(c, power_type, i /*floorplan id*/, &p_usage_cache_l1dir[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #else
	    	    updatePowUsage(c, power_type, floorplan_id.L1dir.at(i), &p_usage_cache_l1dir[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    #endif
	    }
	  break;	
	  case 32:
	  //l2dir
	    for(i = 0; i < device_tech.number_L2dir; i++){
	    switch(p_powerModel.L2dir)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		l2dirarray = p_Mproc.SSTreturnL2dir(i);
		l2dirarray->SSTcomputeEnergy(false, counts.L2_read[i], counts.L2_readmiss[i], counts.L2_write[i], counts.L2_writemiss[i], counts.L3_read[i], counts.L3_readmiss[i], 
			counts.L3_write[i], counts.L3_writemiss[i], counts.L1Dir_read[i], counts.L1Dir_readmiss[i], counts.L1Dir_write[i], counts.L1Dir_writemiss[i], 
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i], counts.homeL2_read[i], counts.homeL2_readmiss[i], counts.homeL2_write[i], counts.homeL2_writemiss[i], counts.homeL3_read[i], counts.homeL3_readmiss[i], 
			counts.homeL3_write[i], counts.homeL3_writemiss[i]);
		leakage = (I)l2dirarray->power.readOp.leakage + (I)l2dirarray->power.readOp.gate_leakage;
		dynamicPower = (I)l2dirarray->rt_power.readOp.dynamic / executionTime;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", l2dir leakage = " << leakage << "readOP = " << l2dirarray->power.readOp.leakage << " gate = " << l2dirarray->power.readOp.gate_leakage << std::endl;
		//using namespace io_interval;  std::cout << "SST CompID " << c->getId() << ", dynamicPower = " << dynamicPower << ", l2dirarray->rt_power = " << l2dirarray->rt_power.readOp.dynamic << ", executionTime = " << executionTime << std::endl;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)l2dirarray->power.readOp.dynamic * (I)cache_l2dir_tech.op_freq; //(cache_clockrate)
		#endif     
	      break;
	      case 1:
	      /*SimPanalyzer*/	
		
	      break;
	      case 2:
	      /*McPAT05*/                    
	      break;
	      case 3:
	      /*IntSim*/
		#ifdef INTSIM_H
                dynamicPower = (I)counts.l2dir_access * (I)p_unitPower.l2dir;
		leakage = (I)intsim_L2dir->chip->leakage_power_logic_gates + intsim_L2dir->chip->leakage_power_repeaters;    
		totalPowerUsage = dynamicPower+leakage;
		#endif
              break;

          case 4:
          /*ORION*/
          break;
	    } // end switch power model
	    updatePowUsage(c, power_type, floorplan_id.L2dir.at(i), &p_usage_cache_l2dir[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    }
	  break;										
	  case 33:
	  //uarch
	        ////totalPowerUsage = 1*p_unitPower.uarch; //usage_count*p_unitPower.uarch;
                ////updatePowUsage(c, power_type, 0, &p_usage_uarch, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	} // end switch power_type
	return p_usage_uarch;
    } // end else

}

/***************************************************************
* Update component's currentPower, totalEnergy, and peak power *
****************************************************************/
void Power::updatePowUsage(IntrospectedComponent *c, ptype power_type, int fid, Pdissipation_t *comp_pusage, const I& totalPowerUsage, const I& dynamicPower, const I& leakage, const I& TDP)
{
	unsigned i=0;
	I tempPr, tempPl, tempPc, tempPt = 0.0;

	// update "itemized (ptype)" power
	comp_pusage->totalEnergy = comp_pusage->totalEnergy + totalPowerUsage;
	comp_pusage->currentPower = totalPowerUsage; //=runtime dynamic power + leakage
	comp_pusage->leakagePower = leakage; //=threshold leakage + gate leakage
	comp_pusage->runtimeDynamicPower = dynamicPower;
	comp_pusage->TDP = TDP;

	if ( median(p_meanPeak) < median(comp_pusage->currentPower) ){
	     p_meanPeak = comp_pusage->currentPower;
             comp_pusage->peak = p_meanPeak * I(0.95,1.05);  //manual error bar (5%)
	}

	//comp_pusage->currentSimTime = Simulation::getSimulation()->getCurrentSimCycle() / Simulation::getSimulation()->getFreq(); //convert ps to s
	  comp_pusage->currentSimTime = Simulation::getSimulation()->getCurrentSimCycle() * Simulation::getSimulation()->getTimeLord()->getSecFactor() / 1000000000000000000.0; //convert sim time base to s
	//std::cout << "sim time base is in " << Simulation::getSimulation()->getTimeLord()->getSecFactor() / 1000000000000000000.0 << " sec." << std::endl;


	switch(power_type)
	{
	  case CACHE_IL1:
		for(i = 0; i < device_tech.number_il1; i++){
		    tempPr += p_usage_cache_il1[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_il1[i].leakagePower;
		    tempPc += p_usage_cache_il1[i].currentPower;
		    tempPt += p_usage_cache_il1[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.il1 = tempPr;
		p_usage_uarch.itemizedLeakagePower.il1 = tempPl;
		p_usage_uarch.itemizedCurrentPower.il1 = tempPc;
		p_usage_uarch.itemizedTDP.il1 = tempPt;
		p_usage_uarch.itemizedTotalPower.il1 += comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.il1 = comp_pusage->peak;
	  break;
	  case CACHE_IL2:
		p_usage_uarch.itemizedRuntimeDynamicPower.il2 = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.il2 = leakage;
		p_usage_uarch.itemizedCurrentPower.il2 = totalPowerUsage;
		p_usage_uarch.itemizedTDP.il2 = TDP;
		p_usage_uarch.itemizedTotalPower.il2 = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.il2 = comp_pusage->peak;
	  break;
	  case CACHE_DL1:
		for(i = 0; i < device_tech.number_dl1; i++){
		    tempPr += p_usage_cache_dl1[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_dl1[i].leakagePower;
		    tempPc += p_usage_cache_dl1[i].currentPower;
		    tempPt += p_usage_cache_dl1[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.dl1 = tempPr;
		p_usage_uarch.itemizedLeakagePower.dl1 = tempPl;
		p_usage_uarch.itemizedCurrentPower.dl1 = tempPc;
		p_usage_uarch.itemizedTDP.dl1 = tempPt;
		p_usage_uarch.itemizedTotalPower.dl1 += comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.dl1 = comp_pusage->peak;
	  break;
	  case CACHE_DL2:
		p_usage_uarch.itemizedRuntimeDynamicPower.dl2 = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.dl2 = leakage;
		p_usage_uarch.itemizedCurrentPower.dl2 = totalPowerUsage;
		p_usage_uarch.itemizedTDP.dl2 = TDP;
		p_usage_uarch.itemizedTotalPower.dl2 = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.dl2 = comp_pusage->peak;
	  break;
	  case CACHE_ITLB:
		p_usage_uarch.itemizedRuntimeDynamicPower.itlb = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.itlb = leakage;
		p_usage_uarch.itemizedCurrentPower.itlb = totalPowerUsage;
		p_usage_uarch.itemizedTDP.itlb = TDP;
		p_usage_uarch.itemizedTotalPower.itlb = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.itlb = comp_pusage->peak;
	  break;
	  case CACHE_DTLB:
		p_usage_uarch.itemizedRuntimeDynamicPower.dtlb = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.dtlb = leakage;
		p_usage_uarch.itemizedCurrentPower.dtlb = totalPowerUsage;
		p_usage_uarch.itemizedTDP.dtlb = TDP;
		p_usage_uarch.itemizedTotalPower.dtlb = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.dtlb = comp_pusage->peak;
	  break;
	  case CLOCK:
		p_usage_uarch.itemizedRuntimeDynamicPower.clock = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.clock = leakage;
		p_usage_uarch.itemizedCurrentPower.clock = totalPowerUsage;
		p_usage_uarch.itemizedTDP.clock = TDP;
		p_usage_uarch.itemizedTotalPower.clock = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.clock = comp_pusage->peak;
	  break;
	  case BPRED:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_bpred[i].runtimeDynamicPower;
		    tempPl += p_usage_bpred[i].leakagePower;
		    tempPc += p_usage_bpred[i].currentPower;
		    tempPt += p_usage_bpred[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.bpred = tempPr;
		p_usage_uarch.itemizedLeakagePower.bpred = tempPl;
		p_usage_uarch.itemizedCurrentPower.bpred = tempPc;
		p_usage_uarch.itemizedTDP.bpred = tempPt;
		p_usage_uarch.itemizedTotalPower.bpred = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.bpred = comp_pusage->peak;
	  break;
	  case RF:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_rf[i].runtimeDynamicPower;
		    tempPl += p_usage_rf[i].leakagePower;
		    tempPc += p_usage_rf[i].currentPower;
		    tempPt += p_usage_rf[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.rf = tempPr;
		p_usage_uarch.itemizedLeakagePower.rf = tempPl;
		p_usage_uarch.itemizedCurrentPower.rf = tempPc;
		p_usage_uarch.itemizedTDP.rf = tempPt;
		p_usage_uarch.itemizedTotalPower.rf = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.rf = comp_pusage->peak;
	  break;
	  case IO:
		p_usage_uarch.itemizedRuntimeDynamicPower.io = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.io = leakage;
		p_usage_uarch.itemizedCurrentPower.io = totalPowerUsage;
		p_usage_uarch.itemizedTDP.io = TDP;
		p_usage_uarch.itemizedTotalPower.io = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.io = comp_pusage->peak;
	  break;
	  case LOGIC:
		p_usage_uarch.itemizedRuntimeDynamicPower.logic = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.logic = leakage;
		p_usage_uarch.itemizedCurrentPower.logic = totalPowerUsage;
		p_usage_uarch.itemizedTDP.logic = TDP;
		p_usage_uarch.itemizedTotalPower.logic = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.logic = comp_pusage->peak;
	  break;
	  case EXEU_ALU:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_alu[i].runtimeDynamicPower;
		    tempPl += p_usage_alu[i].leakagePower;
		    tempPc += p_usage_alu[i].currentPower;
		    tempPt += p_usage_alu[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.alu = tempPr;
		p_usage_uarch.itemizedLeakagePower.alu = tempPl;
		p_usage_uarch.itemizedCurrentPower.alu = tempPc;
		p_usage_uarch.itemizedTDP.alu = tempPt;
		p_usage_uarch.itemizedTotalPower.alu = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.alu = comp_pusage->peak;
	  break;
	  case EXEU_FPU:
		p_usage_uarch.itemizedRuntimeDynamicPower.fpu = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.fpu = leakage;
		p_usage_uarch.itemizedCurrentPower.fpu = totalPowerUsage;
		p_usage_uarch.itemizedTDP.fpu = TDP;
		p_usage_uarch.itemizedTotalPower.fpu = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.fpu = comp_pusage->peak;
	  break;
	  case MULT:
		p_usage_uarch.itemizedRuntimeDynamicPower.mult = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.mult = leakage;
		p_usage_uarch.itemizedCurrentPower.mult = totalPowerUsage;
		p_usage_uarch.itemizedTDP.mult = TDP;
		p_usage_uarch.itemizedTotalPower.mult = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.mult = comp_pusage->peak;
	  break;
	  case 14: 
		//IB
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_ib[i].runtimeDynamicPower;
		    tempPl += p_usage_ib[i].leakagePower;
		    tempPc += p_usage_ib[i].currentPower;
		    tempPt += p_usage_ib[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.ib = tempPr;
		p_usage_uarch.itemizedLeakagePower.ib = tempPl;
		p_usage_uarch.itemizedCurrentPower.ib = tempPc;
		p_usage_uarch.itemizedTDP.ib = tempPt;
		p_usage_uarch.itemizedTotalPower.ib = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.ib = comp_pusage->peak;
	  break;
	  case ISSUE_Q:
		p_usage_uarch.itemizedRuntimeDynamicPower.issueQ = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.issueQ = leakage;
		p_usage_uarch.itemizedCurrentPower.issueQ = totalPowerUsage;
		p_usage_uarch.itemizedTDP.issueQ = TDP;
		p_usage_uarch.itemizedTotalPower.issueQ = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.issueQ = comp_pusage->peak;
	  break;
	  case INST_DECODER:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_decoder[i].runtimeDynamicPower;
		    tempPl += p_usage_decoder[i].leakagePower;
		    tempPc += p_usage_decoder[i].currentPower;
		    tempPt += p_usage_decoder[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.decoder = tempPr;
		p_usage_uarch.itemizedLeakagePower.decoder = tempPl;
		p_usage_uarch.itemizedCurrentPower.decoder = tempPc;
		p_usage_uarch.itemizedTDP.decoder = tempPt;
		p_usage_uarch.itemizedTotalPower.decoder = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.decoder = comp_pusage->peak;
	  break;
	  case BYPASS:
		p_usage_uarch.itemizedRuntimeDynamicPower.bypass = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.bypass = leakage;
		p_usage_uarch.itemizedCurrentPower.bypass = totalPowerUsage;
		p_usage_uarch.itemizedTDP.bypass = TDP;
		p_usage_uarch.itemizedTotalPower.bypass = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.bypass = comp_pusage->peak;
	  break;
	  case EXEU:
		p_usage_uarch.itemizedRuntimeDynamicPower.exeu = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.exeu = leakage;
		p_usage_uarch.itemizedCurrentPower.exeu = totalPowerUsage;
		p_usage_uarch.itemizedTDP.exeu = TDP;
		p_usage_uarch.itemizedTotalPower.exeu = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.exeu = comp_pusage->peak;
	  break;
	  case PIPELINE:
		p_usage_uarch.itemizedRuntimeDynamicPower.pipeline = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.pipeline = leakage;
		p_usage_uarch.itemizedCurrentPower.pipeline = totalPowerUsage;
		p_usage_uarch.itemizedTDP.pipeline = TDP;
		p_usage_uarch.itemizedTotalPower.pipeline = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.pipeline = comp_pusage->peak;
	  break;
	  case 20:  //LSQ
	for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_lsq[i].runtimeDynamicPower;
		    tempPl += p_usage_lsq[i].leakagePower;
		    tempPc += p_usage_lsq[i].currentPower;
		    tempPt += p_usage_lsq[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.lsq = tempPr;
		p_usage_uarch.itemizedLeakagePower.lsq = tempPl;
		p_usage_uarch.itemizedCurrentPower.lsq = tempPc;
		p_usage_uarch.itemizedTDP.lsq = tempPt;
		p_usage_uarch.itemizedTotalPower.lsq = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.lsq = comp_pusage->peak;
	  break;
	  case RAT:
		p_usage_uarch.itemizedRuntimeDynamicPower.rat = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.rat = leakage;
		p_usage_uarch.itemizedCurrentPower.rat = totalPowerUsage;
		p_usage_uarch.itemizedTDP.rat = TDP;
		p_usage_uarch.itemizedTotalPower.rat = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.rat = comp_pusage->peak;
	  break;
	  case 22:  //ROB
		p_usage_uarch.itemizedRuntimeDynamicPower.rob = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.rob = leakage;
		p_usage_uarch.itemizedCurrentPower.rob = totalPowerUsage;
		p_usage_uarch.itemizedTDP.rob = TDP;
		p_usage_uarch.itemizedTotalPower.rob = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.rob = comp_pusage->peak;
	  break;
	  case 23:  //BTB
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_btb[i].runtimeDynamicPower;
		    tempPl += p_usage_btb[i].leakagePower;
		    tempPc += p_usage_btb[i].currentPower;
		    tempPt += p_usage_btb[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.btb = tempPr;
		p_usage_uarch.itemizedLeakagePower.btb = tempPl;
		p_usage_uarch.itemizedCurrentPower.btb = tempPc;
		p_usage_uarch.itemizedTDP.btb = tempPt;
		p_usage_uarch.itemizedTotalPower.btb = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.btb = comp_pusage->peak;
	  break;
	  case CACHE_L2:
		for(i = 0; i < device_tech.number_L2; i++){
		    tempPr += p_usage_cache_l2[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_l2[i].leakagePower;
		    tempPc += p_usage_cache_l2[i].currentPower;
		    tempPt += p_usage_cache_l2[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.L2 = tempPr;
		p_usage_uarch.itemizedLeakagePower.L2 = tempPl;
		p_usage_uarch.itemizedCurrentPower.L2 = tempPc;
		p_usage_uarch.itemizedTDP.L2 = tempPt;
		p_usage_uarch.itemizedTotalPower.L2 += comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.L2 = comp_pusage->peak;
	  break;
	  case MEM_CTRL:
		p_usage_uarch.itemizedRuntimeDynamicPower.mc = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.mc = leakage;
		p_usage_uarch.itemizedCurrentPower.mc = totalPowerUsage;
		p_usage_uarch.itemizedTDP.mc = TDP;
		p_usage_uarch.itemizedTotalPower.mc = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.mc = comp_pusage->peak;
	  break;
	  case ROUTER:
		p_usage_uarch.itemizedRuntimeDynamicPower.router = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.router = leakage;
		p_usage_uarch.itemizedCurrentPower.router = totalPowerUsage;
		p_usage_uarch.itemizedTDP.router = TDP;
		p_usage_uarch.itemizedTotalPower.router = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.router = comp_pusage->peak;
	  break;
	  case LOAD_Q:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_loadQ[i].runtimeDynamicPower;
		    tempPl += p_usage_loadQ[i].leakagePower;
		    tempPc += p_usage_loadQ[i].currentPower;
		    tempPt += p_usage_loadQ[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.loadQ = tempPr;
		p_usage_uarch.itemizedLeakagePower.loadQ = tempPl;
		p_usage_uarch.itemizedCurrentPower.loadQ = tempPc;
		p_usage_uarch.itemizedTDP.loadQ = tempPt;
		p_usage_uarch.itemizedTotalPower.loadQ = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.loadQ = comp_pusage->peak;
	  break;
	  case RENAME_U:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_renameU[i].runtimeDynamicPower;
		    tempPl += p_usage_renameU[i].leakagePower;
		    tempPc += p_usage_renameU[i].currentPower;
		    tempPt += p_usage_renameU[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.renameU = tempPr;
		p_usage_uarch.itemizedLeakagePower.renameU = tempPl;
		p_usage_uarch.itemizedCurrentPower.renameU = tempPc;
		p_usage_uarch.itemizedTDP.renameU = tempPt;
		p_usage_uarch.itemizedTotalPower.renameU = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.renameU = comp_pusage->peak;
	  break;
	  case SCHEDULER_U:
		for(i = 0; i < device_tech.number_core; i++){
		    tempPr += p_usage_schedulerU[i].runtimeDynamicPower;
		    tempPl += p_usage_schedulerU[i].leakagePower;
		    tempPc += p_usage_schedulerU[i].currentPower;
		    tempPt += p_usage_schedulerU[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.schedulerU = tempPr;
		p_usage_uarch.itemizedLeakagePower.schedulerU = tempPl;
		p_usage_uarch.itemizedCurrentPower.schedulerU = tempPc;
		p_usage_uarch.itemizedTDP.schedulerU = tempPt;
		p_usage_uarch.itemizedTotalPower.schedulerU = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.schedulerU = comp_pusage->peak;
	  break;
	  case CACHE_L3:
		for(i = 0; i < device_tech.number_L3; i++){
		    tempPr += p_usage_cache_l3[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_l3[i].leakagePower;
		    tempPc += p_usage_cache_l3[i].currentPower;
		    tempPt += p_usage_cache_l3[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.L3 = tempPr;
		p_usage_uarch.itemizedLeakagePower.L3 = tempPl;
		p_usage_uarch.itemizedCurrentPower.L3 = tempPc;
		p_usage_uarch.itemizedTDP.L3 = tempPt;
		p_usage_uarch.itemizedTotalPower.L3 += comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.L3 = comp_pusage->peak;
	  break;
	  case CACHE_L1DIR:
		for(i = 0; i < device_tech.number_L1dir; i++){
		    tempPr += p_usage_cache_l1dir[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_l1dir[i].leakagePower;
		    tempPc += p_usage_cache_l1dir[i].currentPower;
		    tempPt += p_usage_cache_l1dir[i].TDP;		    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.L1dir = tempPr;
		p_usage_uarch.itemizedLeakagePower.L1dir = tempPl;
		p_usage_uarch.itemizedCurrentPower.L1dir = tempPc;
		p_usage_uarch.itemizedTDP.L1dir = tempPt;
		p_usage_uarch.itemizedTotalPower.L1dir = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.L1dir = comp_pusage->peak;
	  break;
	  case CACHE_L2DIR:
		for(i = 0; i < device_tech.number_L2dir; i++){
		    tempPr += p_usage_cache_l2dir[i].runtimeDynamicPower;
		    tempPl += p_usage_cache_l2dir[i].leakagePower;
		    tempPc += p_usage_cache_l2dir[i].currentPower;
		    tempPt += p_usage_cache_l2dir[i].TDP;	    
		}
		p_usage_uarch.itemizedRuntimeDynamicPower.L2dir = tempPr;
		p_usage_uarch.itemizedLeakagePower.L2dir = tempPl;
		p_usage_uarch.itemizedCurrentPower.L2dir = tempPc;
		p_usage_uarch.itemizedTDP.L2dir = tempPt;
		p_usage_uarch.itemizedTotalPower.L2dir = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.L2dir = comp_pusage->peak;
	  break;
	  default:
	  break;
	}

	//update floorplan power information
	map<int,floorplan_t>::iterator fit = p_chip.floorplan.find(fid);
        if(fit == p_chip.floorplan.end())
            cout << "ERROR: No matching floorplan is found" << endl;
	if((*fit).second.p_usage_floorplan.currentSimTime != comp_pusage->currentSimTime){
	    //reset the power except totalEnergy & peak
	    (*fit).second.p_usage_floorplan.currentPower = 0.0;
	    (*fit).second.p_usage_floorplan.leakagePower = 0.0;
	    (*fit).second.p_usage_floorplan.runtimeDynamicPower = 0.0;
	    (*fit).second.p_usage_floorplan.TDP = 0.0;
	    (*fit).second.p_usage_floorplan.currentSimTime = comp_pusage->currentSimTime;
	}
	(*fit).second.p_usage_floorplan.totalEnergy += totalPowerUsage;
	(*fit).second.p_usage_floorplan.currentPower += totalPowerUsage;
	(*fit).second.p_usage_floorplan.leakagePower += leakage;
	(*fit).second.p_usage_floorplan.runtimeDynamicPower += dynamicPower;
	(*fit).second.p_usage_floorplan.TDP += TDP;
	//using namespace io_interval; std::cout << "comp ID " << c->getId() << " reside on FID " << fid << " has new fp currentPower= " << (*fit).second.p_usage_floorplan.currentPower << std::endl;
	if ( median((*fit).second.p_usage_floorplan.peak) < median(totalPowerUsage) ){
             (*fit).second.p_usage_floorplan.peak = totalPowerUsage * I(0.95,1.05);  //manual error bar (5%)
	}
	

	// update component overall (ALL) power
	p_usage_uarch.totalEnergy = p_usage_uarch.totalEnergy + totalPowerUsage;
	p_usage_uarch.currentPower = p_usage_uarch.itemizedCurrentPower.il1 + 
					p_usage_cache_il2.currentPower +
					p_usage_uarch.itemizedCurrentPower.dl1 +
					p_usage_cache_dl2.currentPower +
					p_usage_cache_itlb.currentPower +
					p_usage_cache_dtlb.currentPower +
					p_usage_clock.currentPower +
					p_usage_io.currentPower +
					p_usage_logic.currentPower +
					p_usage_uarch.itemizedCurrentPower.alu +
					p_usage_fpu.currentPower +
					p_usage_mult.currentPower +	
					p_usage_uarch.itemizedCurrentPower.rf +
					p_usage_uarch.itemizedCurrentPower.bpred +
					p_usage_uarch.itemizedCurrentPower.ib +
					p_usage_rs.currentPower +
					p_usage_uarch.itemizedCurrentPower.decoder +
					p_usage_bypass.currentPower +
					p_usage_exeu.currentPower +
					p_usage_pipeline.currentPower +
					p_usage_uarch.itemizedCurrentPower.lsq +
					p_usage_rat.currentPower +
					p_usage_rob.currentPower +
					p_usage_uarch.itemizedCurrentPower.btb +
					p_usage_uarch.itemizedCurrentPower.L2 +
					p_usage_mc.currentPower +
					p_usage_uarch.itemizedCurrentPower.renameU +
					p_usage_uarch.itemizedCurrentPower.schedulerU +
					p_usage_uarch.itemizedCurrentPower.loadQ +
					p_usage_uarch.itemizedCurrentPower.L3 +
					p_usage_uarch.itemizedCurrentPower.L1dir +
					p_usage_uarch.itemizedCurrentPower.L2dir +
					p_usage_router.currentPower;
	p_usage_uarch.leakagePower = p_usage_uarch.itemizedLeakagePower.il1 + 
					p_usage_cache_il2.leakagePower +
					p_usage_uarch.itemizedLeakagePower.dl1 +
					p_usage_cache_dl2.leakagePower +
					p_usage_cache_itlb.leakagePower +
					p_usage_cache_dtlb.leakagePower +
					p_usage_clock.leakagePower +
					p_usage_io.leakagePower +
					p_usage_logic.leakagePower +
					p_usage_uarch.itemizedLeakagePower.alu +
					p_usage_fpu.leakagePower +
					p_usage_mult.leakagePower +	
					p_usage_uarch.itemizedLeakagePower.rf +
					p_usage_uarch.itemizedLeakagePower.bpred +
					p_usage_uarch.itemizedLeakagePower.ib +
					p_usage_rs.leakagePower +
					p_usage_uarch.itemizedLeakagePower.decoder +
					p_usage_bypass.leakagePower +
					p_usage_exeu.leakagePower +
					p_usage_pipeline.leakagePower +
					p_usage_uarch.itemizedLeakagePower.lsq +
					p_usage_rat.leakagePower +
					p_usage_rob.leakagePower +
					p_usage_uarch.itemizedLeakagePower.btb +
					p_usage_uarch.itemizedLeakagePower.L2 +
					p_usage_mc.leakagePower +
					p_usage_uarch.itemizedLeakagePower.renameU +
					p_usage_uarch.itemizedLeakagePower.schedulerU +
					p_usage_uarch.itemizedLeakagePower.loadQ +
					p_usage_uarch.itemizedLeakagePower.L3 +
					p_usage_uarch.itemizedLeakagePower.L1dir +
					p_usage_uarch.itemizedLeakagePower.L2dir +
					p_usage_router.leakagePower;
/*using namespace io_interval; std:: cout << "SST total leakage " << p_usage_uarch.itemizedLeakagePower.il1 << " "
	<< p_usage_uarch.itemizedLeakagePower.dl1 << " " << p_usage_cache_itlb.leakagePower << " " 
	<< p_usage_cache_dtlb.leakagePower << " " << p_usage_alu.leakagePower << " "
	<< p_usage_fpu.leakagePower << " " << p_usage_rf.leakagePower << " "
	<< p_usage_bpred.leakagePower << " " << p_usage_ib.leakagePower << " "
	<< p_usage_bypass.leakagePower << " " << p_usage_lsq.leakagePower << " "
	<< p_usage_schedulerU.leakagePower << std::endl; */
	p_usage_uarch.runtimeDynamicPower = p_usage_uarch.itemizedRuntimeDynamicPower.il1 + 
					p_usage_cache_il2.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.dl1 +
					p_usage_cache_dl2.runtimeDynamicPower +
					p_usage_cache_itlb.runtimeDynamicPower +
					p_usage_cache_dtlb.runtimeDynamicPower +
					p_usage_clock.runtimeDynamicPower +
					p_usage_io.runtimeDynamicPower +
					p_usage_logic.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.alu +
					p_usage_fpu.runtimeDynamicPower +
					p_usage_mult.runtimeDynamicPower +	
					p_usage_uarch.itemizedRuntimeDynamicPower.rf +
					p_usage_uarch.itemizedRuntimeDynamicPower.bpred +
					p_usage_uarch.itemizedRuntimeDynamicPower.ib +
					p_usage_rs.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.decoder +
					p_usage_bypass.runtimeDynamicPower +
					p_usage_exeu.runtimeDynamicPower +
					p_usage_pipeline.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.lsq +
					p_usage_rat.runtimeDynamicPower +
					p_usage_rob.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.btb +
					p_usage_uarch.itemizedRuntimeDynamicPower.L2 +
					p_usage_mc.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.renameU +
					p_usage_uarch.itemizedRuntimeDynamicPower.schedulerU +
					p_usage_uarch.itemizedRuntimeDynamicPower.loadQ +
					p_usage_uarch.itemizedRuntimeDynamicPower.L3 +
					p_usage_uarch.itemizedRuntimeDynamicPower.L1dir +
					p_usage_uarch.itemizedRuntimeDynamicPower.L2dir +
					p_usage_router.runtimeDynamicPower;
	p_usage_uarch.TDP = p_usage_uarch.itemizedTDP.il1 + 
					p_usage_cache_il2.TDP +
					p_usage_uarch.itemizedTDP.dl1 +
					p_usage_cache_dl2.TDP +
					p_usage_cache_itlb.TDP +
					p_usage_cache_dtlb.TDP +
					p_usage_clock.TDP +
					p_usage_io.TDP +
					p_usage_logic.TDP +
					p_usage_uarch.itemizedTDP.alu +
					p_usage_fpu.TDP +
					p_usage_mult.TDP +	
					p_usage_uarch.itemizedTDP.rf +
					p_usage_uarch.itemizedTDP.bpred +
					p_usage_uarch.itemizedTDP.ib +
					p_usage_rs.TDP +
					p_usage_uarch.itemizedTDP.decoder +
					p_usage_bypass.TDP +
					p_usage_exeu.TDP +
					p_usage_pipeline.TDP +
					p_usage_uarch.itemizedTDP.lsq +
					p_usage_rat.TDP +
					p_usage_rob.TDP +
					p_usage_uarch.itemizedTDP.btb +
					p_usage_uarch.itemizedTDP.L2 +
					p_usage_mc.TDP +
					p_usage_uarch.itemizedTDP.renameU +
					p_usage_uarch.itemizedTDP.schedulerU +
					p_usage_uarch.itemizedTDP.loadQ +
					p_usage_uarch.itemizedTDP.L3 +
					p_usage_uarch.itemizedTDP.L1dir +
					p_usage_uarch.itemizedTDP.L2dir +
					p_usage_router.TDP;	
	if ( median(p_meanPeakAll) < median(totalPowerUsage) ){
	     p_meanPeakAll = totalPowerUsage;
             p_usage_uarch.peak = p_meanPeakAll * I(0.95,1.05);  //manual error bar (5%)
	}
	
	p_usage_uarch.currentSimTime = comp_pusage->currentSimTime ;

}

/***************************************************************
* Get execution time (= total cycles / clockrate)              *
* Here execution time equals current sim time minus            *
* the time when power was last analyzed (obtained from PDB)    *
* Return time in second                                   
* NOTE, Power = Energy/time = Energy_per_access * Access_counts/Time.
* Here "time is core's view of time, and in SST this is related 
* to component's current cycle & component's default timebase and 
* can be obtained by Simulation::getCurrentSimCycle() (sim cycle
* actually is sim "time"					*
****************************************************************/
I Power::getExecutionTime(IntrospectedComponent *c)
{
        Time_t current, previous;
        Pdissipation_t pusage;
        I time;
        
        current = (Time_t)Simulation::getSimulation()->getCurrentSimCycle();
	//the above is the simulation core time with default unit, ps.
	
	//convert current sim time to s
	//current = current / Simulation::getSimulation()->getFreq();
	current = current * Simulation::getSimulation()->getTimeLord()->getSecFactor() / 1000000000000000000.0; 
	// =cycle*default time base (=as->ps->s)	

        std::pair<bool, Pdissipation_t> res = c->readPowerStats(c);

	if(!res.first)  //there is no entry in PDB yet; i.e. previous (=start) time = 0
	    previous = 0; 
	else	
            previous = res.second.currentSimTime; //in PDB, currentSimTime is in unit, sec.  

        time = (I)current - (I)previous;

	//using namespace io_interval; std::cout << "current = " << current << " and previous = " << previous << std::endl;
        
        return (time);

}

/************************************************************************
* Decouple the power-related parameters from Component::Params_t params *
*************************************************************************/
void Power::setTech(Component::Params_t deviceParams)
{
    Component::Params_t::iterator it= deviceParams.begin();
    unsigned int i, n;
    char chtmp[60];
    char chtmp1[60];
    chtmp1[0]='\0';

	std::cout << "l2dir floorplan size = " << floorplan_id.L2dir.size() << endl; 
    //set up # elements in the floorplan structure
    while (it != deviceParams.end()){
	if (!it->first.compare("number_of_cores")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_core);
		//resize RF para vector
    p_usage_rf.resize(device_tech.number_core);
    floorplan_id.rf.resize(device_tech.number_core);
     //resize ALU para vector
    p_usage_alu.resize(device_tech.number_core);
    floorplan_id.alu.resize(device_tech.number_core);
    //resize LSQ para vector
    p_usage_lsq.resize(device_tech.number_core);
    floorplan_id.lsq.resize(device_tech.number_core);
    //resize LOAD_Q para vector
    p_usage_loadQ.resize(device_tech.number_core);
    floorplan_id.loadQ.resize(device_tech.number_core);
    //resize IB para vector
    p_usage_ib.resize(device_tech.number_core);
    floorplan_id.ib.resize(device_tech.number_core);
    //resize INST_DECODER para vector
    p_usage_decoder.resize(device_tech.number_core);
    floorplan_id.decoder.resize(device_tech.number_core);
    //resize RENAME_U para vector
    p_usage_renameU.resize(device_tech.number_core);
    floorplan_id.rename.resize(device_tech.number_core);
    //resize SCHEDULER_U para vector
    p_usage_schedulerU.resize(device_tech.number_core);
    floorplan_id.scheduler.resize(device_tech.number_core);
    //resize BTB para vector
    p_usage_btb.resize(device_tech.number_core);
    floorplan_id.btb.resize(device_tech.number_core);
    //resize BPT para vector
    p_usage_bpred.resize(device_tech.number_core);
    floorplan_id.bpred.resize(device_tech.number_core);
	}
	else if (!it->first.compare("number_of_IL1s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_il1);
		if (device_tech.number_il1 != 0){
		    //resize il1 para vector
		    cache_il1_tech.unit_scap.resize(device_tech.number_il1);
    		    cache_il1_tech.line_size.resize(device_tech.number_il1);
    		    cache_il1_tech.num_banks.resize(device_tech.number_il1);
    		    cache_il1_tech.assoc.resize(device_tech.number_il1);
    		    cache_il1_tech.throughput.resize(device_tech.number_il1);
    		    cache_il1_tech.latency.resize(device_tech.number_il1);
    		    cache_il1_tech.miss_buf_size.resize(device_tech.number_il1);
    		    cache_il1_tech.fill_buf_size.resize(device_tech.number_il1);
    		    cache_il1_tech.prefetch_buf_size.resize(device_tech.number_il1);
    		    cache_il1_tech.wbb_buf_size.resize(device_tech.number_il1);
    		    cache_il1_tech.output_width.resize(device_tech.number_il1);
    		    cache_il1_tech.cache_policy.resize(device_tech.number_il1);
		    p_usage_cache_il1.resize(device_tech.number_il1);
    		    floorplan_id.il1.resize(device_tech.number_il1);
		}
	}
	else if (!it->first.compare("number_of_DL1s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_dl1);
		if(device_tech.number_dl1 != 0){
		//resize dl1 para vector
    		  cache_dl1_tech.unit_scap.resize(device_tech.number_dl1);
   		  cache_dl1_tech.line_size.resize(device_tech.number_dl1);
  		  cache_dl1_tech.num_banks.resize(device_tech.number_dl1);
   		 cache_dl1_tech.assoc.resize(device_tech.number_dl1);
   		 cache_dl1_tech.throughput.resize(device_tech.number_dl1);
   		 cache_dl1_tech.latency.resize(device_tech.number_dl1);
   		 cache_dl1_tech.miss_buf_size.resize(device_tech.number_dl1);
   		 cache_dl1_tech.fill_buf_size.resize(device_tech.number_dl1);
   		 cache_dl1_tech.prefetch_buf_size.resize(device_tech.number_dl1);
   		 cache_dl1_tech.wbb_buf_size.resize(device_tech.number_dl1);
   		 cache_dl1_tech.output_width.resize(device_tech.number_dl1);
   		 cache_dl1_tech.cache_policy.resize(device_tech.number_dl1);
   		 p_usage_cache_dl1.resize(device_tech.number_dl1);
   		 floorplan_id.dl1.resize(device_tech.number_dl1);
		}
	}
	else if (!it->first.compare("number_of_ITLBs")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_itlb);
		if(device_tech.number_itlb != 0){
		//resize itlb para vector
    		cache_itlb_tech.unit_scap.resize(device_tech.number_itlb);
    		cache_itlb_tech.line_size.resize(device_tech.number_itlb);
    		cache_itlb_tech.num_banks.resize(device_tech.number_itlb);
    		cache_itlb_tech.assoc.resize(device_tech.number_itlb);
    		cache_itlb_tech.throughput.resize(device_tech.number_itlb);
    		cache_itlb_tech.latency.resize(device_tech.number_itlb);
    		cache_itlb_tech.miss_buf_size.resize(device_tech.number_itlb);    
    		cache_itlb_tech.fill_buf_size.resize(device_tech.number_itlb);    
    		cache_itlb_tech.prefetch_buf_size.resize(device_tech.number_itlb);    
    		cache_itlb_tech.wbb_buf_size.resize(device_tech.number_itlb); 
    		cache_itlb_tech.output_width.resize(device_tech.number_itlb);
    		cache_itlb_tech.cache_policy.resize(device_tech.number_itlb);
		}
	}
	else if (!it->first.compare("number_of_DTLBs")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_dtlb);
		if(device_tech.number_dtlb != 0){
		//resize dtlb para vector
    		cache_dtlb_tech.unit_scap.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.line_size.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.num_banks.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.assoc.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.throughput.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.latency.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.miss_buf_size.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.fill_buf_size.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.prefetch_buf_size.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.wbb_buf_size.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.output_width.resize(device_tech.number_dtlb);
    		cache_dtlb_tech.cache_policy.resize(device_tech.number_dtlb);
		}
	}
	else if (!it->first.compare("number_of_L2s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L2);
		if(device_tech.number_L2 != 0){
		//resize l2 para vector
    		cache_l2_tech.unit_scap.resize(device_tech.number_L2);
    		cache_l2_tech.line_size.resize(device_tech.number_L2);
    		cache_l2_tech.num_banks.resize(device_tech.number_L2);
    		cache_l2_tech.assoc.resize(device_tech.number_L2);
    		cache_l2_tech.throughput.resize(device_tech.number_L2);
    		cache_l2_tech.latency.resize(device_tech.number_L2);
    		cache_l2_tech.miss_buf_size.resize(device_tech.number_L2);             
    		cache_l2_tech.fill_buf_size.resize(device_tech.number_L2);                                                               
    		cache_l2_tech.prefetch_buf_size.resize(device_tech.number_L2);                                                 
    		cache_l2_tech.wbb_buf_size.resize(device_tech.number_L2);
    		cache_l2_tech.output_width.resize(device_tech.number_L2);
    		cache_l2_tech.cache_policy.resize(device_tech.number_L2);                                                               
   		 p_usage_cache_l2.resize(device_tech.number_L2);
    		floorplan_id.L2.resize(device_tech.number_L2);
		}
	}
	else if (!it->first.compare("number_of_L3s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L3);
		if(device_tech.number_L3 != 0){
		//resize l3 para vector
    		cache_l3_tech.unit_scap.resize(device_tech.number_L3);
    		cache_l3_tech.line_size.resize(device_tech.number_L3);
    		cache_l3_tech.num_banks.resize(device_tech.number_L3);
   		 cache_l3_tech.assoc.resize(device_tech.number_L3);
   		 cache_l3_tech.throughput.resize(device_tech.number_L3);
   		 cache_l3_tech.latency.resize(device_tech.number_L3);
    		cache_l3_tech.miss_buf_size.resize(device_tech.number_L3);
    		cache_l3_tech.fill_buf_size.resize(device_tech.number_L3);
    		cache_l3_tech.prefetch_buf_size.resize(device_tech.number_L3);
   		 cache_l3_tech.wbb_buf_size.resize(device_tech.number_L3);
   		 cache_l3_tech.output_width.resize(device_tech.number_L3);
    		cache_l3_tech.cache_policy.resize(device_tech.number_L3);
   		 p_usage_cache_l3.resize(device_tech.number_L3);
    		floorplan_id.L3.resize(device_tech.number_L3);
		}
	}
	else if (!it->first.compare("number_of_L1Directories")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L1dir);
		if(device_tech.number_L1dir != 0){
		//resize l1dir para vector
    		cache_l1dir_tech.unit_scap.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.line_size.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.num_banks.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.assoc.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.throughput.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.latency.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.miss_buf_size.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.fill_buf_size.resize(device_tech.number_L1dir);
   		 cache_l1dir_tech.prefetch_buf_size.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.wbb_buf_size.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.output_width.resize(device_tech.number_L1dir);
    		cache_l1dir_tech.cache_policy.resize(device_tech.number_L1dir);
    		p_usage_cache_l1dir.resize(device_tech.number_L1dir);
    		floorplan_id.L1dir.resize(device_tech.number_L1dir);
		}
	}
	else if (!it->first.compare("number_of_L2Directories")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L2dir);
		if(device_tech.number_L2dir != 0){
		//resize l2dir para vector
    		cache_l2dir_tech.unit_scap.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.line_size.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.num_banks.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.assoc.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.throughput.resize(device_tech.number_L2dir);
    		cache_l2dir_tech.latency.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.miss_buf_size.resize(device_tech.number_L2dir);
    		cache_l2dir_tech.fill_buf_size.resize(device_tech.number_L2dir);
    		cache_l2dir_tech.prefetch_buf_size.resize(device_tech.number_L2dir);
    		cache_l2dir_tech.wbb_buf_size.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.output_width.resize(device_tech.number_L2dir);
   		 cache_l2dir_tech.cache_policy.resize(device_tech.number_L2dir);
    		p_usage_cache_l2dir.resize(device_tech.number_L2dir);
    		floorplan_id.L2dir.resize(device_tech.number_L2dir);
		}
	}
	it++;
    }  

    
    
         
    
    std::cout << "2nd time: l2dir floorplan size = " << floorplan_id.L2dir.size() << endl;
    
 

    it= deviceParams.begin();
    while (it != deviceParams.end()){
	if (!it->first.compare("machine_type")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.machineType);
	}
	else if (!it->first.compare("core_clock_rate")){  //Mc
	        sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
	}
	else if(!it->first.compare("core_temperature")){  //Mc
		sscanf(it->second.c_str(), "%d", &core_tech.core_temperature);
	} 
	else if(!it->first.compare("core_tech_node")){  //Mc
		sscanf(it->second.c_str(), "%d", &core_tech.core_tech_node);
	} 
	else if (!it->first.compare("cache_il1_floorplan_id")){  
		////sscanf(it->second.c_str(), "%d", &floorplan_id.il1); //keep for the non-mesmthi case
		// Insert subcomponents type of component's interest into a list
		// Used for compute_temperature
	    	i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.il1.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(CACHE_IL1,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.il1.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(CACHE_IL1,atoi(chtmp1)));
		chtmp1[0]='\0';    
	}
	else if (!it->first.compare("cache_il2_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.il2.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_IL2,floorplan_id.il2.at(0)));  
	}
	else if (!it->first.compare("cache_dl1_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.dl1); 
		// Insert subcomponents type of component's interest into a list
		// Used for compute_temperature
	    	i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.dl1.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(CACHE_DL1,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.dl1.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(CACHE_DL1,atoi(chtmp1)));
		chtmp1[0]='\0';   	      
	}
	else if (!it->first.compare("cache_dl2_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.dl2.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_DL2,floorplan_id.dl2.at(0)));  
	}
	else if (!it->first.compare("cache_itlb_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.itlb.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_ITLB,floorplan_id.itlb.at(0)));  
	}
	else if (!it->first.compare("cache_dtlb_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.dtlb.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_DTLB,floorplan_id.dtlb.at(0)));  
	}
	else if (!it->first.compare("clock_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.clock.at(0));
		subcompList.insert(std::pair<ptype,int>(CLOCK,floorplan_id.clock.at(0)));  
	}
	else if (!it->first.compare("bpred_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.bpred);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.bpred.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(BPRED,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.bpred.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(BPRED,atoi(chtmp1)));
		chtmp1[0]='\0';    
 
	}
	else if (!it->first.compare("rf_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.rf);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.rf.at(i) = atoi(chtmp1);
				std::cout << "floorplan_id.rf(" << i << ")= " << floorplan_id.rf.at(i) << std::endl;
				subcompList.insert(std::pair<ptype,int>(RF,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.rf.at(i) = atoi(chtmp1);
		std::cout << "floorplan_id.rf(" << i << ")= " << floorplan_id.rf.at(i) << std::endl;
		subcompList.insert(std::pair<ptype,int>(RF,atoi(chtmp1)));
		chtmp1[0]='\0';    
  
	}
	else if (!it->first.compare("io_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.io.at(0));
		subcompList.insert(std::pair<ptype,int>(IO,floorplan_id.io.at(0)));  
	}
	else if (!it->first.compare("logic_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.logic.at(0));
		subcompList.insert(std::pair<ptype,int>(LOGIC,floorplan_id.logic.at(0)));  
	}
	else if (!it->first.compare("alu_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.alu);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.alu.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(EXEU_ALU,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.alu.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(EXEU_ALU,atoi(chtmp1)));
		chtmp1[0]='\0';    
 
	}
	else if (!it->first.compare("fpu_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.fpu.at(0));
		subcompList.insert(std::pair<ptype,int>(EXEU_FPU,floorplan_id.fpu.at(0)));  
	}
	else if (!it->first.compare("mult_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.mult.at(0));
		subcompList.insert(std::pair<ptype,int>(MULT,floorplan_id.mult.at(0)));  
	}
	else if (!it->first.compare("ib_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.ib);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.ib.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>((ptype)14,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.ib.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>((ptype)14,atoi(chtmp1)));
		chtmp1[0]='\0';    
 
	}
	else if (!it->first.compare("issueQ_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.issueQ.at(0));
		subcompList.insert(std::pair<ptype,int>(ISSUE_Q,floorplan_id.issueQ.at(0)));  
	}
	else if (!it->first.compare("decoder_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.decoder);  
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.decoder.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(INST_DECODER,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.decoder.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(INST_DECODER,atoi(chtmp1)));
		chtmp1[0]='\0';    

	}
	else if (!it->first.compare("bypass_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.bypass.at(0));
		subcompList.insert(std::pair<ptype,int>(BYPASS,floorplan_id.bypass.at(0)));  
	}
	else if (!it->first.compare("exeu_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.exeu.at(0));
		subcompList.insert(std::pair<ptype,int>(EXEU,floorplan_id.exeu.at(0)));  
	}
	else if (!it->first.compare("pipeline_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.pipeline.at(0));
		subcompList.insert(std::pair<ptype,int>(PIPELINE,floorplan_id.pipeline.at(0)));  
	}
	else if (!it->first.compare("lsq_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.lsq);  
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.lsq.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>((ptype)20,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.lsq.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>((ptype)20,atoi(chtmp1)));
		chtmp1[0]='\0';    

	}
	else if (!it->first.compare("rat_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.rat.at(0));
		subcompList.insert(std::pair<ptype,int>(RAT,floorplan_id.rat.at(0)));  
	}
	else if (!it->first.compare("rob_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.rob.at(0));
		subcompList.insert(std::pair<ptype,int>((ptype)22,floorplan_id.rob.at(0)));  
	}
	else if (!it->first.compare("btb_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.btb);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.btb.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>((ptype)23,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.btb.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>((ptype)23,atoi(chtmp1)));
		chtmp1[0]='\0';    
  
	}
	else if (!it->first.compare("cache_l2_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.L2); 
		// Insert subcomponents type of component's interest into a list
		// Used for compute_temperature
	    	i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.L2.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(CACHE_L2,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.L2.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(CACHE_L2,atoi(chtmp1)));
		chtmp1[0]='\0';   	        
	}
	else if (!it->first.compare("router_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.router.at(0));
		subcompList.insert(std::pair<ptype,int>(ROUTER,floorplan_id.router.at(0)));  
	}
	else if (!it->first.compare("mc_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.mc.at(0)); 
		// Insert subcomponents type of component's interest into a list
		// Used for compute_temperature
		subcompList.insert(std::pair<ptype,int>(MEM_CTRL,floorplan_id.mc.at(0)));
	      
	}
	else if (!it->first.compare("loadQ_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.loadQ);
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.loadQ.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(LOAD_Q,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.loadQ.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(LOAD_Q,atoi(chtmp1)));
		chtmp1[0]='\0';    
  
	}
	else if (!it->first.compare("rename_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.rename); 
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.rename.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(RENAME_U,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.rename.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(RENAME_U,atoi(chtmp1)));
		chtmp1[0]='\0';    

	}
	else if (!it->first.compare("scheduler_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.scheduler);  
		i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.scheduler.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(SCHEDULER_U,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.scheduler.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(SCHEDULER_U,atoi(chtmp1)));
		chtmp1[0]='\0';    

	}
	else if (!it->first.compare("cache_l3_floorplan_id")){  
	        sscanf(it->second.c_str(), "%d", &floorplan_id.L3.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_L3,floorplan_id.L3.at(0)));  
	}
	else if (!it->first.compare("cache_l1dir_floorplan_id")){  
	        ////sscanf(it->second.c_str(), "%d", &floorplan_id.L1dir); //keep for the non-mesmthi case
		// Insert subcomponents type of component's interest into a list
		// Used for compute_temperature
	    	i=0;
		for(n=0; n < it->second.length(); n++)
		{
			if (it->second[n]!=',')
			{
				sprintf(chtmp,"%c",it->second[n]);
				strcat(chtmp1,chtmp);
			}
			else{
				floorplan_id.L1dir.at(i) = atoi(chtmp1);
				subcompList.insert(std::pair<ptype,int>(CACHE_L1DIR,atoi(chtmp1)));
				////assert((i+1) < device_tech.number_il1);
				i = i + 1;
				chtmp1[0]='\0';
			}
		}
		floorplan_id.L1dir.at(i) = atoi(chtmp1);
		subcompList.insert(std::pair<ptype,int>(CACHE_L1DIR,atoi(chtmp1)));
		chtmp1[0]='\0';     
	}
	else if (!it->first.compare("cache_l2dir_floorplan_id")){ 
		std::cout << " I entered here at l2dir" << std::endl; 
	        sscanf(it->second.c_str(), "%d", &floorplan_id.L2dir.at(0));
		subcompList.insert(std::pair<ptype,int>(CACHE_L2DIR,floorplan_id.L2dir.at(0)));  
	}
	else if (!it->first.compare("power_monitor")){
	    if (!it->second.compare("NO"))
		p_powerMonitor = false;
	    else
		p_powerMonitor = true;
	}
	else if (!it->first.compare("temperature_monitor")){
	    if (!it->second.compare("NO"))
		p_tempMonitor = false;
	    else
		p_tempMonitor = true;
	}
	else if (!it->first.compare("opt_for_clk")){  //this is important!!! true or false will give different power values
	    if (!it->second.compare("YES"))
		opt_for_clk = true;
	    else
		opt_for_clk = false;
	}
	else if(!it->first.compare("system_failure_type")){ 
	        if (!it->second.compare("SERIES"))
			p_systemTopology = SERIES;
		else if (!it->second.compare("SERIES_PARALLEL"))
			p_systemTopology = SERIES_PARALLEL;
		else if (!it->second.compare("PARALLEL"))
			p_systemTopology = PARALLEL;
	}
        it++;
    }
    
}


/***************************************************************
* Reset component's counter values to zero                     *
***************************************************************/
void Power::resetCounts(usagecounts_t *counts)
{ 
	memset(counts, 0, sizeof(usagecounts_t)); //memset also makes uninitialized vector size becomes 0 
}

/***************************************************************
* Estimate clock die area					*
* Copied from sim-panalyzer (sim-panalyzer.c). Die area are     *
* estimated by methods in S-P when xxx_pspec is created.        * 
****************************************************************/
double Power::estimateClockDieAreaSimPan ()
{
        double tdarea = 0;

        #ifdef LV2_PANALYZER_H
	if(rf_pspec)
		tdarea
			+= rf_pspec->dimension->area;
	if(bpred_pspec)
		tdarea
			+= bpred_pspec->dimension->area;

	if(il1_pspec && (il1_pspec != dl1_pspec && il1_pspec != dl2_pspec))
		tdarea
			+= il1_pspec->dimension->area;
	if(dl1_pspec)
		tdarea
			+= dl1_pspec->dimension->area;
	if(il2_pspec && (il2_pspec != dl1_pspec && il2_pspec != dl2_pspec))
		tdarea
		    += il1_pspec->dimension->area;
	if(dl2_pspec)
		tdarea
	       += dl1_pspec->dimension->area;
	if(itlb_pspec)
		tdarea
		   += itlb_pspec->dimension->area;
	if(dtlb_pspec)
	        tdarea
		    += dtlb_pspec->dimension->area;
	#endif
	return (tdarea);
	
}

/***************************************************************
* total clocked node capacitance in F				*
* Copied from sim-panalyzer (sim-panalyzer.c). Capacitance are  *
* estimated by methods in S-P when xxx_pspec is created.        * 
****************************************************************/
double Power::estimateClockNodeCapSimPan()
{
	double tcnodeCeff = 0;

	#ifdef LV2_PANALYZER_H
        if(rf_pspec) {
	   tcnodeCeff
		       += rf_pspec->Ceffs->cnodeCeff;
	   
	}
	if(bpred_pspec) {
	   tcnodeCeff
		   += bpred_pspec->Ceffs->cnodeCeff;
	   
	}
       
        if(il1_pspec && (il1_pspec != dl1_pspec && il1_pspec != dl2_pspec)) {
	        tcnodeCeff
		    += (il1_pspec->t_Ceffs->cnodeCeff + il1_pspec->d_Ceffs->cnodeCeff);
	}
  
       if(dl1_pspec) {
	        tcnodeCeff
		    += (dl1_pspec->t_Ceffs->cnodeCeff + dl1_pspec->d_Ceffs->cnodeCeff);
	    
	}
				  
	if(il2_pspec && (il2_pspec != dl1_pspec && il2_pspec != dl2_pspec)) {
		tcnodeCeff
			+= (il2_pspec->t_Ceffs->cnodeCeff + il2_pspec->d_Ceffs->cnodeCeff);
		
	}
	if(dl2_pspec) {
		tcnodeCeff
			+= (dl2_pspec->t_Ceffs->cnodeCeff + dl2_pspec->d_Ceffs->cnodeCeff);
		
	}
	if(itlb_pspec) {
		tcnodeCeff
		    += (itlb_pspec->t_Ceffs->cnodeCeff + itlb_pspec->d_Ceffs->cnodeCeff);
	        
	}
	if(dtlb_pspec) {
	        tcnodeCeff
		    += (dtlb_pspec->t_Ceffs->cnodeCeff + dtlb_pspec->d_Ceffs->cnodeCeff);
		
	}
	#endif
	return (tcnodeCeff);
	
} 

#ifdef McPAT07_H
/***************************************************************
* Pass tech params to McPAT07	 			       *	
* McPAT07 interface				               * 
****************************************************************/
void Power::McPATSetup()
{
        unsigned int i;

   //All number_of_* at the level of 'system' 03/21/2009
	p_Mp1->sys.number_of_cores=1;
	p_Mp1->sys.number_of_L1Directories=device_tech.number_L1dir;
	p_Mp1->sys.number_of_L2Directories=device_tech.number_L2dir;
	p_Mp1->sys.number_of_L2s = device_tech.number_L2;
	p_Mp1->sys.Private_L2 = false;
	p_Mp1->sys.number_of_L3s=device_tech.number_L3;
	p_Mp1->sys.number_of_NoCs = core_tech.core_number_of_NoCs;
	// All params at the level of 'system'
	p_Mp1->sys.core_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.target_core_clockrate=2660;
	p_Mp1->sys.target_chip_area=200;
	p_Mp1->sys.temperature = core_tech.core_temperature;
	p_Mp1->sys.number_cache_levels=3;
	p_Mp1->sys.homogeneous_cores=1;
	p_Mp1->sys.homogeneous_L1Directories=0;
	p_Mp1->sys.homogeneous_L2Directories=0;
	p_Mp1->sys.homogeneous_L2s=0;  //always set hetero so we can create multiple copies in McPAT
	p_Mp1->sys.homogeneous_L3s=0;
	p_Mp1->sys.homogeneous_NoCs=1;
	p_Mp1->sys.homogeneous_ccs=1;
	p_Mp1->sys.Max_area_deviation=10;
	p_Mp1->sys.Max_power_deviation=50;
	p_Mp1->sys.device_type=0;
	p_Mp1->sys.longer_channel_device =(bool)core_tech.core_long_channel;
	p_Mp1->sys.Embedded =false;

	p_Mp1->sys.opt_dynamic_power=1;
	p_Mp1->sys.opt_lakage_power=0;
	p_Mp1->sys.opt_clockrate=0;
	p_Mp1->sys.opt_area=0;
	p_Mp1->sys.interconnect_projection_type=0;
	
	p_Mp1->sys.L1_property=0; 
	p_Mp1->sys.L2_property=3;	
	p_Mp1->sys.L3_property=2;
		
	p_Mp1->sys.virtual_memory_page_size = core_tech.core_virtual_memory_page_size;
		p_Mp1->sys.core[0].clock_rate=(int)(device_tech.clockRate/1000000); // Mc unit is MHz
		p_Mp1->sys.core[0].opt_local = true;
		p_Mp1->sys.core[0].x86 = true;
		p_Mp1->sys.core[0].machine_bits = core_tech.machine_bits;
		p_Mp1->sys.core[0].virtual_address_width = core_tech.core_virtual_address_width;
		p_Mp1->sys.core[0].physical_address_width = core_tech.core_physical_address_width;
		p_Mp1->sys.core[0].instruction_length = core_tech.core_instruction_length;
		p_Mp1->sys.core[0].opcode_width = core_tech.core_opcode_width;
		p_Mp1->sys.core[0].micro_opcode_width = core_tech.core_micro_opcode_width;

		p_Mp1->sys.core[0].machine_type = device_tech.machineType;
		p_Mp1->sys.core[0].internal_datapath_width=64;
		p_Mp1->sys.core[0].number_hardware_threads = core_tech.core_number_hardware_threads;
		p_Mp1->sys.core[0].fetch_width = core_tech.core_fetch_width;
	p_Mp1->sys.core[0].number_instruction_fetch_ports=core_tech.core_number_instruction_fetch_ports;
		p_Mp1->sys.core[0].decode_width = core_tech.core_decode_width;
		p_Mp1->sys.core[0].issue_width = core_tech.core_issue_width;
		p_Mp1->sys.core[0].peak_issue_width = core_tech.core_peak_issue_width;
		p_Mp1->sys.core[0].commit_width = core_tech.core_commit_width;
		p_Mp1->sys.core[0].pipelines_per_core[0]=1;
		p_Mp1->sys.core[0].pipeline_depth[0] = core_tech.core_int_pipeline_depth;
		strcpy(p_Mp1->sys.core[0].FPU,"1");
		strcpy(p_Mp1->sys.core[0].divider_multiplier,"1");
		p_Mp1->sys.core[0].ALU_per_core = core_tech.ALU_per_core;
		p_Mp1->sys.core[0].FPU_per_core = core_tech.FPU_per_core;
		p_Mp1->sys.core[0].MUL_per_core = core_tech.MUL_per_core;
		p_Mp1->sys.core[0].instruction_buffer_size = core_tech.core_instruction_buffer_size;
		p_Mp1->sys.core[0].decoded_stream_buffer_size=16;		
		//strcpy(sys.core[i].instruction_window_scheme,"default");
		p_Mp1->sys.core[0].instruction_window_scheme=0;
		p_Mp1->sys.core[0].instruction_window_size = core_tech.core_instruction_window_size;
		p_Mp1->sys.core[0].ROB_size = core_tech.core_ROB_size;
		p_Mp1->sys.core[0].archi_Regs_IRF_size = core_tech.archi_Regs_IRF_size;
		p_Mp1->sys.core[0].archi_Regs_FRF_size = core_tech.archi_Regs_FRF_size;
		p_Mp1->sys.core[0].phy_Regs_IRF_size = core_tech.core_phy_Regs_IRF_size;
		p_Mp1->sys.core[0].phy_Regs_FRF_size = core_tech.core_phy_Regs_FRF_size;
		//strcpy(sys.core[i].rename_scheme,"default");
		p_Mp1->sys.core[0].rename_scheme=0;
		p_Mp1->sys.core[0].register_windows_size = core_tech.core_register_windows_size;
		strcpy(p_Mp1->sys.core[0].LSU_order,"inorder");
		p_Mp1->sys.core[0].store_buffer_size = core_tech.core_store_buffer_size;
		p_Mp1->sys.core[0].load_buffer_size = core_tech.core_load_buffer_size;
		p_Mp1->sys.core[0].memory_ports = core_tech.core_memory_ports;
		strcpy(p_Mp1->sys.core[0].Dcache_dual_pump,"N");
		p_Mp1->sys.core[0].RAS_size = core_tech.core_RAS_size;
		//all stats at the level of p_Mp1->system.core(0-n)
		p_Mp1->sys.core[0].total_instructions=2;
		p_Mp1->sys.core[0].int_instructions=2;
		p_Mp1->sys.core[0].fp_instructions=2;
		p_Mp1->sys.core[0].branch_instructions=2;
		p_Mp1->sys.core[0].branch_mispredictions=2;
		p_Mp1->sys.core[0].committed_instructions=2;
		p_Mp1->sys.core[0].load_instructions=2;
		p_Mp1->sys.core[0].store_instructions=2;
		p_Mp1->sys.core[0].total_cycles=1;
		p_Mp1->sys.core[0].idle_cycles=0;
		p_Mp1->sys.core[0].busy_cycles=1;
		p_Mp1->sys.core[0].instruction_buffer_reads=2;
		p_Mp1->sys.core[0].instruction_buffer_write=2;
		p_Mp1->sys.core[0].ROB_reads=2;
		p_Mp1->sys.core[0].ROB_writes=2;
		p_Mp1->sys.core[0].rename_accesses=2;
		p_Mp1->sys.core[0].inst_window_reads=2;
		p_Mp1->sys.core[0].inst_window_writes=2;
		p_Mp1->sys.core[0].inst_window_wakeup_accesses=2;
		p_Mp1->sys.core[0].inst_window_selections=2;
		p_Mp1->sys.core[0].archi_int_regfile_reads=2;
		p_Mp1->sys.core[0].archi_float_regfile_reads=2;
		p_Mp1->sys.core[0].phy_int_regfile_reads=2;
		p_Mp1->sys.core[0].phy_float_regfile_reads=2;
		p_Mp1->sys.core[0].windowed_reg_accesses=2;
		p_Mp1->sys.core[0].windowed_reg_transports=2;
		p_Mp1->sys.core[0].function_calls=2;
		p_Mp1->sys.core[0].ialu_accesses=1;
		p_Mp1->sys.core[0].fpu_accesses=1;
		p_Mp1->sys.core[0].mul_accesses=1;
		p_Mp1->sys.core[0].cdb_alu_accesses=1;
		p_Mp1->sys.core[0].cdb_mul_accesses=1;
		p_Mp1->sys.core[0].cdb_fpu_accesses=1;
		//p_Mp1->sys.core[0].bypassbus_access=1;
		p_Mp1->sys.core[0].load_buffer_reads=1;
		p_Mp1->sys.core[0].load_buffer_writes=1;
		p_Mp1->sys.core[0].load_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_reads=1;
		p_Mp1->sys.core[0].store_buffer_writes=1;
		p_Mp1->sys.core[0].store_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_forwards=1;
		p_Mp1->sys.core[0].main_memory_access=6;
		p_Mp1->sys.core[0].main_memory_read=3;
		p_Mp1->sys.core[0].main_memory_write=3;
		p_Mp1->sys.core[0].IFU_duty_cycle = 1;
		p_Mp1->sys.core[0].BR_duty_cycle = 1;		
		p_Mp1->sys.core[0].LSU_duty_cycle = 1;
		p_Mp1->sys.core[0].MemManU_I_duty_cycle =1;
		p_Mp1->sys.core[0].MemManU_D_duty_cycle =1;
		p_Mp1->sys.core[0].ALU_duty_cycle =1;
		p_Mp1->sys.core[0].MUL_duty_cycle =1;
		p_Mp1->sys.core[0].FPU_duty_cycle =1;
		p_Mp1->sys.core[0].ALU_cdb_duty_cycle =1;
		p_Mp1->sys.core[0].MUL_cdb_duty_cycle =1;
		p_Mp1->sys.core[0].FPU_cdb_duty_cycle =1;
		//p_Mp1->system.core?.predictor
		p_Mp1->sys.core[0].predictor.prediction_width = bpred_tech.prediction_width;
		strcpy(p_Mp1->sys.core[0].predictor.prediction_scheme,"tournament");
		p_Mp1->sys.core[0].predictor.predictor_size=2;
		p_Mp1->sys.core[0].predictor.predictor_entries=1024;
		p_Mp1->sys.core[0].predictor.local_predictor_entries = bpred_tech.local_predictor_entries;
		for (int j=0; j<20; j++) p_Mp1->sys.core[0].predictor.local_predictor_size[j] = bpred_tech.local_predictor_size;
		p_Mp1->sys.core[0].predictor.global_predictor_entries = bpred_tech.global_predictor_entries;
		p_Mp1->sys.core[0].predictor.global_predictor_bits = bpred_tech.global_predictor_bits;
		p_Mp1->sys.core[0].predictor.chooser_predictor_entries = bpred_tech.chooser_predictor_entries;
		p_Mp1->sys.core[0].predictor.chooser_predictor_bits = bpred_tech.chooser_predictor_bits;
		p_Mp1->sys.core[0].predictor.predictor_accesses=263886;
		//p_Mp1->system.core?.itlb
		p_Mp1->sys.core[0].itlb.number_entries=cache_itlb_tech.number_entries;
		p_Mp1->sys.core[0].itlb.total_hits=1;
		p_Mp1->sys.core[0].itlb.total_accesses=1;
		p_Mp1->sys.core[0].itlb.total_misses=0;
		//p_Mp1->system.core?.icache
		for(i=0; i < device_tech.number_il1; i++){
		p_Mp1->sys.core[0].icache.icache_config[0]=(int)cache_il1_tech.unit_scap.at(i);
		p_Mp1->sys.core[0].icache.icache_config[1]=cache_il1_tech.line_size.at(i);
		p_Mp1->sys.core[0].icache.icache_config[2]=cache_il1_tech.assoc.at(i);
		p_Mp1->sys.core[0].icache.icache_config[3]=cache_il1_tech.num_banks.at(i);
		p_Mp1->sys.core[0].icache.icache_config[4]=(int)cache_il1_tech.throughput.at(i);
		p_Mp1->sys.core[0].icache.icache_config[5]=(int)cache_il1_tech.latency.at(i);
		p_Mp1->sys.core[0].icache.icache_config[6]=(int)cache_il1_tech.output_width.at(i);
		p_Mp1->sys.core[0].icache.icache_config[7]=(int)cache_il1_tech.cache_policy.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[0]=cache_il1_tech.miss_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[1]=cache_il1_tech.fill_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[2]=cache_il1_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[3]=cache_il1_tech.wbb_buf_size.at(i);
		p_Mp1->sys.core[0].icache.total_accesses=1;
		p_Mp1->sys.core[0].icache.read_accesses=1;
		p_Mp1->sys.core[0].icache.read_misses=1;
		p_Mp1->sys.core[0].icache.replacements=0;
		p_Mp1->sys.core[0].icache.read_hits=1;
		p_Mp1->sys.core[0].icache.total_hits=1;
		p_Mp1->sys.core[0].icache.total_misses=1;
		p_Mp1->sys.core[0].icache.miss_buffer_access=1;
		p_Mp1->sys.core[0].icache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_hits=1;
		}
		//system.core?.dtlb
		for(i=0; i < device_tech.number_dtlb; i++){
		p_Mp1->sys.core[0].dtlb.number_entries=cache_dtlb_tech.number_entries;
		p_Mp1->sys.core[0].dtlb.total_accesses=2;
		p_Mp1->sys.core[0].dtlb.read_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_hits=1;
		p_Mp1->sys.core[0].dtlb.read_hits=1;
		p_Mp1->sys.core[0].dtlb.read_misses=0;
		p_Mp1->sys.core[0].dtlb.write_misses=0;
		p_Mp1->sys.core[0].dtlb.total_hits=1;
		p_Mp1->sys.core[0].dtlb.total_misses=1;
		}
		//system.core?.dcache
		for(i=0; i < device_tech.number_dl1; i++){
		p_Mp1->sys.core[0].dcache.dcache_config[0]=(int)cache_dl1_tech.unit_scap.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[1]=cache_dl1_tech.line_size.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[2]=cache_dl1_tech.assoc.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[3]=cache_dl1_tech.num_banks.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[4]=(int)cache_dl1_tech.throughput.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[5]=(int)cache_dl1_tech.latency.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[6]=(int)cache_dl1_tech.output_width.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[7]=(int)cache_dl1_tech.cache_policy.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[0]=cache_dl1_tech.miss_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[1]=cache_dl1_tech.fill_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[2]=cache_dl1_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[3]=cache_dl1_tech.wbb_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.total_accesses=2;
		p_Mp1->sys.core[0].dcache.read_accesses=1;
		p_Mp1->sys.core[0].dcache.write_accesses=1;
		p_Mp1->sys.core[0].dcache.total_hits=1;
		p_Mp1->sys.core[0].dcache.total_misses=0;
		p_Mp1->sys.core[0].dcache.read_hits=1;
		p_Mp1->sys.core[0].dcache.write_hits=1;
		p_Mp1->sys.core[0].dcache.read_misses=0;
		p_Mp1->sys.core[0].dcache.write_misses=0;
		p_Mp1->sys.core[0].dcache.replacements=1;
		p_Mp1->sys.core[0].dcache.write_backs=1;
		p_Mp1->sys.core[0].dcache.miss_buffer_access=0;
		p_Mp1->sys.core[0].dcache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_hits=1;
		p_Mp1->sys.core[0].dcache.wbb_writes=1;
		p_Mp1->sys.core[0].dcache.wbb_reads=1;
		}
		//system.core?.BTB
		p_Mp1->sys.core[0].BTB.BTB_config[0]=(int)btb_tech.unit_scap;
		p_Mp1->sys.core[0].BTB.BTB_config[1]=btb_tech.line_size;
		p_Mp1->sys.core[0].BTB.BTB_config[2]=btb_tech.assoc;
		p_Mp1->sys.core[0].BTB.BTB_config[3]=btb_tech.num_banks;
		p_Mp1->sys.core[0].BTB.BTB_config[4]=(int)btb_tech.throughput;
		p_Mp1->sys.core[0].BTB.BTB_config[5]=(int)btb_tech.latency;
		p_Mp1->sys.core[0].BTB.total_accesses=2;
		p_Mp1->sys.core[0].BTB.read_accesses=1;
		p_Mp1->sys.core[0].BTB.write_accesses=1;
		p_Mp1->sys.core[0].BTB.total_hits=1;
		p_Mp1->sys.core[0].BTB.total_misses=0;
		p_Mp1->sys.core[0].BTB.read_hits=1;
		p_Mp1->sys.core[0].BTB.write_hits=1;
		p_Mp1->sys.core[0].BTB.read_misses=0;
		p_Mp1->sys.core[0].BTB.write_misses=0;
		p_Mp1->sys.core[0].BTB.replacements=1;
	//system_L1directory
	for(i=0; i < device_tech.number_L1dir; i++){
	p_Mp1->sys.L1Directory[i].Dir_config[0]=(int)cache_l1dir_tech.unit_scap.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[1]=cache_l1dir_tech.line_size.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[2]=cache_l1dir_tech.assoc.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[3]=cache_l1dir_tech.num_banks.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[4]=(int)cache_l1dir_tech.throughput.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[5]=(int)cache_l1dir_tech.latency.at(i);
	//p_Mp1->sys.L1Directory[i].Dir_config[6]=(int)cache_l1dir_tech.output_width.at(i);
	//p_Mp1->sys.L1Directory[i].Dir_config[7]=(int)cache_l1dir_tech.cache_policy.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[0]=cache_l1dir_tech.miss_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[1]=cache_l1dir_tech.fill_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[2]=cache_l1dir_tech.prefetch_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[3]=cache_l1dir_tech.wbb_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].clockrate = (int)(cache_l1dir_tech.op_freq/1000000); // Mc unit is MHz;
	p_Mp1->sys.L1Directory[i].ports[20]=1;
	p_Mp1->sys.L1Directory[i].device_type=cache_l1dir_tech.device_type;
	p_Mp1->sys.L1Directory[i].Directory_type=cache_l1dir_tech.directory_type;
	strcpy(p_Mp1->sys.L1Directory[i].threeD_stack,"N");
	p_Mp1->sys.L1Directory[i].total_accesses=2;
	p_Mp1->sys.L1Directory[i].read_accesses=1;
	p_Mp1->sys.L1Directory[i].write_accesses=1;
	p_Mp1->sys.L1Directory[i].duty_cycle =1;

	}
	//system_L2directory
	for(i=0; i < device_tech.number_L2dir; i++){
	p_Mp1->sys.L2Directory[i].Dir_config[0]=(int)cache_l2dir_tech.unit_scap.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[1]=cache_l2dir_tech.line_size.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[2]=cache_l2dir_tech.assoc.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[3]=cache_l2dir_tech.num_banks.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[4]=(int)cache_l2dir_tech.throughput.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[5]=(int)cache_l2dir_tech.latency.at(i);
	//p_Mp1->sys.L2Directory[i].Dir_config[6]=(int)cache_l2dir_tech.output_width.at(i);
	//p_Mp1->sys.L2Directory[i].Dir_config[7]=(int)cache_l2dir_tech.cache_policy.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[0]=cache_l2dir_tech.miss_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[1]=cache_l2dir_tech.fill_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[2]=cache_l2dir_tech.prefetch_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[3]=cache_l2dir_tech.wbb_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].clockrate = (int)(cache_l2dir_tech.op_freq/1000000); // Mc unit is MHz;
	p_Mp1->sys.L2Directory[i].ports[20]=1;
	p_Mp1->sys.L2Directory[i].device_type=cache_l2dir_tech.device_type;
	strcpy(p_Mp1->sys.L2Directory[i].threeD_stack,"N");
	p_Mp1->sys.L2Directory[i].total_accesses=2;
	p_Mp1->sys.L2Directory[i].read_accesses=1;
	p_Mp1->sys.L2Directory[i].write_accesses=1;
	p_Mp1->sys.L2Directory[i].duty_cycle =1;

	}
		//system_L2
	    for(i=0; i < device_tech.number_L2; i++){
		p_Mp1->sys.L2[i].L2_config[0]=(int)cache_l2_tech.unit_scap.at(i);
		//std::cout << "l2 scap["<<i<<"] = " << cache_l2_tech.unit_scap.at(i) << std::endl; 
		p_Mp1->sys.L2[i].L2_config[1]=cache_l2_tech.line_size.at(i);
		//std::cout << "l2 linezise["<<i<<"] = " << cache_l2_tech.line_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[2]=cache_l2_tech.assoc.at(i);
		//std::cout << "l2 assoc["<<i<<"] = " << cache_l2_tech.assoc.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[3]=cache_l2_tech.num_banks.at(i);
		//std::cout << "l2 numbanks["<<i<<"] = " << cache_l2_tech.num_banks.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[4]=(int)cache_l2_tech.throughput.at(i);
		//std::cout << "l2 throughput["<<i<<"] = " << cache_l2_tech.throughput.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[5]=(int)cache_l2_tech.latency.at(i);
		//std::cout << "l2 latency["<<i<<"] = " << cache_l2_tech.latency.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[6]=(int)cache_l2_tech.output_width.at(i);
		p_Mp1->sys.L2[i].L2_config[7]=(int)cache_l2_tech.cache_policy.at(i);
		p_Mp1->sys.L2[i].clockrate=(int)(cache_l2_tech.op_freq/1000000); // Mc unit is MHz;;
		p_Mp1->sys.L2[i].ports[20]=1;
		p_Mp1->sys.L2[i].device_type=cache_l2_tech.device_type;
		strcpy(p_Mp1->sys.L2[i].threeD_stack,"N");
		p_Mp1->sys.L2[i].buffer_sizes[0]=cache_l2_tech.miss_buf_size.at(i);
		//std::cout << "l2 missbuf["<<i<<"] = " << cache_l2_tech.miss_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[1]=cache_l2_tech.fill_buf_size.at(i);
		//std::cout << "l2 fillbuf["<<i<<"] = " << cache_l2_tech.fill_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[2]=cache_l2_tech.prefetch_buf_size.at(i);
		//std::cout << "l2 prefetchbuf["<<i<<"] = " << cache_l2_tech.prefetch_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[3]=cache_l2_tech.wbb_buf_size.at(i);
		//std::cout << "l2 wbbbuf["<<i<<"] = " << cache_l2_tech.wbb_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].total_accesses=2;
		p_Mp1->sys.L2[i].read_accesses=1;
		p_Mp1->sys.L2[i].write_accesses=1;
		p_Mp1->sys.L2[i].total_hits=1;
		p_Mp1->sys.L2[i].total_misses=0;
		p_Mp1->sys.L2[i].read_hits=1;
		p_Mp1->sys.L2[i].write_hits=1;
		p_Mp1->sys.L2[i].read_misses=0;
		p_Mp1->sys.L2[i].write_misses=0;
		p_Mp1->sys.L2[i].replacements=1;
		p_Mp1->sys.L2[i].write_backs=1;
		p_Mp1->sys.L2[i].miss_buffer_accesses=1;
		p_Mp1->sys.L2[i].fill_buffer_accesses=1;
		p_Mp1->sys.L2[i].prefetch_buffer_accesses=1;
		p_Mp1->sys.L2[i].prefetch_buffer_writes=1;
		p_Mp1->sys.L2[i].prefetch_buffer_reads=1;
		p_Mp1->sys.L2[i].prefetch_buffer_hits=1;
		p_Mp1->sys.L2[i].wbb_writes=1;
		p_Mp1->sys.L2[i].wbb_reads=1;
		p_Mp1->sys.L2[i].duty_cycle =1;
		p_Mp1->sys.L2[i].merged_dir=false;
		p_Mp1->sys.L2[i].homenode_read_accesses =1;
		p_Mp1->sys.L2[i].homenode_write_accesses=1;
		p_Mp1->sys.L2[i].homenode_read_hits=1;
		p_Mp1->sys.L2[i].homenode_write_hits=1;
		p_Mp1->sys.L2[i].homenode_read_misses=1;
		p_Mp1->sys.L2[i].homenode_write_misses=1;
		p_Mp1->sys.L2[i].dir_duty_cycle=1;
	    }
		//system_L3
	    for(i=0; i < device_tech.number_L3; i++){	
		p_Mp1->sys.L3[i].L3_config[0]=(int)cache_l3_tech.unit_scap.at(i);
		p_Mp1->sys.L3[i].L3_config[1]=cache_l3_tech.line_size.at(i);
		p_Mp1->sys.L3[i].L3_config[2]=cache_l3_tech.assoc.at(i);
		p_Mp1->sys.L3[i].L3_config[3]=cache_l3_tech.num_banks.at(i);
		p_Mp1->sys.L3[i].L3_config[4]=(int)cache_l3_tech.throughput.at(i);
		p_Mp1->sys.L3[i].L3_config[5]=(int)cache_l3_tech.latency.at(i);
		p_Mp1->sys.L3[i].L3_config[6]=(int)cache_l3_tech.output_width.at(i);
		p_Mp1->sys.L3[i].L3_config[7]=(int)cache_l3_tech.cache_policy.at(i);
		p_Mp1->sys.L3[i].clockrate=(int)(cache_l3_tech.op_freq/1000000); // Mc unit is MHz;;
		p_Mp1->sys.L3[i].ports[20]=1;
		p_Mp1->sys.L3[i].device_type=cache_l3_tech.device_type;
		strcpy(p_Mp1->sys.L3[i].threeD_stack,"N");
		p_Mp1->sys.L3[i].buffer_sizes[0]=cache_l3_tech.miss_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[1]=cache_l3_tech.fill_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[2]=cache_l3_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[3]=cache_l3_tech.wbb_buf_size.at(i);
		p_Mp1->sys.L3[i].total_accesses=2;
		p_Mp1->sys.L3[i].read_accesses=1;
		p_Mp1->sys.L3[i].write_accesses=1;
		p_Mp1->sys.L3[i].total_hits=1;
		p_Mp1->sys.L3[i].total_misses=0;
		p_Mp1->sys.L3[i].read_hits=1;
		p_Mp1->sys.L3[i].write_hits=1;
		p_Mp1->sys.L3[i].read_misses=0;
		p_Mp1->sys.L3[i].write_misses=0;
		p_Mp1->sys.L3[i].replacements=1;
		p_Mp1->sys.L3[i].write_backs=1;
		p_Mp1->sys.L3[i].miss_buffer_accesses=1;
		p_Mp1->sys.L3[i].fill_buffer_accesses=1;
		p_Mp1->sys.L3[i].prefetch_buffer_accesses=1;
		p_Mp1->sys.L3[i].prefetch_buffer_writes=1;
		p_Mp1->sys.L3[i].prefetch_buffer_reads=1;
		p_Mp1->sys.L3[i].prefetch_buffer_hits=1;
		p_Mp1->sys.L3[i].wbb_writes=1;
		p_Mp1->sys.L3[i].wbb_reads=1;
		p_Mp1->sys.L3[i].duty_cycle =1;
		p_Mp1->sys.L3[i].merged_dir=false;
		p_Mp1->sys.L3[i].homenode_read_accesses =1;
		p_Mp1->sys.L3[i].homenode_write_accesses=1;
		p_Mp1->sys.L3[i].homenode_read_hits=1;
		p_Mp1->sys.L3[i].homenode_write_hits=1;
		p_Mp1->sys.L3[i].homenode_read_misses=1;
		p_Mp1->sys.L3[i].homenode_write_misses=1;
		p_Mp1->sys.L3[i].dir_duty_cycle=1;
	    }
	//system_NoC
	p_Mp1->sys.NoC[0].clockrate=(int)(router_tech.clockrate/1000000);  // Mc unit is MHz;
	if (router_tech.topology == TWODMESH)
	    strcpy(p_Mp1->sys.NoC[0].topology,"2Dmesh");
	else if (router_tech.topology == RING)
	    strcpy(p_Mp1->sys.NoC[0].topology,"ring");
	else if (router_tech.topology == CROSSBAR)
	    strcpy(p_Mp1->sys.NoC[0].topology,"crossbar");
	p_Mp1->sys.NoC[0].type=true;  	/*1 NoC, O bus*/
	p_Mp1->sys.NoC[0].chip_coverage=1;
	p_Mp1->sys.NoC[0].has_global_link = (bool)router_tech.has_global_link;
	p_Mp1->sys.NoC[0].link_length = router_tech.link_length;
	p_Mp1->sys.NoC[0].horizontal_nodes=router_tech.horizontal_nodes;
	p_Mp1->sys.NoC[0].vertical_nodes=router_tech.vertical_nodes;
	p_Mp1->sys.NoC[0].input_ports=router_tech.input_ports;
	p_Mp1->sys.NoC[0].output_ports=router_tech.output_ports;
	p_Mp1->sys.NoC[0].virtual_channel_per_port=router_tech.virtual_channel_per_port;
	p_Mp1->sys.NoC[0].flit_bits=router_tech.flit_bits;
	p_Mp1->sys.NoC[0].input_buffer_entries_per_vc=router_tech.input_buffer_entries_per_vc;
	p_Mp1->sys.NoC[0].total_accesses=1;
	p_Mp1->sys.NoC[0].duty_cycle=0.6;
	p_Mp1->sys.NoC[0].route_over_perc = 0.5;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[0]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[1]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[2]=0;
	p_Mp1->sys.NoC[0].number_of_crossbars=1;
	p_Mp1->sys.NoC[0].dual_pump=0; //0 single pump; 1 dual pump
	strcpy(p_Mp1->sys.NoC[0].crossbar_type,"matrix");
	strcpy(p_Mp1->sys.NoC[0].crosspoint_type,"tri");
		//system.NoC?.xbar0;
		p_Mp1->sys.NoC[0].xbar0.number_of_inputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.number_of_outputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.flit_bits=router_tech.flit_bits;
		p_Mp1->sys.NoC[0].xbar0.input_buffer_entries_per_port=1;
		p_Mp1->sys.NoC[0].xbar0.ports_of_input_buffer[20]=1;
		p_Mp1->sys.NoC[0].xbar0.crossbar_accesses=521;
	//system_mem
	p_Mp1->sys.mem.mem_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.mem.device_clock=200; //MHz
	p_Mp1->sys.mem.peak_transfer_rate = mc_tech.memory_peak_transfer_rate;
	p_Mp1->sys.mem.capacity_per_channel=4096; //MB
	p_Mp1->sys.mem.number_ranks = mc_tech.memory_number_ranks;
	p_Mp1->sys.mem.num_banks_of_DRAM_chip=8;
	p_Mp1->sys.mem.Block_width_of_DRAM_chip=64; //B
	p_Mp1->sys.mem.output_width_of_DRAM_chip=8;
	p_Mp1->sys.mem.page_size_of_DRAM_chip=8;
	p_Mp1->sys.mem.burstlength_of_DRAM_chip=8;
	p_Mp1->sys.mem.internal_prefetch_of_DRAM_chip=4;
	p_Mp1->sys.mem.memory_accesses=2;
	p_Mp1->sys.mem.memory_reads=1;
	p_Mp1->sys.mem.memory_writes=1;
	//system_mc
	p_Mp1->sys.mc.mc_clock = (int)(mc_tech.mc_clock/1000000);  // Mc unit is MHz;
	p_Mp1->sys.mc.llc_line_length = mc_tech.llc_line_length;
	p_Mp1->sys.mc.number_mcs=2;
	p_Mp1->sys.mc.peak_transfer_rate =mc_tech.memory_peak_transfer_rate;
	p_Mp1->sys.mc.number_ranks=mc_tech.memory_number_ranks;
	p_Mp1->sys.mc.memory_channels_per_mc = mc_tech.memory_channels_per_mc;
	p_Mp1->sys.mc.req_window_size_per_channel = mc_tech.req_window_size_per_channel;
	p_Mp1->sys.mc.IO_buffer_size_per_channel = mc_tech.IO_buffer_size_per_channel;
	p_Mp1->sys.mc.databus_width = mc_tech.databus_width;
	p_Mp1->sys.mc.addressbus_width = mc_tech.addressbus_width;
	p_Mp1->sys.mc.memory_accesses=2;
	p_Mp1->sys.mc.memory_reads=1;
	p_Mp1->sys.mc.memory_writes=1;
	p_Mp1->sys.mc.LVDS=true;
	p_Mp1->sys.mc.type=1;
	//system_niu
	p_Mp1->sys.niu.clockrate =1;
	p_Mp1->sys.niu.number_units=1;
	p_Mp1->sys.niu.type = 1;
	p_Mp1->sys.niu.duty_cycle =1;
	p_Mp1->sys.niu.total_load_perc=1;
	//system_pcie
	p_Mp1->sys.pcie.clockrate =1;
	p_Mp1->sys.pcie.number_units=1;
	p_Mp1->sys.pcie.num_channels=1;
	p_Mp1->sys.pcie.type = 1;
	p_Mp1->sys.pcie.withPHY = false;
	p_Mp1->sys.pcie.duty_cycle =1;
	p_Mp1->sys.pcie.total_load_perc=1;
	//system_flash_controller
	p_Mp1->sys.flashc.mc_clock =1;
	p_Mp1->sys.flashc.number_mcs=1;
	p_Mp1->sys.flashc.peak_transfer_rate =1;
	p_Mp1->sys.flashc.memory_channels_per_mc=1;
	p_Mp1->sys.flashc.number_ranks=1;
	p_Mp1->sys.flashc.req_window_size_per_channel=1;
	p_Mp1->sys.flashc.IO_buffer_size_per_channel=1;
	p_Mp1->sys.flashc.databus_width=1;
	p_Mp1->sys.flashc.addressbus_width=1;
	p_Mp1->sys.flashc.memory_accesses=1;
	p_Mp1->sys.flashc.memory_reads=1;
	p_Mp1->sys.flashc.memory_writes=1;
	p_Mp1->sys.flashc.LVDS=true;
	p_Mp1->sys.flashc.withPHY = false;
	p_Mp1->sys.flashc.type =1;
	p_Mp1->sys.flashc.duty_cycle =1;
	p_Mp1->sys.flashc.total_load_perc=1;
}
#endif //McPAT07_H


#ifdef McPAT06_H
/***************************************************************
* Pass tech params to McPAT06	 			       *	
* McPAT06 interface				               * 
****************************************************************/
void Power::McPAT06Setup()
{
        unsigned int i;

   //All number_of_* at the level of 'system' 03/21/2009
	p_Mp1->sys.number_of_cores=1;
	p_Mp1->sys.number_of_L1Directories=device_tech.number_L1dir;
	p_Mp1->sys.number_of_L2Directories=device_tech.number_L2dir;
	p_Mp1->sys.number_of_L2s = device_tech.number_L2;
	p_Mp1->sys.number_of_L3s=device_tech.number_L3;
	p_Mp1->sys.number_of_NoCs = core_tech.core_number_of_NoCs;
	// All params at the level of 'system'

	
	p_Mp1->sys.homogeneous_L1Directories=0;
	p_Mp1->sys.homogeneous_L2Directories=0;
	p_Mp1->sys.homogeneous_NoCs=1;
	p_Mp1->sys.homogeneous_ccs=1;
	p_Mp1->sys.homogeneous_cores=1;
	p_Mp1->sys.core_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.target_core_clockrate=2200;
	p_Mp1->sys.target_chip_area=200;
	p_Mp1->sys.temperature = core_tech.core_temperature;
	p_Mp1->sys.number_cache_levels=2;
	p_Mp1->sys.L1_property=0; 
	p_Mp1->sys.L2_property=3;
	p_Mp1->sys.homogeneous_L2s=0;  //always set hetero so we can create multiple copies in McPAT
	p_Mp1->sys.L3_property=2;
	p_Mp1->sys.homogeneous_L3s=0;
	p_Mp1->sys.Max_area_deviation=10;
	p_Mp1->sys.Max_power_deviation=50;
	p_Mp1->sys.device_type=0;
	p_Mp1->sys.opt_dynamic_power=1;
	p_Mp1->sys.opt_lakage_power=0;
	p_Mp1->sys.opt_clockrate=0;
	p_Mp1->sys.opt_area=0;
	p_Mp1->sys.interconnect_projection_type=0;
	p_Mp1->sys.virtual_memory_page_size = core_tech.core_virtual_memory_page_size;
		p_Mp1->sys.core[0].clock_rate=(int)(device_tech.clockRate/1000000); // Mc unit is MHz
		p_Mp1->sys.core[0].machine_bits = core_tech.machine_bits;
		p_Mp1->sys.core[0].virtual_address_width = core_tech.core_virtual_address_width;
		p_Mp1->sys.core[0].physical_address_width = core_tech.core_physical_address_width;
		p_Mp1->sys.core[0].instruction_length = core_tech.core_instruction_length;
		p_Mp1->sys.core[0].opcode_width = core_tech.core_opcode_width;
		p_Mp1->sys.core[0].machine_type = device_tech.machineType;
		p_Mp1->sys.core[0].internal_datapath_width=64;
		p_Mp1->sys.core[0].number_hardware_threads = core_tech.core_number_hardware_threads;
		p_Mp1->sys.core[0].fetch_width = core_tech.core_fetch_width;
		p_Mp1->sys.core[0].number_instruction_fetch_ports=core_tech.core_number_instruction_fetch_ports;
		p_Mp1->sys.core[0].decode_width = core_tech.core_decode_width;
		p_Mp1->sys.core[0].issue_width = core_tech.core_issue_width;
		p_Mp1->sys.core[0].commit_width = core_tech.core_commit_width;
		p_Mp1->sys.core[0].pipelines_per_core[0]=1;
		p_Mp1->sys.core[0].pipeline_depth[0] = core_tech.core_int_pipeline_depth;
		strcpy(p_Mp1->sys.core[0].FPU,"1");
		strcpy(p_Mp1->sys.core[0].divider_multiplier,"1");
		p_Mp1->sys.core[0].ALU_per_core = core_tech.ALU_per_core;
		p_Mp1->sys.core[0].FPU_per_core = core_tech.FPU_per_core;
		p_Mp1->sys.core[0].instruction_buffer_size = core_tech.core_instruction_buffer_size;
		p_Mp1->sys.core[0].decoded_stream_buffer_size=16;		
		//strcpy(sys.core[i].instruction_window_scheme,"default");
		p_Mp1->sys.core[0].instruction_window_scheme=0;
		p_Mp1->sys.core[0].instruction_window_size = core_tech.core_instruction_window_size;
		p_Mp1->sys.core[0].ROB_size = core_tech.core_ROB_size;
		p_Mp1->sys.core[0].archi_Regs_IRF_size = core_tech.archi_Regs_IRF_size;
		p_Mp1->sys.core[0].archi_Regs_FRF_size = core_tech.archi_Regs_FRF_size;
		p_Mp1->sys.core[0].phy_Regs_IRF_size = core_tech.core_phy_Regs_IRF_size;
		p_Mp1->sys.core[0].phy_Regs_FRF_size = core_tech.core_phy_Regs_FRF_size;
		//strcpy(sys.core[i].rename_scheme,"default");
		p_Mp1->sys.core[0].rename_scheme=0;
		p_Mp1->sys.core[0].register_windows_size = core_tech.core_register_windows_size;
		strcpy(p_Mp1->sys.core[0].LSU_order,"inorder");
		p_Mp1->sys.core[0].store_buffer_size = core_tech.core_store_buffer_size;
		p_Mp1->sys.core[0].load_buffer_size = core_tech.core_load_buffer_size;
		p_Mp1->sys.core[0].memory_ports = core_tech.core_memory_ports;
		strcpy(p_Mp1->sys.core[0].Dcache_dual_pump,"N");
		p_Mp1->sys.core[0].RAS_size = core_tech.core_RAS_size;
		//all stats at the level of p_Mp1->system.core(0-n)
		p_Mp1->sys.core[0].total_instructions=2;
		p_Mp1->sys.core[0].int_instructions=2;
		p_Mp1->sys.core[0].fp_instructions=2;
		p_Mp1->sys.core[0].branch_instructions=2;
		p_Mp1->sys.core[0].branch_mispredictions=2;
		p_Mp1->sys.core[0].committed_instructions=2;
		p_Mp1->sys.core[0].load_instructions=2;
		p_Mp1->sys.core[0].store_instructions=2;
		p_Mp1->sys.core[0].total_cycles=1;
		p_Mp1->sys.core[0].idle_cycles=0;
		p_Mp1->sys.core[0].busy_cycles=1;
		p_Mp1->sys.core[0].instruction_buffer_reads=2;
		p_Mp1->sys.core[0].instruction_buffer_write=2;
		p_Mp1->sys.core[0].ROB_reads=2;
		p_Mp1->sys.core[0].ROB_writes=2;
		p_Mp1->sys.core[0].rename_accesses=2;
		p_Mp1->sys.core[0].inst_window_reads=2;
		p_Mp1->sys.core[0].inst_window_writes=2;
		p_Mp1->sys.core[0].inst_window_wakeup_accesses=2;
		p_Mp1->sys.core[0].inst_window_selections=2;
		p_Mp1->sys.core[0].archi_int_regfile_reads=2;
		p_Mp1->sys.core[0].archi_float_regfile_reads=2;
		p_Mp1->sys.core[0].phy_int_regfile_reads=2;
		p_Mp1->sys.core[0].phy_float_regfile_reads=2;
		p_Mp1->sys.core[0].windowed_reg_accesses=2;
		p_Mp1->sys.core[0].windowed_reg_transports=2;
		p_Mp1->sys.core[0].function_calls=2;
		p_Mp1->sys.core[0].ialu_access=1;
		p_Mp1->sys.core[0].fpu_access=1;
		p_Mp1->sys.core[0].bypassbus_access=1;
		p_Mp1->sys.core[0].load_buffer_reads=1;
		p_Mp1->sys.core[0].load_buffer_writes=1;
		p_Mp1->sys.core[0].load_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_reads=1;
		p_Mp1->sys.core[0].store_buffer_writes=1;
		p_Mp1->sys.core[0].store_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_forwards=1;
		p_Mp1->sys.core[0].main_memory_access=6;
		p_Mp1->sys.core[0].main_memory_read=3;
		p_Mp1->sys.core[0].main_memory_write=3;
		//p_Mp1->system.core?.predictor
		p_Mp1->sys.core[0].predictor.prediction_width = bpred_tech.prediction_width;
		strcpy(p_Mp1->sys.core[0].predictor.prediction_scheme,"tournament");
		p_Mp1->sys.core[0].predictor.predictor_size=2;
		p_Mp1->sys.core[0].predictor.predictor_entries=1024;
		p_Mp1->sys.core[0].predictor.local_predictor_entries = bpred_tech.local_predictor_entries;
		p_Mp1->sys.core[0].predictor.local_predictor_size = bpred_tech.local_predictor_size;
		p_Mp1->sys.core[0].predictor.global_predictor_entries = bpred_tech.global_predictor_entries;
		p_Mp1->sys.core[0].predictor.global_predictor_bits = bpred_tech.global_predictor_bits;
		p_Mp1->sys.core[0].predictor.chooser_predictor_entries = bpred_tech.chooser_predictor_entries;
		p_Mp1->sys.core[0].predictor.chooser_predictor_bits = bpred_tech.chooser_predictor_bits;
		p_Mp1->sys.core[0].predictor.predictor_accesses=263886;
		//p_Mp1->system.core?.itlb
		p_Mp1->sys.core[0].itlb.number_entries=cache_itlb_tech.number_entries;
		p_Mp1->sys.core[0].itlb.total_hits=1;
		p_Mp1->sys.core[0].itlb.total_accesses=1;
		p_Mp1->sys.core[0].itlb.total_misses=0;
		//p_Mp1->system.core?.icache
		for(i=0; i < device_tech.number_il1; i++){
		p_Mp1->sys.core[0].icache.icache_config[0]=(int)cache_il1_tech.unit_scap.at(i);
		p_Mp1->sys.core[0].icache.icache_config[1]=cache_il1_tech.line_size.at(i);
		p_Mp1->sys.core[0].icache.icache_config[2]=cache_il1_tech.assoc.at(i);
		p_Mp1->sys.core[0].icache.icache_config[3]=cache_il1_tech.num_banks.at(i);
		p_Mp1->sys.core[0].icache.icache_config[4]=(int)cache_il1_tech.throughput.at(i);
		p_Mp1->sys.core[0].icache.icache_config[5]=(int)cache_il1_tech.latency.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[0]=cache_il1_tech.miss_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[1]=cache_il1_tech.fill_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[2]=cache_il1_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.core[0].icache.buffer_sizes[3]=cache_il1_tech.wbb_buf_size.at(i);
		p_Mp1->sys.core[0].icache.total_accesses=1;
		p_Mp1->sys.core[0].icache.read_accesses=1;
		p_Mp1->sys.core[0].icache.read_misses=1;
		p_Mp1->sys.core[0].icache.replacements=0;
		p_Mp1->sys.core[0].icache.read_hits=1;
		p_Mp1->sys.core[0].icache.total_hits=1;
		p_Mp1->sys.core[0].icache.total_misses=1;
		p_Mp1->sys.core[0].icache.miss_buffer_access=1;
		p_Mp1->sys.core[0].icache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_hits=1;
		}
		//system.core?.dtlb
		for(i=0; i < device_tech.number_dtlb; i++){
		p_Mp1->sys.core[0].dtlb.number_entries=cache_dtlb_tech.number_entries;
		p_Mp1->sys.core[0].dtlb.total_accesses=2;
		p_Mp1->sys.core[0].dtlb.read_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_hits=1;
		p_Mp1->sys.core[0].dtlb.read_hits=1;
		p_Mp1->sys.core[0].dtlb.read_misses=0;
		p_Mp1->sys.core[0].dtlb.write_misses=0;
		p_Mp1->sys.core[0].dtlb.total_hits=1;
		p_Mp1->sys.core[0].dtlb.total_misses=1;
		}
		//system.core?.dcache
		for(i=0; i < device_tech.number_dl1; i++){
		p_Mp1->sys.core[0].dcache.dcache_config[0]=(int)cache_dl1_tech.unit_scap.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[1]=cache_dl1_tech.line_size.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[2]=cache_dl1_tech.assoc.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[3]=cache_dl1_tech.num_banks.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[4]=(int)cache_dl1_tech.throughput.at(i);
		p_Mp1->sys.core[0].dcache.dcache_config[5]=(int)cache_dl1_tech.latency.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[0]=cache_dl1_tech.miss_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[1]=cache_dl1_tech.fill_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[2]=cache_dl1_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.buffer_sizes[3]=cache_dl1_tech.wbb_buf_size.at(i);
		p_Mp1->sys.core[0].dcache.total_accesses=2;
		p_Mp1->sys.core[0].dcache.read_accesses=1;
		p_Mp1->sys.core[0].dcache.write_accesses=1;
		p_Mp1->sys.core[0].dcache.total_hits=1;
		p_Mp1->sys.core[0].dcache.total_misses=0;
		p_Mp1->sys.core[0].dcache.read_hits=1;
		p_Mp1->sys.core[0].dcache.write_hits=1;
		p_Mp1->sys.core[0].dcache.read_misses=0;
		p_Mp1->sys.core[0].dcache.write_misses=0;
		p_Mp1->sys.core[0].dcache.replacements=1;
		p_Mp1->sys.core[0].dcache.write_backs=1;
		p_Mp1->sys.core[0].dcache.miss_buffer_access=0;
		p_Mp1->sys.core[0].dcache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_hits=1;
		p_Mp1->sys.core[0].dcache.wbb_writes=1;
		p_Mp1->sys.core[0].dcache.wbb_reads=1;
		}
		//system.core?.BTB
		p_Mp1->sys.core[0].BTB.BTB_config[0]=(int)btb_tech.unit_scap;
		p_Mp1->sys.core[0].BTB.BTB_config[1]=btb_tech.line_size;
		p_Mp1->sys.core[0].BTB.BTB_config[2]=btb_tech.assoc;
		p_Mp1->sys.core[0].BTB.BTB_config[3]=btb_tech.num_banks;
		p_Mp1->sys.core[0].BTB.BTB_config[4]=(int)btb_tech.throughput;
		p_Mp1->sys.core[0].BTB.BTB_config[5]=(int)btb_tech.latency;
		p_Mp1->sys.core[0].BTB.total_accesses=2;
		p_Mp1->sys.core[0].BTB.read_accesses=1;
		p_Mp1->sys.core[0].BTB.write_accesses=1;
		p_Mp1->sys.core[0].BTB.total_hits=1;
		p_Mp1->sys.core[0].BTB.total_misses=0;
		p_Mp1->sys.core[0].BTB.read_hits=1;
		p_Mp1->sys.core[0].BTB.write_hits=1;
		p_Mp1->sys.core[0].BTB.read_misses=0;
		p_Mp1->sys.core[0].BTB.write_misses=0;
		p_Mp1->sys.core[0].BTB.replacements=1;
	//system_L1directory
	for(i=0; i < device_tech.number_L1dir; i++){
	p_Mp1->sys.L1Directory[i].Dir_config[0]=(int)cache_l1dir_tech.unit_scap.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[1]=cache_l1dir_tech.line_size.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[2]=cache_l1dir_tech.assoc.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[3]=cache_l1dir_tech.num_banks.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[4]=(int)cache_l1dir_tech.throughput.at(i);
	p_Mp1->sys.L1Directory[i].Dir_config[5]=(int)cache_l1dir_tech.latency.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[0]=cache_l1dir_tech.miss_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[1]=cache_l1dir_tech.fill_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[2]=cache_l1dir_tech.prefetch_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].buffer_sizes[3]=cache_l1dir_tech.wbb_buf_size.at(i);
	p_Mp1->sys.L1Directory[i].clockrate = (int)(cache_l1dir_tech.op_freq/1000000); // Mc unit is MHz;
	p_Mp1->sys.L1Directory[i].ports[20]=1;
	p_Mp1->sys.L1Directory[i].device_type=cache_l1dir_tech.device_type;
	p_Mp1->sys.L1Directory[i].Directory_type=cache_l1dir_tech.directory_type;
	strcpy(p_Mp1->sys.L1Directory[i].threeD_stack,"N");
	p_Mp1->sys.L1Directory[i].total_accesses=2;
	p_Mp1->sys.L1Directory[i].read_accesses=1;
	p_Mp1->sys.L1Directory[i].write_accesses=1;
	}
	//system_L2directory
	for(i=0; i < device_tech.number_L2dir; i++){
	p_Mp1->sys.L2Directory[i].Dir_config[0]=(int)cache_l2dir_tech.unit_scap.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[1]=cache_l2dir_tech.line_size.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[2]=cache_l2dir_tech.assoc.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[3]=cache_l2dir_tech.num_banks.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[4]=(int)cache_l2dir_tech.throughput.at(i);
	p_Mp1->sys.L2Directory[i].Dir_config[5]=(int)cache_l2dir_tech.latency.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[0]=cache_l2dir_tech.miss_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[1]=cache_l2dir_tech.fill_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[2]=cache_l2dir_tech.prefetch_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].buffer_sizes[3]=cache_l2dir_tech.wbb_buf_size.at(i);
	p_Mp1->sys.L2Directory[i].clockrate = (int)(cache_l2dir_tech.op_freq/1000000); // Mc unit is MHz;
	p_Mp1->sys.L2Directory[i].ports[20]=1;
	p_Mp1->sys.L2Directory[i].device_type=cache_l2dir_tech.device_type;
	strcpy(p_Mp1->sys.L2Directory[i].threeD_stack,"N");
	p_Mp1->sys.L2Directory[i].total_accesses=2;
	p_Mp1->sys.L2Directory[i].read_accesses=1;
	p_Mp1->sys.L2Directory[i].write_accesses=1;
	}
		//system_L2
	    for(i=0; i < device_tech.number_L2; i++){
		p_Mp1->sys.L2[i].L2_config[0]=(int)cache_l2_tech.unit_scap.at(i);
		//std::cout << "l2 scap["<<i<<"] = " << cache_l2_tech.unit_scap.at(i) << std::endl; 
		p_Mp1->sys.L2[i].L2_config[1]=cache_l2_tech.line_size.at(i);
		//std::cout << "l2 linezise["<<i<<"] = " << cache_l2_tech.line_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[2]=cache_l2_tech.assoc.at(i);
		//std::cout << "l2 assoc["<<i<<"] = " << cache_l2_tech.assoc.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[3]=cache_l2_tech.num_banks.at(i);
		//std::cout << "l2 numbanks["<<i<<"] = " << cache_l2_tech.num_banks.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[4]=(int)cache_l2_tech.throughput.at(i);
		//std::cout << "l2 throughput["<<i<<"] = " << cache_l2_tech.throughput.at(i) << std::endl;
		p_Mp1->sys.L2[i].L2_config[5]=(int)cache_l2_tech.latency.at(i);
		//std::cout << "l2 latency["<<i<<"] = " << cache_l2_tech.latency.at(i) << std::endl;
		p_Mp1->sys.L2[i].clockrate=(int)(cache_l2_tech.op_freq/1000000); // Mc unit is MHz;;
		p_Mp1->sys.L2[i].ports[20]=1;
		p_Mp1->sys.L2[i].device_type=cache_l2_tech.device_type;
		strcpy(p_Mp1->sys.L2[i].threeD_stack,"N");
		p_Mp1->sys.L2[i].buffer_sizes[0]=cache_l2_tech.miss_buf_size.at(i);
		//std::cout << "l2 missbuf["<<i<<"] = " << cache_l2_tech.miss_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[1]=cache_l2_tech.fill_buf_size.at(i);
		//std::cout << "l2 fillbuf["<<i<<"] = " << cache_l2_tech.fill_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[2]=cache_l2_tech.prefetch_buf_size.at(i);
		//std::cout << "l2 prefetchbuf["<<i<<"] = " << cache_l2_tech.prefetch_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].buffer_sizes[3]=cache_l2_tech.wbb_buf_size.at(i);
		//std::cout << "l2 wbbbuf["<<i<<"] = " << cache_l2_tech.wbb_buf_size.at(i) << std::endl;
		p_Mp1->sys.L2[i].total_accesses=2;
		p_Mp1->sys.L2[i].read_accesses=1;
		p_Mp1->sys.L2[i].write_accesses=1;
		p_Mp1->sys.L2[i].total_hits=1;
		p_Mp1->sys.L2[i].total_misses=0;
		p_Mp1->sys.L2[i].read_hits=1;
		p_Mp1->sys.L2[i].write_hits=1;
		p_Mp1->sys.L2[i].read_misses=0;
		p_Mp1->sys.L2[i].write_misses=0;
		p_Mp1->sys.L2[i].replacements=1;
		p_Mp1->sys.L2[i].write_backs=1;
		p_Mp1->sys.L2[i].miss_buffer_accesses=1;
		p_Mp1->sys.L2[i].fill_buffer_accesses=1;
		p_Mp1->sys.L2[i].prefetch_buffer_accesses=1;
		p_Mp1->sys.L2[i].prefetch_buffer_writes=1;
		p_Mp1->sys.L2[i].prefetch_buffer_reads=1;
		p_Mp1->sys.L2[i].prefetch_buffer_hits=1;
		p_Mp1->sys.L2[i].wbb_writes=1;
		p_Mp1->sys.L2[i].wbb_reads=1;
	    }
		//system_L3
	    for(i=0; i < device_tech.number_L3; i++){	
		p_Mp1->sys.L3[i].L3_config[0]=(int)cache_l3_tech.unit_scap.at(i);
		p_Mp1->sys.L3[i].L3_config[1]=cache_l3_tech.line_size.at(i);
		p_Mp1->sys.L3[i].L3_config[2]=cache_l3_tech.assoc.at(i);
		p_Mp1->sys.L3[i].L3_config[3]=cache_l3_tech.num_banks.at(i);
		p_Mp1->sys.L3[i].L3_config[4]=(int)cache_l3_tech.throughput.at(i);
		p_Mp1->sys.L3[i].L3_config[5]=(int)cache_l3_tech.latency.at(i);
		p_Mp1->sys.L3[i].clockrate=(int)(cache_l3_tech.op_freq/1000000); // Mc unit is MHz;;
		p_Mp1->sys.L3[i].ports[20]=1;
		p_Mp1->sys.L3[i].device_type=cache_l3_tech.device_type;
		strcpy(p_Mp1->sys.L3[i].threeD_stack,"N");
		p_Mp1->sys.L3[i].buffer_sizes[0]=cache_l3_tech.miss_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[1]=cache_l3_tech.fill_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[2]=cache_l3_tech.prefetch_buf_size.at(i);
		p_Mp1->sys.L3[i].buffer_sizes[3]=cache_l3_tech.wbb_buf_size.at(i);
		p_Mp1->sys.L3[i].total_accesses=2;
		p_Mp1->sys.L3[i].read_accesses=1;
		p_Mp1->sys.L3[i].write_accesses=1;
		p_Mp1->sys.L3[i].total_hits=1;
		p_Mp1->sys.L3[i].total_misses=0;
		p_Mp1->sys.L3[i].read_hits=1;
		p_Mp1->sys.L3[i].write_hits=1;
		p_Mp1->sys.L3[i].read_misses=0;
		p_Mp1->sys.L3[i].write_misses=0;
		p_Mp1->sys.L3[i].replacements=1;
		p_Mp1->sys.L3[i].write_backs=1;
		p_Mp1->sys.L3[i].miss_buffer_accesses=1;
		p_Mp1->sys.L3[i].fill_buffer_accesses=1;
		p_Mp1->sys.L3[i].prefetch_buffer_accesses=1;
		p_Mp1->sys.L3[i].prefetch_buffer_writes=1;
		p_Mp1->sys.L3[i].prefetch_buffer_reads=1;
		p_Mp1->sys.L3[i].prefetch_buffer_hits=1;
		p_Mp1->sys.L3[i].wbb_writes=1;
		p_Mp1->sys.L3[i].wbb_reads=1;
	    }
	//system_mem
	p_Mp1->sys.mem.mem_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.mem.device_clock=200; //MHz
	p_Mp1->sys.mem.peak_transfer_rate = mc_tech.memory_peak_transfer_rate;
	p_Mp1->sys.mem.capacity_per_channel=4096; //MB
	p_Mp1->sys.mem.number_ranks = mc_tech.memory_number_ranks;
	p_Mp1->sys.mem.num_banks_of_DRAM_chip=8;
	p_Mp1->sys.mem.Block_width_of_DRAM_chip=64; //B
	p_Mp1->sys.mem.output_width_of_DRAM_chip=8;
	p_Mp1->sys.mem.page_size_of_DRAM_chip=8;
	p_Mp1->sys.mem.burstlength_of_DRAM_chip=8;
	p_Mp1->sys.mem.internal_prefetch_of_DRAM_chip=4;
	p_Mp1->sys.mem.memory_accesses=2;
	p_Mp1->sys.mem.memory_reads=1;
	p_Mp1->sys.mem.memory_writes=1;
	//system_mc
	p_Mp1->sys.mc.mc_clock = (int)(mc_tech.mc_clock/1000000);  // Mc unit is MHz;
	p_Mp1->sys.mc.llc_line_length = mc_tech.llc_line_length;
	p_Mp1->sys.mc.number_mcs=4;
	p_Mp1->sys.mc.type=0;
	p_Mp1->sys.mc.memory_channels_per_mc = mc_tech.memory_channels_per_mc;
	p_Mp1->sys.mc.req_window_size_per_channel = mc_tech.req_window_size_per_channel;
	p_Mp1->sys.mc.IO_buffer_size_per_channel = mc_tech.IO_buffer_size_per_channel;
	p_Mp1->sys.mc.databus_width = mc_tech.databus_width;
	p_Mp1->sys.mc.addressbus_width = mc_tech.addressbus_width;
	p_Mp1->sys.mc.memory_accesses=2;
	p_Mp1->sys.mc.memory_reads=1;
	p_Mp1->sys.mc.memory_writes=1;
	//system_NoC
	p_Mp1->sys.NoC[0].clockrate=(int)(router_tech.clockrate/1000000);  // Mc unit is MHz;
	if (router_tech.topology == TWODMESH)
	    strcpy(p_Mp1->sys.NoC[0].topology,"2Dmesh");
	else if (router_tech.topology == RING)
	    strcpy(p_Mp1->sys.NoC[0].topology,"ring");
	else if (router_tech.topology == CROSSBAR)
	    strcpy(p_Mp1->sys.NoC[0].topology,"crossbar");	
	p_Mp1->sys.NoC[0].horizontal_nodes=router_tech.horizontal_nodes;
	p_Mp1->sys.NoC[0].vertical_nodes=router_tech.vertical_nodes;
	p_Mp1->sys.NoC[0].input_ports=router_tech.input_ports;
	p_Mp1->sys.NoC[0].output_ports=router_tech.output_ports;
	p_Mp1->sys.NoC[0].virtual_channel_per_port=router_tech.virtual_channel_per_port;
	p_Mp1->sys.NoC[0].flit_bits=router_tech.flit_bits;
	p_Mp1->sys.NoC[0].input_buffer_entries_per_vc=router_tech.input_buffer_entries_per_vc;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[0]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[1]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[2]=0;
	p_Mp1->sys.NoC[0].number_of_crossbars=1;
	p_Mp1->sys.NoC[0].dual_pump=0; //0 single pump; 1 dual pump
	strcpy(p_Mp1->sys.NoC[0].crossbar_type,"matrix");
	strcpy(p_Mp1->sys.NoC[0].crosspoint_type,"tri");
		//system.NoC?.xbar0;
		p_Mp1->sys.NoC[0].xbar0.number_of_inputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.number_of_outputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.flit_bits=router_tech.flit_bits;
		p_Mp1->sys.NoC[0].xbar0.input_buffer_entries_per_port=1;
		p_Mp1->sys.NoC[0].xbar0.ports_of_input_buffer[20]=1;
		p_Mp1->sys.NoC[0].xbar0.crossbar_accesses=521;

}
#endif //McPAT06_H

#ifdef McPAT05_H
/***************************************************************
* Pass tech params to McPAT05	 			       *	
* McPAT05 interface				               * 
****************************************************************/
void Power::McPAT05Setup()
{
   //All number_of_* at the level of 'system' 03/21/2009
	p_Mp1->sys.number_of_cores=1;
	p_Mp1->sys.number_of_L2s = device_tech.number_L2;
	p_Mp1->sys.number_of_L3s=1;
	p_Mp1->sys.number_of_NoCs = core_tech.core_number_of_NoCs;
	// All params at the level of 'system'
	p_Mp1->sys.homogeneous_cores=1;
	p_Mp1->sys.core_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.target_core_clockrate=3000;
	p_Mp1->sys.target_chip_area=200;
	p_Mp1->sys.temperature = core_tech.core_temperature;
	p_Mp1->sys.number_cache_levels=3;
	p_Mp1->sys.L1_property=0;
	p_Mp1->sys.L2_property=3;
	p_Mp1->sys.homogeneous_L2s=1;
	p_Mp1->sys.L3_property=2;
	p_Mp1->sys.homogeneous_L3s=1;
	p_Mp1->sys.Max_area_deviation=10;
	p_Mp1->sys.Max_power_deviation=50;
	p_Mp1->sys.device_type=0;
	p_Mp1->sys.opt_dynamic_power=1;
	p_Mp1->sys.opt_lakage_power=0;
	p_Mp1->sys.opt_clockrate=0;
	p_Mp1->sys.opt_area=0;
	p_Mp1->sys.interconnect_projection_type=0;
	p_Mp1->sys.virtual_memory_page_size = core_tech.core_virtual_memory_page_size;
		p_Mp1->sys.core[0].clock_rate=(int)(device_tech.clockRate/1000000); // Mc unit is MHz
		p_Mp1->sys.core[0].machine_bits = core_tech.machine_bits;
		p_Mp1->sys.core[0].virtual_address_width = core_tech.core_virtual_address_width;
		p_Mp1->sys.core[0].physical_address_width = core_tech.core_physical_address_width;
		p_Mp1->sys.core[0].instruction_length = core_tech.core_instruction_length;
		p_Mp1->sys.core[0].opcode_width = core_tech.core_opcode_width;
		p_Mp1->sys.core[0].machine_type = device_tech.machineType;
		p_Mp1->sys.core[0].internal_datapath_width=64;
		p_Mp1->sys.core[0].number_hardware_threads = core_tech.core_number_hardware_threads;
		p_Mp1->sys.core[0].fetch_width = core_tech.core_fetch_width;
		p_Mp1->sys.core[0].number_instruction_fetch_ports=1;
		p_Mp1->sys.core[0].decode_width = core_tech.core_decode_width;
		p_Mp1->sys.core[0].issue_width = core_tech.core_issue_width;
		p_Mp1->sys.core[0].commit_width = core_tech.core_commit_width;
		p_Mp1->sys.core[0].pipelines_per_core[0]=1;
		p_Mp1->sys.core[0].pipeline_depth[0] = core_tech.core_int_pipeline_depth;
		strcpy(p_Mp1->sys.core[0].FPU,"1");
		strcpy(p_Mp1->sys.core[0].divider_multiplier,"1");
		p_Mp1->sys.core[0].ALU_per_core = core_tech.ALU_per_core;
		p_Mp1->sys.core[0].FPU_per_core = core_tech.FPU_per_core;
		p_Mp1->sys.core[0].instruction_buffer_size = core_tech.core_instruction_buffer_size;
		p_Mp1->sys.core[0].decoded_stream_buffer_size=20;
		p_Mp1->sys.core[0].instruction_window_scheme=0;
		p_Mp1->sys.core[0].instruction_window_size = core_tech.core_instruction_window_size;
		p_Mp1->sys.core[0].ROB_size = core_tech.core_ROB_size;
		p_Mp1->sys.core[0].archi_Regs_IRF_size = core_tech.archi_Regs_IRF_size;
		p_Mp1->sys.core[0].archi_Regs_FRF_size = core_tech.archi_Regs_FRF_size;
		p_Mp1->sys.core[0].phy_Regs_IRF_size = core_tech.core_phy_Regs_IRF_size;
		p_Mp1->sys.core[0].phy_Regs_FRF_size = core_tech.core_phy_Regs_FRF_size;
		p_Mp1->sys.core[0].rename_scheme=0;
		p_Mp1->sys.core[0].register_windows_size = core_tech.core_register_windows_size;
		strcpy(p_Mp1->sys.core[0].LSU_order,"inorder");
		p_Mp1->sys.core[0].store_buffer_size = core_tech.core_store_buffer_size;
		p_Mp1->sys.core[0].load_buffer_size = core_tech.core_load_buffer_size;
		p_Mp1->sys.core[0].memory_ports = core_tech.core_memory_ports;
		strcpy(p_Mp1->sys.core[0].Dcache_dual_pump,"N");
		p_Mp1->sys.core[0].RAS_size = core_tech.core_RAS_size;
		//all stats at the level of p_Mp1->system.core(0-n)
		p_Mp1->sys.core[0].total_instructions=2;
		p_Mp1->sys.core[0].int_instructions=2;
		p_Mp1->sys.core[0].fp_instructions=2;
		p_Mp1->sys.core[0].branch_instructions=2;
		p_Mp1->sys.core[0].branch_mispredictions=2;
		p_Mp1->sys.core[0].commited_instructions=2;
		p_Mp1->sys.core[0].load_instructions=2;
		p_Mp1->sys.core[0].store_instructions=2;
		p_Mp1->sys.core[0].total_cycles=1;
		p_Mp1->sys.core[0].idle_cycles=0;
		p_Mp1->sys.core[0].busy_cycles=1;
		p_Mp1->sys.core[0].instruction_buffer_reads=2;
		p_Mp1->sys.core[0].instruction_buffer_write=2;
		p_Mp1->sys.core[0].ROB_reads=2;
		p_Mp1->sys.core[0].ROB_writes=2;
		p_Mp1->sys.core[0].rename_accesses=2;
		p_Mp1->sys.core[0].inst_window_reads=2;
		p_Mp1->sys.core[0].inst_window_writes=2;
		p_Mp1->sys.core[0].inst_window_wakeup_access=2;
		p_Mp1->sys.core[0].inst_window_selections=2;
		p_Mp1->sys.core[0].archi_int_regfile_reads=2;
		p_Mp1->sys.core[0].archi_float_regfile_reads=2;
		p_Mp1->sys.core[0].phy_int_regfile_reads=2;
		p_Mp1->sys.core[0].phy_float_regfile_reads=2;
		p_Mp1->sys.core[0].windowed_reg_accesses=2;
		p_Mp1->sys.core[0].windowed_reg_transports=2;
		p_Mp1->sys.core[0].function_calls=2;
		p_Mp1->sys.core[0].ialu_access=1;
		p_Mp1->sys.core[0].fpu_access=1;
		p_Mp1->sys.core[0].bypassbus_access=2;
		p_Mp1->sys.core[0].load_buffer_reads=1;
		p_Mp1->sys.core[0].load_buffer_writes=1;
		p_Mp1->sys.core[0].load_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_reads=1;
		p_Mp1->sys.core[0].store_buffer_writes=1;
		p_Mp1->sys.core[0].store_buffer_cams=1;
		p_Mp1->sys.core[0].store_buffer_forwards=1;
		p_Mp1->sys.core[0].main_memory_access=6;
		p_Mp1->sys.core[0].main_memory_read=3;
		p_Mp1->sys.core[0].main_memory_write=3;
		//p_Mp1->system.core?.predictor
		p_Mp1->sys.core[0].predictor.prediction_width = bpred_tech.prediction_width;
		strcpy(p_Mp1->sys.core[0].predictor.prediction_scheme,"tournament");
		p_Mp1->sys.core[0].predictor.predictor_size=2;
		p_Mp1->sys.core[0].predictor.predictor_entries=1024;
		p_Mp1->sys.core[0].predictor.local_predictor_entries = bpred_tech.local_predictor_entries;
		p_Mp1->sys.core[0].predictor.local_predictor_size = bpred_tech.local_predictor_size;
		p_Mp1->sys.core[0].predictor.global_predictor_entries = bpred_tech.global_predictor_entries;
		p_Mp1->sys.core[0].predictor.global_predictor_bits = bpred_tech.global_predictor_bits;
		p_Mp1->sys.core[0].predictor.chooser_predictor_entries = bpred_tech.chooser_predictor_entries;
		p_Mp1->sys.core[0].predictor.chooser_predictor_bits = bpred_tech.chooser_predictor_bits;
		p_Mp1->sys.core[0].predictor.predictor_accesses=263886;
		//p_Mp1->system.core?.itlb
		p_Mp1->sys.core[0].itlb.number_entries=cache_itlb_tech.number_entries;
		p_Mp1->sys.core[0].itlb.total_hits=2;
		p_Mp1->sys.core[0].itlb.total_accesses=2;
		p_Mp1->sys.core[0].itlb.total_misses=0;
		//p_Mp1->system.core?.icache
		p_Mp1->sys.core[0].icache.icache_config[0]=(int)cache_il1_tech.unit_scap.at(0);
		p_Mp1->sys.core[0].icache.icache_config[1]=cache_il1_tech.line_size.at(0);
		p_Mp1->sys.core[0].icache.icache_config[2]=cache_il1_tech.assoc.at(0);
		p_Mp1->sys.core[0].icache.icache_config[3]=cache_il1_tech.num_banks.at(0);
		p_Mp1->sys.core[0].icache.icache_config[4]=(int)cache_il1_tech.throughput.at(0);
		p_Mp1->sys.core[0].icache.icache_config[5]=(int)cache_il1_tech.latency.at(0);
		p_Mp1->sys.core[0].icache.buffer_sizes[0]=cache_il1_tech.miss_buf_size.at(0);
		p_Mp1->sys.core[0].icache.buffer_sizes[1]=cache_il1_tech.fill_buf_size.at(0);
		p_Mp1->sys.core[0].icache.buffer_sizes[2]=cache_il1_tech.prefetch_buf_size.at(0);
		p_Mp1->sys.core[0].icache.buffer_sizes[3]=cache_il1_tech.wbb_buf_size.at(0);
		p_Mp1->sys.core[0].icache.total_accesses=1;
		p_Mp1->sys.core[0].icache.read_accesses=1;
		p_Mp1->sys.core[0].icache.read_misses=1;
		p_Mp1->sys.core[0].icache.replacements=0;
		p_Mp1->sys.core[0].icache.read_hits=1;
		p_Mp1->sys.core[0].icache.total_hits=1;
		p_Mp1->sys.core[0].icache.total_misses=1;
		p_Mp1->sys.core[0].icache.miss_buffer_access=1;
		p_Mp1->sys.core[0].icache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].icache.prefetch_buffer_hits=1;
		//system.core?.dtlb
		p_Mp1->sys.core[0].dtlb.number_entries=cache_dtlb_tech.number_entries;
		p_Mp1->sys.core[0].dtlb.total_accesses=2;
		p_Mp1->sys.core[0].dtlb.read_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_accesses=1;
		p_Mp1->sys.core[0].dtlb.write_hits=1;
		p_Mp1->sys.core[0].dtlb.read_hits=1;
		p_Mp1->sys.core[0].dtlb.read_misses=0;
		p_Mp1->sys.core[0].dtlb.write_misses=0;
		p_Mp1->sys.core[0].dtlb.total_hits=1;
		p_Mp1->sys.core[0].dtlb.total_misses=1;
		//system.core?.dcache
		p_Mp1->sys.core[0].dcache.dcache_config[0]=(int)cache_dl1_tech.unit_scap.at(0);
		p_Mp1->sys.core[0].dcache.dcache_config[1]=cache_dl1_tech.line_size.at(0);
		p_Mp1->sys.core[0].dcache.dcache_config[2]=cache_dl1_tech.assoc.at(0);
		p_Mp1->sys.core[0].dcache.dcache_config[3]=cache_dl1_tech.num_banks.at(0);
		p_Mp1->sys.core[0].dcache.dcache_config[4]=(int)cache_dl1_tech.throughput.at(0);
		p_Mp1->sys.core[0].dcache.dcache_config[5]=(int)cache_dl1_tech.latency.at(0);
		p_Mp1->sys.core[0].dcache.buffer_sizes[0]=cache_dl1_tech.miss_buf_size.at(0);
		p_Mp1->sys.core[0].dcache.buffer_sizes[1]=cache_dl1_tech.fill_buf_size.at(0);
		p_Mp1->sys.core[0].dcache.buffer_sizes[2]=cache_dl1_tech.prefetch_buf_size.at(0);
		p_Mp1->sys.core[0].dcache.buffer_sizes[3]=cache_dl1_tech.wbb_buf_size.at(0);
		p_Mp1->sys.core[0].dcache.total_accesses=2;
		p_Mp1->sys.core[0].dcache.read_accesses=1;
		p_Mp1->sys.core[0].dcache.write_accesses=1;
		p_Mp1->sys.core[0].dcache.total_hits=1;
		p_Mp1->sys.core[0].dcache.total_misses=0;
		p_Mp1->sys.core[0].dcache.read_hits=1;
		p_Mp1->sys.core[0].dcache.write_hits=1;
		p_Mp1->sys.core[0].dcache.read_misses=0;
		p_Mp1->sys.core[0].dcache.write_misses=0;
		p_Mp1->sys.core[0].dcache.replacements=1;
		p_Mp1->sys.core[0].dcache.write_backs=1;
		p_Mp1->sys.core[0].dcache.miss_buffer_access=0;
		p_Mp1->sys.core[0].dcache.fill_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_accesses=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_writes=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_reads=1;
		p_Mp1->sys.core[0].dcache.prefetch_buffer_hits=1;
		p_Mp1->sys.core[0].dcache.wbb_writes=1;
		p_Mp1->sys.core[0].dcache.wbb_reads=1;
		//system.core?.BTB
		p_Mp1->sys.core[0].BTB.BTB_config[0]=(int)btb_tech.unit_scap;
		p_Mp1->sys.core[0].BTB.BTB_config[1]=btb_tech.line_size;
		p_Mp1->sys.core[0].BTB.BTB_config[2]=btb_tech.assoc;
		p_Mp1->sys.core[0].BTB.BTB_config[3]=btb_tech.num_banks;
		p_Mp1->sys.core[0].BTB.BTB_config[4]=(int)btb_tech.throughput;
		p_Mp1->sys.core[0].BTB.BTB_config[5]=(int)btb_tech.latency;
		p_Mp1->sys.core[0].BTB.total_accesses=2;
		p_Mp1->sys.core[0].BTB.read_accesses=1;
		p_Mp1->sys.core[0].BTB.write_accesses=1;
		p_Mp1->sys.core[0].BTB.total_hits=1;
		p_Mp1->sys.core[0].BTB.total_misses=0;
		p_Mp1->sys.core[0].BTB.read_hits=1;
		p_Mp1->sys.core[0].BTB.write_hits=1;
		p_Mp1->sys.core[0].BTB.read_misses=0;
		p_Mp1->sys.core[0].BTB.write_misses=0;
		p_Mp1->sys.core[0].BTB.replacements=1;
	//system_L2directory
	p_Mp1->sys.L2directory.L2Dir_config[0]=(int)cache_l2dir_tech.unit_scap.at(0);
	p_Mp1->sys.L2directory.L2Dir_config[1]=cache_l2dir_tech.line_size.at(0);
	p_Mp1->sys.L2directory.L2Dir_config[2]=cache_l2dir_tech.assoc.at(0);
	p_Mp1->sys.L2directory.L2Dir_config[3]=cache_l2dir_tech.num_banks.at(0);
	p_Mp1->sys.L2directory.L2Dir_config[4]=(int)cache_l2dir_tech.throughput.at(0);
	p_Mp1->sys.L2directory.L2Dir_config[5]=(int)cache_l2dir_tech.latency.at(0);
	p_Mp1->sys.L2directory.clockrate = (int)(cache_l2_tech.op_freq/1000000); // Mc unit is MHz;
	p_Mp1->sys.L2directory.ports[20]=1;
	p_Mp1->sys.L2directory.device_type=2;
	strcpy(p_Mp1->sys.L2directory.threeD_stack,"N");
	p_Mp1->sys.L2directory.total_accesses=2;
	p_Mp1->sys.L2directory.read_accesses=1;
	p_Mp1->sys.L2directory.write_accesse=1;
		//system_L2
		p_Mp1->sys.L2[0].L2_config[0]=(int)cache_l2_tech.unit_scap.at(0);
		p_Mp1->sys.L2[0].L2_config[1]=cache_l2_tech.line_size.at(0);
		p_Mp1->sys.L2[0].L2_config[2]=cache_l2_tech.assoc.at(0);
		p_Mp1->sys.L2[0].L2_config[3]=cache_l2_tech.num_banks.at(0);
		p_Mp1->sys.L2[0].L2_config[4]=(int)cache_l2_tech.throughput.at(0);
		p_Mp1->sys.L2[0].L2_config[5]=(int)cache_l2_tech.latency.at(0);
		p_Mp1->sys.L2[0].clockrate=3000;
		p_Mp1->sys.L2[0].ports[20]=1;
		p_Mp1->sys.L2[0].device_type=2;
		strcpy(p_Mp1->sys.L2[0].threeD_stack,"N");
		p_Mp1->sys.L2[0].buffer_sizes[0]=cache_l2_tech.miss_buf_size.at(0);
		p_Mp1->sys.L2[0].buffer_sizes[1]=cache_l2_tech.fill_buf_size.at(0);
		p_Mp1->sys.L2[0].buffer_sizes[2]=cache_l2_tech.prefetch_buf_size.at(0);
		p_Mp1->sys.L2[0].buffer_sizes[3]=cache_l2_tech.wbb_buf_size.at(0);
		p_Mp1->sys.L2[0].total_accesses=2;
		p_Mp1->sys.L2[0].read_accesses=1;
		p_Mp1->sys.L2[0].write_accesses=1;
		p_Mp1->sys.L2[0].total_hits=1;
		p_Mp1->sys.L2[0].total_misses=0;
		p_Mp1->sys.L2[0].read_hits=1;
		p_Mp1->sys.L2[0].write_hits=1;
		p_Mp1->sys.L2[0].read_misses=0;
		p_Mp1->sys.L2[0].write_misses=0;
		p_Mp1->sys.L2[0].replacements=1;
		p_Mp1->sys.L2[0].write_backs=1;
		p_Mp1->sys.L2[0].miss_buffer_accesses=1;
		p_Mp1->sys.L2[0].fill_buffer_accesses=1;
		p_Mp1->sys.L2[0].prefetch_buffer_accesses=1;
		p_Mp1->sys.L2[0].prefetch_buffer_writes=1;
		p_Mp1->sys.L2[0].prefetch_buffer_reads=1;
		p_Mp1->sys.L2[0].prefetch_buffer_hits=1;
		p_Mp1->sys.L2[0].wbb_writes=1;
		p_Mp1->sys.L2[0].wbb_reads=1;
	//system_mem
	p_Mp1->sys.mem.mem_tech_node = core_tech.core_tech_node;
	p_Mp1->sys.mem.device_clock=200; //MHz
	p_Mp1->sys.mem.peak_transfer_rate = mc_tech.peak_transfer_rate;
	p_Mp1->sys.mem.capacity_per_channel=4096; //MB
	p_Mp1->sys.mem.number_ranks = mc_tech.number_ranks;
	p_Mp1->sys.mem.num_banks_of_DRAM_chip=8;
	p_Mp1->sys.mem.Block_width_of_DRAM_chip=64; //B
	p_Mp1->sys.mem.output_width_of_DRAM_chip=8;
	p_Mp1->sys.mem.page_size_of_DRAM_chip=8;
	p_Mp1->sys.mem.burstlength_of_DRAM_chip=8;
	p_Mp1->sys.mem.internal_prefetch_of_DRAM_chip=4;
	p_Mp1->sys.mem.memory_accesses=2;
	p_Mp1->sys.mem.memory_reads=1;
	p_Mp1->sys.mem.memory_writes=1;
	//system_mc
	p_Mp1->sys.mc.mc_clock = (int)(mc_tech.mc_clock/1000000);  // Mc unit is MHz;
	p_Mp1->sys.mc.llc_line_length = mc_tech.llc_line_length;
	p_Mp1->sys.mc.number_mcs=2;
	p_Mp1->sys.mc.memory_channels_per_mc = mc_tech.memory_channels_per_mc;
	p_Mp1->sys.mc.req_window_size_per_channel = mc_tech.req_window_size_per_channel;
	p_Mp1->sys.mc.IO_buffer_size_per_channel = mc_tech.IO_buffer_size_per_channel;
	p_Mp1->sys.mc.databus_width = mc_tech.databus_width;
	p_Mp1->sys.mc.addressbus_width = mc_tech.addressbus_width;
	p_Mp1->sys.mc.memory_accesses=2;
	p_Mp1->sys.mc.memory_reads=1;
	p_Mp1->sys.mc.memory_writes=1;
	//system_NoC
	p_Mp1->sys.NoC[0].clockrate=(int)(router_tech.clockrate/1000000);  // Mc unit is MHz;
	if (router_tech.topology == TWODMESH)
	    strcpy(p_Mp1->sys.NoC[0].topology,"2Dmesh");
	else if (router_tech.topology == RING)
	    strcpy(p_Mp1->sys.NoC[0].topology,"ring");
	else if (router_tech.topology == CROSSBAR)
	    strcpy(p_Mp1->sys.NoC[0].topology,"crossbar");	p_Mp1->sys.NoC[0].horizontal_nodes=router_tech.horizontal_nodes;
	p_Mp1->sys.NoC[0].vertical_nodes=router_tech.vertical_nodes;
	p_Mp1->sys.NoC[0].input_ports=router_tech.input_ports;
	p_Mp1->sys.NoC[0].output_ports=router_tech.output_ports;
	p_Mp1->sys.NoC[0].virtual_channel_per_port=router_tech.virtual_channel_per_port;
	p_Mp1->sys.NoC[0].flit_bits=router_tech.flit_bits;
	p_Mp1->sys.NoC[0].input_buffer_entries_per_vc=router_tech.input_buffer_entries_per_vc;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[0]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[1]=1;
	p_Mp1->sys.NoC[0].ports_of_input_buffer[2]=0;
	p_Mp1->sys.NoC[0].number_of_crossbars=1;
	strcpy(p_Mp1->sys.NoC[0].dual_pump,"N");
	strcpy(p_Mp1->sys.NoC[0].crossbar_type,"matrix");
	strcpy(p_Mp1->sys.NoC[0].crosspoint_type,"tri");
		//system.NoC?.xbar0;
		p_Mp1->sys.NoC[0].xbar0.number_of_inputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.number_of_outputs_of_crossbars=4;
		p_Mp1->sys.NoC[0].xbar0.flit_bits=router_tech.flit_bits;
		p_Mp1->sys.NoC[0].xbar0.input_buffer_entries_per_port=1;
		p_Mp1->sys.NoC[0].xbar0.ports_of_input_buffer[20]=1;
		p_Mp1->sys.NoC[0].xbar0.crossbar_accesses=521;

}
#endif //McPAT05_H


/* The following is adopted from W Song's power interface */
/************************************************************************
* Creates floorplans and thermal tiles based on the user inputs in xml  *
* (currently hard coded in power.h. Floorplans include sizing and      *
* coordinate information (feature_t) and device parameters. Thermal     *
* tiles are created for silicon, interface, spreader, and heatsink      *
* layers. Codes are from W Song's power interface.			*
************************************************************************/
void Power::setChip(Component::Params_t deviceParams)
{
    //First, set up device parameter values
    setTech(deviceParams);

    //floorplan only needs to be setup once; eventually, floorplan setup should be done by xml input
    if(chip.is_set == false){
  	//initialize floorplan param
  	floorParamInitialize();

  	#ifdef ENERGY_INTERFACE_DEBUG
  	  cout << "setting floorplans ... " << endl;
  	#endif 

  	// floorplan setup
  	int num_floorplans = 0; // counting the number of floorplans
  	floorplan_t floorplan;
  	for(map<int,parameters_floorplan_t>::iterator fit = chip.floorplan.begin(); fit != chip.floorplan.end(); fit++)
  	{
    	  #ifdef ENERGY_INTERFACE_DEBUG
    	    cout << "ENERGY_INTERFACE_DEBUG: setting floorplan #" << (*fit).first << " (" << (*fit).second.name << ")" << endl;
    	  #endif
    	  floorplan.id = (*fit).second.id;
    	  floorplan.name = (*fit).second.name;
    	  floorplan.feature = (feature_t)(*fit).second.feature;
    	  floorplan.device_tech = (parameters_tech_t)(*fit).second.device_tech;
    	  floorplan.device_tech.temperature = (*chip.thermal_tile.find(pair<int,int>(SILICON,(*fit).first))).second.temperature;
	  memset(&floorplan.p_usage_floorplan,0,sizeof(Pdissipation_t));
	  floorplan.pstate = P_ACTIVE;
    	  p_chip.floorplan.insert(pair<int,floorplan_t>((*fit).first,floorplan));
    	  ++num_floorplans;
  	}

  	// number of floorplans used for chip-level thermal modeling
  	chip.num_floorplans = num_floorplans;

  	// link the chip to thermal library if want thermal modeling
  	if (p_tempMonitor == true){
   	   switch(chip.thermal_library)
    	  {
      		case HOTSPOT: p_chip.thermal_library = new HotSpot_library(chip); break;
      		default: cout << "ERROR: invalid thermal library" << endl; break;
    	  }
  	}
	chip.is_set = true;
    }
}


/********************************************************
* Floorplan/thermal tiles parameter are temporarily	*
* initialized and hard coded here			*
* Eventually, these should be read in from xml		*
********************************************************/
void Power::floorParamInitialize()
{
   cout << "Initializing the floorplan/thermal tiles parameters ... " << endl;

  parameters_thermal_tile_t tile_input;		// thermal tile setup
  parameters_floorplan_t floorplan_input;	// floorplan setup

  // chip setup
  chip.thermal_library = HOTSPOT;
  chip.thermal_threshold = 350.0;//81.8 + 273.15;
  chip.chip_thickness = 0.15e-3;
  chip.chip_thermal_conduct = 100.0;
  chip.chip_heat = 1.75e6;
  chip.heatsink_convection_cap = 140.4;
  chip.heatsink_convection_res = 0.1;
  chip.heatsink_side = 60e-3;
  chip.heatsink_thickness = 6.9e-3;
  chip.heatsink_thermal_conduct = 400.0;
  chip.heatsink_heat = 3.55e6;
  chip.spreader_side = 46e-3;
  chip.spreader_thickness = 1e-3;
  chip.spreader_thermal_conduct = 400.0;
  chip.spreader_heat = 3.55e6;
  chip.interface_thickness = 20e-6;
  chip.interface_thermal_conduct = 4.0;
  chip.interface_heat = 4.0e6;
  chip.secondary_model = false;
  chip.secondary_convection_res = 1.0;
  chip.secondary_convection_cap = 140.4; //FIXME! need updated value.
  chip.metal_layers = 8;
  chip.metal_thickness = 10.0e-6;
  chip.c4_thickness = 0.0001;
  chip.c4_side = 20.0e-6;
  chip.c4_pads = 400;
  chip.substrate_side = 0.021;
  chip.substrate_thickness = 0.001;
  chip.solder_side = 0.021;
  chip.solder_thickness = 0.00094;
  chip.pcb_side = 0.1;
  chip.pcb_thickness = 0.002;
  chip.ambient = 315.0;
  chip.sampling_interval = 1e-3;
  chip.clock_frequency = 10.0/3.0*1e9;
  chip.leakage_used = 0;
  chip.leakage_mode = 0;
  chip.package_model_used = 0;
  chip.block_omit_lateral = false;
  chip.num_grid_rows = 64;
  chip.num_grid_cols = 64;


  // floorplan & thermal tiles
  tile_input.layer = SILICON;
  tile_input.id = 0;
  tile_input.name = "silicon:core0";
  tile_input.temperature = 350;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 0;
  tile_input.name = "interface:core0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 0;
  tile_input.name = "spreader:core0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 0;
  tile_input.name = "heatsink:core0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 0;
  floorplan_input.name = "core0";
  floorplan_input.feature.x_position = 0.0;
  floorplan_input.feature.y_position = 0.0;
  floorplan_input.feature.width = 4.e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(/*feature size*/core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(/*connecting floorplan*/1,/*wire density*/1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(/*connecting floorplan*/4,/*wire density*/1.0));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 0... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 1;
  tile_input.name = "silicon:core1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 1;
  tile_input.name = "interface:core1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 1;
  tile_input.name = "spreader:core1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 1;
  tile_input.name = "heatsink:core1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 1;
  floorplan_input.name = "core1";
  floorplan_input.feature.x_position = 4e-3;
  floorplan_input.feature.y_position = 0.0;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(/*feature size*/core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(/*connecting floorplan*/0,/*wire density*/1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(/*connecting floorplan*/4,/*wire density*/1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(/*connecting floorplan*/6,/*wire density*/0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 1... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 2;
  tile_input.name = "silicon:core2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 2;
  tile_input.name = "interface:core2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 2;
  tile_input.name = "spreader:core2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 2;
  tile_input.name = "heatsink:core2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 2;
  floorplan_input.name = "core2";
  floorplan_input.feature.x_position = 11.07e-3;
  floorplan_input.feature.y_position = 0.0;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(5,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(3,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(6,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 2... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 3;
  tile_input.name = "silicon:core3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 3;
  tile_input.name = "interface:core3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 3;
  tile_input.name = "spreader:core3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 3;
  tile_input.name = "heatsink:core3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 3;
  floorplan_input.name = "core3";
  floorplan_input.feature.x_position = 15.07e-3;
  floorplan_input.feature.y_position = 0.0;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(2,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(5,1.0));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 3... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 4;
  tile_input.name = "silicon:L2_1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 4;
  tile_input.name = "interface:L2_1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 4;
  tile_input.name = "spreader:L2_1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 4;
  tile_input.name = "heatsink:L2_1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 4;
  floorplan_input.name = "L2_1";
  floorplan_input.feature.x_position = 0.0;
  floorplan_input.feature.y_position = 9e-3;
  floorplan_input.feature.width = 8e-3;
  floorplan_input.feature.length = 6e-3;
  floorplan_input.feature.area = 48e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(0,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(1,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(6,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 4... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 5;
  tile_input.name = "silicon:L2_2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 5;
  tile_input.name = "interface:L2_2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 5;
  tile_input.name = "spreader:L2_2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 5;
  tile_input.name = "heatsink:L2_2";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 5;
  floorplan_input.name = "L2_2";
  floorplan_input.feature.x_position = 11.07e-3;
  floorplan_input.feature.y_position = 9e-3;
  floorplan_input.feature.width = 8e-3;
  floorplan_input.feature.length = 6e-3;
  floorplan_input.feature.area = 48e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(2,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(3,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(6,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 5... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 6;
  tile_input.name = "silicon:gap0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 6;
  tile_input.name = "interface:gap0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 6;
  tile_input.name = "spreader:gap0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 6;
  tile_input.name = "heatsink:gap0";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 6;
  floorplan_input.name = "gap0";
  floorplan_input.feature.x_position = 8e-3;
  floorplan_input.feature.y_position = 0.0;
  floorplan_input.feature.width = 3.07e-3;
  floorplan_input.feature.length = 15e-3;
  floorplan_input.feature.area = 46.05e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(1,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(2,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(4,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(5,0.1));
  ////floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 6... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 7;
  tile_input.name = "silicon:board";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 7;
  tile_input.name = "interface:board";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 7;
  tile_input.name = "spreader:board";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 7;
  tile_input.name = "heatsink:board";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 7;
  floorplan_input.name = "board";
  floorplan_input.feature.x_position = 0;
  floorplan_input.feature.y_position = 15e-3;
  floorplan_input.feature.width = 19.07e-3;
  floorplan_input.feature.length = 15e-3;
  floorplan_input.feature.area = 286.05e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(4,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(5,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(6,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(12,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(13,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 7... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 8;
  tile_input.name = "silicon:core4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 8;
  tile_input.name = "interface:core4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 8;
  tile_input.name = "spreader:core4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 8;
  tile_input.name = "heatsink:core4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 8;
  floorplan_input.name = "core4";
  floorplan_input.feature.x_position = 0.0;
  floorplan_input.feature.y_position = 36e-3;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(12,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(9,1.0));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 8... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 9;
  tile_input.name = "silicon:core5";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 9;
  tile_input.name = "interface:core5";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 9;
  tile_input.name = "spreader:core5";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 9;
  tile_input.name = "heatsink:core5";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 9;
  floorplan_input.name = "core5";
  floorplan_input.feature.x_position = 4e-3;
  floorplan_input.feature.y_position = 36e-3;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(12,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(8,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 9... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 10;
  tile_input.name = "silicon:core6";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 10;
  tile_input.name = "interface:core6";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 10;
  tile_input.name = "spreader:core6";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 10;
  tile_input.name = "heatsink:core6";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 10;
  floorplan_input.name = "core6";
  floorplan_input.feature.x_position = 11.07e-3;
  floorplan_input.feature.y_position = 36e-3;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(11,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(13,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 10... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 11;
  tile_input.name = "silicon:core7";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 11;
  tile_input.name = "interface:core7";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 11;
  tile_input.name = "spreader:core7";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 11;
  tile_input.name = "heatsink:core7";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 11;
  floorplan_input.name = "core7";
  floorplan_input.feature.x_position = 15.07e-3;
  floorplan_input.feature.y_position = 36e-3;
  floorplan_input.feature.width = 4e-3;
  floorplan_input.feature.length = 9e-3;
  floorplan_input.feature.area = 36e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(10,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(13,1.0));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 11... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 12;
  tile_input.name = "silicon:L2_3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 12;
  tile_input.name = "interface:L2_3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 12;
  tile_input.name = "spreader:L2_3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 12;
  tile_input.name = "heatsink:L2_3";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 12;
  floorplan_input.name = "L2_3";
  floorplan_input.feature.x_position = 0.0;
  floorplan_input.feature.y_position = 30e-3;
  floorplan_input.feature.width = 8e-3;
  floorplan_input.feature.length = 6e-3;
  floorplan_input.feature.area = 48e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(8,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(9,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 12... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 13;
  tile_input.name = "silicon:L2_4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 13;
  tile_input.name = "interface:L2_4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 13;
  tile_input.name = "spreader:L2_4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 13;
  tile_input.name = "heatsink:L2_4";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 13;
  floorplan_input.name = "L2_4";
  floorplan_input.feature.x_position = 11.07e-3;
  floorplan_input.feature.y_position = 30e-3;
  floorplan_input.feature.width = 8e-3;
  floorplan_input.feature.length = 6e-3;
  floorplan_input.feature.area = 48e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(10,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(11,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 13... " << endl;

  tile_input.layer = SILICON;
  tile_input.id = 14;
  tile_input.name = "silicon:gap1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 14;
  tile_input.name = "interface:gap1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 14;
  tile_input.name = "spreader:gap1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 14;
  tile_input.name = "heatsink:gap1";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 14;
  floorplan_input.name = "gap1";
  floorplan_input.feature.x_position = 8e-3;
  floorplan_input.feature.y_position = 30e-3;
  floorplan_input.feature.width = 3.07e-3;
  floorplan_input.feature.length = 15e-3;
  floorplan_input.feature.area = 46.05e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(7,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(9,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(10,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(12,0.1));
  floorplan_input.thermal_correlation.insert(pair<int,double>(13,0.1));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 14... " << endl;

  /*tile_input.layer = SILICON;
  tile_input.id = 15;
  tile_input.name = "silicon:core15";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = INTERFACE;
  tile_input.id = 15;
  tile_input.name = "interface:core15";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = 15;
  tile_input.name = "spreader:core15";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = 15;
  tile_input.name = "heatsink:core15";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  floorplan_input.id = 15;
  floorplan_input.name = "core15";
  floorplan_input.feature.x_position = 14.44602e-3;
  floorplan_input.feature.y_position = 14.44602e-3;
  floorplan_input.feature.width = 4.81534e-3;
  floorplan_input.feature.length = 4.81534e-3;
  floorplan_input.feature.area = 23.1875e-6;
  floorplan_input.device_tech.set_default(core_tech.core_tech_node, HP);
  floorplan_input.device_tech.clock_frequency = device_tech.clockRate;
  floorplan_input.thermal_correlation.insert(pair<int,double>(11,1.0));
  floorplan_input.thermal_correlation.insert(pair<int,double>(14,1.0));
  chip.insert(&floorplan_input);
  cout << "Initializing the parameters tile 15... " << endl;*/

  // rest of thermal tiles (sides of spreader and heatsink)
  tile_input.layer = SPREADER;
  tile_input.id = -1;
  tile_input.name = "spreader:side:west";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -1;
  tile_input.name = "heatsink:inner:west";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = -2;
  tile_input.name = "spreader:side:east";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -2;
  tile_input.name = "heatsink:inner:east";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = -3;
  tile_input.name = "spreader:side:north";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -3;
  tile_input.name = "heatsink:inner:north";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = SPREADER;
  tile_input.id = -4;
  tile_input.name = "spreader:side:south";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -4;
  tile_input.name = "heatsink:inner:south";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -5;
  tile_input.name = "heatsink:outer:west";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -6;
  tile_input.name = "heatsink:outer:east";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -7;
  tile_input.name = "heatsink:outer:north";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);
  tile_input.layer = HEATSINK;
  tile_input.id = -8;
  tile_input.name = "heatsink:outer:south";
  tile_input.temperature = 350.0;
  chip.insert(&tile_input);

}

/*****************************************
* Initialize device params		 *
******************************************/
void parameters_tech_t::set_default(double size, int RAM_tech_type)
{
  memset(this,0,sizeof(parameters_tech_t));

  set = true;

  RAM_tech = RAM_tech_type;

  // Constants
  bulk_copper_resistivity = 1.8e-8;
  copper_resistivity = 2.2e-8;
  copper_reflectivity = 0.5;
  Rents_const_k = 4.0;
  Rents_const_p = 0.6;
  MOSFET_alpha = 1.3;
  subvtslope = 100e-3;

  // IntSim parameters
  critical_path_depth = 10;
  via_design_rule = 3;
  avg_fanouts = 3;
  avg_latches_per_buffer = 20;
  activity_factor = 0.1;
  wiring_aspect_ratio = 2.0;
  specularity_parameter = 0.5;
  power_pad_distance = 300e-6;
  power_pad_length = 50e-6;
  IR_drop_limit = 0.02;
  router_efficiency = 0.5;
  repeater_efficiency = 0.5;
  clock_lost_ratio = 0.2;
  max_H_tree_span = 3e-3;
  clock_factor = 1.0;
  clock_gating_factor = 0.4;
  power_signal_wire_ratio = 2.0;
  clock_signal_wire_ratio = 4.0;
  max_clock_skew = 0.25;

/* ----- 16nm ----- */
  if((size>14.0)&&(size<18.0)) // 16nm
  {
    feature_size = 16.0;
  }

/* ----- 22nm ----- */
  else if((size>20.0)&&(size<24.0))
  {
    feature_size = 22.0;

    fringe_cap[0][0] = 0.115e-15;
    miller_value[0][0] = 1.5;
    ild_thickness[0][0] = 0.15;
    horiz_dielectric_constant[0][0] = 1.414;
    vert_dielectric_constant[0][0] = 3.9;
    aspect_ratio[0][0] = 3.0;
    wire_pitch[0][0] = 2.5 * feature_size*1e-3;
//    cout << "parameters.cc: debug = " << 1.0*bulk_copper_resistivity*1e6/((0.5*aspect_ratio[0][0]*wire_pitch[0][0]-0-0)*(0.5*wire_pitch[0][0]-2.0*0)) << endl;
//  resistance = alpha_scatter * resistivity /((wire_thickness - barrier_thickness - dishing_thickness)*(wire_width - 2 * barrier_thickness));
    wire_r_per_micron[0][0] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][0],/*wire_thickness*/0.5*aspect_ratio[0][0]*wire_pitch[0][0],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][0] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][0],/*wire_thickness*/0.5*aspect_ratio[0][0]*wire_pitch[0][0],/*wire_spacing*/0.5*wire_pitch[0][0],/*ild_thickness*/ild_thickness[0][0],miller_value[0][0],horiz_dielectric_constant[0][0],vert_dielectric_constant[0][0],/*fringe_cap*/fringe_cap[0][0]);

    fringe_cap[0][1] = 0.115e-15;
    miller_value[0][1] = 1.5;
    ild_thickness[0][1] = 0.15;
    horiz_dielectric_constant[0][1] = 1.414;
    vert_dielectric_constant[0][1] = 3.9;
    aspect_ratio[0][1] = 3.0;
    wire_pitch[0][1] = 4.0 * feature_size*1e-3;
    wire_r_per_micron[0][1] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][1],/*wire_thickness*/0.5*aspect_ratio[0][1]*wire_pitch[0][1],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][1] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][1],/*wire_thickness*/0.5*aspect_ratio[0][1]*wire_pitch[0][1],/*wire_spacing*/0.5*wire_pitch[0][1],/*ild_thickness*/ild_thickness[0][1],miller_value[0][1],horiz_dielectric_constant[0][1],vert_dielectric_constant[0][1],/*fringe_cap*/fringe_cap[0][1]);

    fringe_cap[0][2] = 0.115e-15;
    miller_value[0][2] = 1.5;
    ild_thickness[0][2] = 0.3;
    horiz_dielectric_constant[0][2] = 1.414;
    vert_dielectric_constant[0][2] = 3.9;
    aspect_ratio[0][2] = 3.0;
    wire_pitch[0][2] = 8.0 * feature_size*1e-3;
    wire_r_per_micron[0][2] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][2],/*wire_thickness*/0.5*aspect_ratio[0][2]*wire_pitch[0][2],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][2] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][2],/*wire_thickness*/0.5*aspect_ratio[0][2]*wire_pitch[0][2],/*wire_spacing*/0.5*wire_pitch[0][2],/*ild_thickness*/ild_thickness[0][2],miller_value[0][2],horiz_dielectric_constant[0][2],vert_dielectric_constant[0][2],/*fringe_cap*/fringe_cap[0][2]);

    fringe_cap[1][0] = 0.115e-15;
    miller_value[1][0] = 1.5;
    ild_thickness[1][0] = 0.15;
    horiz_dielectric_constant[1][0] = 2.104;
    vert_dielectric_constant[1][0] = 3.9;
    aspect_ratio[1][0] = 2.0;
    wire_pitch[1][0] = 2.5 * feature_size*1e-3;
    wire_r_per_micron[1][0] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][0],/*wire_thickness*/0.5*aspect_ratio[1][0]*wire_pitch[1][0],/*barrier_thickness*/0.003,/*dishing_thickness*/0,/*alpha_scatter*/1.05);
    wire_c_per_micron[1][0] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][0],/*wire_thickness*/0.5*aspect_ratio[1][0]*wire_pitch[1][0],/*wire_spacing*/0.5*wire_pitch[1][0],/*ild_thickness*/ild_thickness[1][0],miller_value[1][0],horiz_dielectric_constant[1][0],vert_dielectric_constant[1][0],/*fringe_cap*/fringe_cap[1][0]);

    fringe_cap[1][1] = 0.115e-15;
    miller_value[1][1] = 1.5;
    ild_thickness[1][1] = 0.15;
    horiz_dielectric_constant[1][1] = 2.104;
    vert_dielectric_constant[1][1] = 3.9;
    aspect_ratio[1][1] = 2.0;
    wire_pitch[1][1] = 4.0 * feature_size*1e-3;
    wire_r_per_micron[1][1] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][1],/*wire_thickness*/0.5*aspect_ratio[1][1]*wire_pitch[1][1],/*barrier_thickness*/0.003,/*dishing_thickness*/0,/*alpha_scatter*/1.05);
    wire_c_per_micron[1][1] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][1],/*wire_thickness*/0.5*aspect_ratio[1][1]*wire_pitch[1][1],/*wire_spacing*/0.5*wire_pitch[1][1],/*ild_thickness*/ild_thickness[1][1],miller_value[1][1],horiz_dielectric_constant[1][1],vert_dielectric_constant[1][1],/*fringe_cap*/fringe_cap[1][1]);

    fringe_cap[1][2] = 0.115e-15;
    miller_value[1][2] = 1.5;
    ild_thickness[1][2] = 0.275;
    horiz_dielectric_constant[1][2] = 2.104;
    vert_dielectric_constant[1][2] = 3.9;
    aspect_ratio[1][2] = 2.2;
    wire_pitch[1][2] = 8.0 * feature_size*1e-3;
    wire_r_per_micron[1][2] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][2],/*wire_thickness*/0.5*aspect_ratio[1][2]*wire_pitch[1][2],/*barrier_thickness*/0.003,/*dishing_thickness*/0.05*aspect_ratio[1][2]*wire_pitch[1][2],/*alpha_scatter*/1.05);
    wire_c_per_micron[1][2] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][2],/*wire_thickness*/0.5*aspect_ratio[1][2]*wire_pitch[1][2],/*wire_spacing*/0.5*wire_pitch[1][2],/*ild_thickness*/ild_thickness[1][2],miller_value[1][2],horiz_dielectric_constant[1][2],vert_dielectric_constant[1][2],/*fringe_cap*/fringe_cap[1][2]);

    fringe_cap[1][3] = 0.115e-15;
    wire_pitch[1][3] = 2.0 * feature_size*1e-3;
    wire_r_per_micron[1][3] = 12.0/(0.5*wire_pitch[1][3]);
    wire_c_per_micron[1][3] = 37.5e-15/(256.0*wire_pitch[1][3]);//F/micron

    switch(RAM_tech)
    {
      case HP:
        vdd = 0.8;
        Vdsat = 0.0233;
        Vth = 0.1395;
        L_phy = 0.009;
        L_elec = 0.00468;
        t_ox = 0.55e-9;
        c_ox = 3.63e-14;
        mobility_eff = 426.07e8;
        C_g_ideal = 3.27e-16;
        C_fringe = 0.06e-15;
        C_junc = 0;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 2626.4e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.0;
        Rn_channel_on = 1.45*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 1.38;
        long_channel_leakage_reduction = 1.0/3.274;
        I_g_on_n[0] = 1.81e-9; 
        I_g_on_n[1] = 1.81e-9; I_g_on_n[2] = 1.81e-9; I_g_on_n[3] = 1.81e-9; I_g_on_n[4] = 1.81e-9; I_g_on_n[5] = 1.81e-9; 
        I_g_on_n[6] = 1.81e-9; I_g_on_n[7] = 1.81e-9; I_g_on_n[8] = 1.81e-9; I_g_on_n[9] = 1.81e-9; I_g_on_n[10] = 1.81e-9;
        I_g_on_n[11] = 1.81e-9; I_g_on_n[12] = 1.81e-9; I_g_on_n[13] = 1.81e-9; I_g_on_n[14] = 1.81e-9; I_g_on_n[15] = 1.81e-9; 
        I_g_on_n[16] = 1.81e-9; I_g_on_n[17] = 1.81e-9; I_g_on_n[18] = 1.81e-9; I_g_on_n[19] = 1.81e-9; I_g_on_n[20] = 1.81e-9;
        I_g_on_n[21] = 1.81e-9; I_g_on_n[22] = 1.81e-9; I_g_on_n[23] = 1.81e-9; I_g_on_n[24] = 1.81e-9; I_g_on_n[25] = 1.81e-9; 
        I_g_on_n[26] = 1.81e-9; I_g_on_n[27] = 1.81e-9; I_g_on_n[28] = 1.81e-9; I_g_on_n[29] = 1.81e-9; I_g_on_n[30] = 1.81e-9;
        I_g_on_n[31] = 1.81e-9; I_g_on_n[32] = 1.81e-9; I_g_on_n[33] = 1.81e-9; I_g_on_n[34] = 1.81e-9; I_g_on_n[35] = 1.81e-9; 
        I_g_on_n[36] = 1.81e-9; I_g_on_n[37] = 1.81e-9; I_g_on_n[38] = 1.81e-9; I_g_on_n[39] = 1.81e-9; I_g_on_n[40] = 1.81e-9;
        I_g_on_n[41] = 1.81e-9; I_g_on_n[42] = 1.81e-9; I_g_on_n[43] = 1.81e-9; I_g_on_n[44] = 1.81e-9; I_g_on_n[45] = 1.81e-9; 
        I_g_on_n[46] = 1.81e-9; I_g_on_n[47] = 1.81e-9; I_g_on_n[48] = 1.81e-9; I_g_on_n[49] = 1.81e-9; I_g_on_n[50] = 1.81e-9;
        I_g_on_n[51] = 1.81e-9; I_g_on_n[52] = 1.81e-9; I_g_on_n[53] = 1.81e-9; I_g_on_n[54] = 1.81e-9; I_g_on_n[55] = 1.81e-9; 
        I_g_on_n[56] = 1.81e-9; I_g_on_n[57] = 1.81e-9; I_g_on_n[58] = 1.81e-9; I_g_on_n[59] = 1.81e-9; I_g_on_n[60] = 1.81e-9;
        I_g_on_n[61] = 1.81e-9; I_g_on_n[62] = 1.81e-9; I_g_on_n[63] = 1.81e-9; I_g_on_n[64] = 1.81e-9; I_g_on_n[65] = 1.81e-9; 
        I_g_on_n[66] = 1.81e-9; I_g_on_n[67] = 1.81e-9; I_g_on_n[68] = 1.81e-9; I_g_on_n[69] = 1.81e-9; I_g_on_n[70] = 1.81e-9;
        I_g_on_n[71] = 1.81e-9; I_g_on_n[72] = 1.81e-9; I_g_on_n[73] = 1.81e-9; I_g_on_n[74] = 1.81e-9; I_g_on_n[75] = 1.81e-9; 
        I_g_on_n[76] = 1.81e-9; I_g_on_n[77] = 1.81e-9; I_g_on_n[78] = 1.81e-9; I_g_on_n[79] = 1.81e-9; I_g_on_n[80] = 1.81e-9;
        I_g_on_n[81] = 1.81e-9; I_g_on_n[82] = 1.81e-9; I_g_on_n[83] = 1.81e-9; I_g_on_n[84] = 1.81e-9; I_g_on_n[85] = 1.81e-9; 
        I_g_on_n[86] = 1.81e-9; I_g_on_n[87] = 1.81e-9; I_g_on_n[88] = 1.81e-9; I_g_on_n[89] = 1.81e-9; I_g_on_n[90] = 1.81e-9;
        I_g_on_n[91] = 1.81e-9; I_g_on_n[92] = 1.81e-9; I_g_on_n[93] = 1.81e-9; I_g_on_n[94] = 1.81e-9; I_g_on_n[95] = 1.81e-9; 
        I_g_on_n[96] = 1.81e-9; I_g_on_n[97] = 1.81e-9; I_g_on_n[98] = 1.81e-9; I_g_on_n[99] = 1.81e-9; I_g_on_n[100] = 1.81e-9;
        I_off_n[0] = 1.22e-7; 
        I_off_n[1] = 1.22e-7; I_off_n[2] = 1.22e-7; I_off_n[3] = 1.22e-7; I_off_n[4] = 1.22e-7; I_off_n[5] = 1.23e-7; 
        I_off_n[6] = 1.23e-7; I_off_n[7] = 1.23e-7; I_off_n[8] = 1.23e-7; I_off_n[9] = 1.24e-7; I_off_n[10] = 1.24e-7;
        I_off_n[11] = 1.24e-7; I_off_n[12] = 1.25e-7; I_off_n[13] = 1.25e-7; I_off_n[14] = 1.25e-7; I_off_n[15] = 1.25e-7; 
        I_off_n[16] = 1.26e-7; I_off_n[17] = 1.26e-7; I_off_n[18] = 1.26e-7; I_off_n[19] = 1.27e-7; I_off_n[20] = 1.27e-7;
        I_off_n[21] = 1.28e-7; I_off_n[22] = 1.28e-7; I_off_n[23] = 1.29e-7; I_off_n[24] = 1.29e-7; I_off_n[25] = 1.30e-7; 
        I_off_n[26] = 1.31e-7; I_off_n[27] = 1.32e-7; I_off_n[28] = 1.32e-7; I_off_n[29] = 1.33e-7; I_off_n[30] = 1.34e-7;
        I_off_n[31] = 1.35e-7; I_off_n[32] = 1.37e-7; I_off_n[33] = 1.38e-7; I_off_n[34] = 1.39e-7; I_off_n[35] = 1.41e-7; 
        I_off_n[36] = 1.43e-7; I_off_n[37] = 1.45e-7; I_off_n[38] = 1.47e-7; I_off_n[39] = 1.49e-7; I_off_n[40] = 1.52e-7;
        I_off_n[41] = 1.55e-7; I_off_n[42] = 1.59e-7; I_off_n[43] = 1.64e-7; I_off_n[44] = 1.69e-7; I_off_n[45] = 1.75e-7; 
        I_off_n[46] = 1.82e-7; I_off_n[47] = 1.89e-7; I_off_n[48] = 1.97e-7; I_off_n[49] = 2.06e-7; I_off_n[50] = 2.15e-7;
        I_off_n[51] = 2.27e-7; I_off_n[52] = 2.41e-7; I_off_n[53] = 2.58e-7; I_off_n[54] = 2.77e-7; I_off_n[55] = 2.98e-7; 
        I_off_n[56] = 3.21e-7; I_off_n[57] = 3.46e-7; I_off_n[58] = 3.72e-7; I_off_n[59] = 3.98e-7; I_off_n[60] = 4.26e-7;
        I_off_n[61] = 4.55e-7; I_off_n[62] = 4.87e-7; I_off_n[63] = 5.23e-7; I_off_n[64] = 5.61e-7; I_off_n[65] = 6.01e-7; 
        I_off_n[66] = 6.43e-7; I_off_n[67] = 6.86e-7; I_off_n[68] = 7.29e-7; I_off_n[69] = 7.73e-7; I_off_n[70] = 8.16e-7;
        I_off_n[71] = 8.59e-7; I_off_n[72] = 9.01e-7; I_off_n[73] = 9.44e-7; I_off_n[74] = 9.87e-7; I_off_n[75] = 1.03e-6; 
        I_off_n[76] = 1.08e-6; I_off_n[77] = 1.13e-6; I_off_n[78] = 1.18e-6; I_off_n[79] = 1.24e-6; I_off_n[80] = 1.30e-6;
        I_off_n[81] = 1.36e-6; I_off_n[82] = 1.43e-6; I_off_n[83] = 1.50e-6; I_off_n[84] = 1.57e-6; I_off_n[85] = 1.65e-6; 
        I_off_n[86] = 1.74e-6; I_off_n[87] = 1.84e-6; I_off_n[88] = 1.94e-6; I_off_n[89] = 2.06e-6; I_off_n[90] = 2.18e-6;
        I_off_n[91] = 2.34e-6; I_off_n[92] = 2.52e-6; I_off_n[93] = 2.74e-6; I_off_n[94] = 2.98e-6; I_off_n[95] = 3.25e-6; 
        I_off_n[96] = 3.54e-6; I_off_n[97] = 3.85e-6; I_off_n[98] = 4.18e-6; I_off_n[99] = 4.52e-6; I_off_n[100] = 4.88e-6;
        break;
      case LSTP:
        vdd = 0.8;
        Vdsat = 0.0664;
        Vth = 0.40126;
        L_phy = 0.014;
        L_elec = 0.008;
        t_ox = 1.1e-9;
        c_ox = 2.30e-14;
        mobility_eff = 738.09e8;
        C_g_ideal = 3.22e-16;
        C_fringe = 0.08e-15;
        C_junc = 0;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 727.6e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.0;
        Rn_channel_on = 1.99*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 0.99;
        long_channel_leakage_reduction = 1.0/1.89;
        break;
      case LOP:
        vdd = 0.6;
        Vdsat = 0.0181;
        Vth = 0.2315;
        L_phy = 0.011;
        L_elec = 0.00604;
        t_ox = 0.8e-9;
        c_ox = 2.87e-14;
        mobility_eff = 698.37e8;
        C_g_ideal = 3.16e-16;
        C_fringe = 0.08e-15;
        C_junc = 0;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 916.1e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.0;
        Rn_channel_on = 1.73*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 1.11;
        long_channel_leakage_reduction = 1.0/2.38;
        I_g_on_n[0] = 2.74e-9; 
        I_g_on_n[1] = 2.74e-9; I_g_on_n[2] = 2.74e-9; I_g_on_n[3] = 2.74e-9; I_g_on_n[4] = 2.74e-9; I_g_on_n[5] = 2.74e-9; 
        I_g_on_n[6] = 2.74e-9; I_g_on_n[7] = 2.74e-9; I_g_on_n[8] = 2.74e-9; I_g_on_n[9] = 2.74e-9; I_g_on_n[10] = 2.74e-9;
        I_g_on_n[11] = 2.74e-9; I_g_on_n[12] = 2.74e-9; I_g_on_n[13] = 2.74e-9; I_g_on_n[14] = 2.74e-9; I_g_on_n[15] = 2.74e-9; 
        I_g_on_n[16] = 2.74e-9; I_g_on_n[17] = 2.74e-9; I_g_on_n[18] = 2.74e-9; I_g_on_n[19] = 2.74e-9; I_g_on_n[20] = 2.74e-9;
        I_g_on_n[21] = 2.74e-9; I_g_on_n[22] = 2.74e-9; I_g_on_n[23] = 2.74e-9; I_g_on_n[24] = 2.74e-9; I_g_on_n[25] = 2.74e-9; 
        I_g_on_n[26] = 2.74e-9; I_g_on_n[27] = 2.74e-9; I_g_on_n[28] = 2.74e-9; I_g_on_n[29] = 2.74e-9; I_g_on_n[30] = 2.74e-9;
        I_g_on_n[31] = 2.74e-9; I_g_on_n[32] = 2.74e-9; I_g_on_n[33] = 2.74e-9; I_g_on_n[34] = 2.74e-9; I_g_on_n[35] = 2.74e-9; 
        I_g_on_n[36] = 2.74e-9; I_g_on_n[37] = 2.74e-9; I_g_on_n[38] = 2.74e-9; I_g_on_n[39] = 2.74e-9; I_g_on_n[40] = 2.74e-9;
        I_g_on_n[41] = 2.74e-9; I_g_on_n[42] = 2.74e-9; I_g_on_n[43] = 2.74e-9; I_g_on_n[44] = 2.74e-9; I_g_on_n[45] = 2.74e-9; 
        I_g_on_n[46] = 2.74e-9; I_g_on_n[47] = 2.74e-9; I_g_on_n[48] = 2.74e-9; I_g_on_n[49] = 2.74e-9; I_g_on_n[50] = 2.74e-9;
        I_g_on_n[51] = 2.74e-9; I_g_on_n[52] = 2.74e-9; I_g_on_n[53] = 2.74e-9; I_g_on_n[54] = 2.74e-9; I_g_on_n[55] = 2.74e-9; 
        I_g_on_n[56] = 2.74e-9; I_g_on_n[57] = 2.74e-9; I_g_on_n[58] = 2.74e-9; I_g_on_n[59] = 2.74e-9; I_g_on_n[60] = 2.74e-9;
        I_g_on_n[61] = 2.74e-9; I_g_on_n[62] = 2.74e-9; I_g_on_n[63] = 2.74e-9; I_g_on_n[64] = 2.74e-9; I_g_on_n[65] = 2.74e-9; 
        I_g_on_n[66] = 2.74e-9; I_g_on_n[67] = 2.74e-9; I_g_on_n[68] = 2.74e-9; I_g_on_n[69] = 2.74e-9; I_g_on_n[70] = 2.74e-9;
        I_g_on_n[71] = 2.74e-9; I_g_on_n[72] = 2.74e-9; I_g_on_n[73] = 2.74e-9; I_g_on_n[74] = 2.74e-9; I_g_on_n[75] = 2.74e-9; 
        I_g_on_n[76] = 2.74e-9; I_g_on_n[77] = 2.74e-9; I_g_on_n[78] = 2.74e-9; I_g_on_n[79] = 2.74e-9; I_g_on_n[80] = 2.74e-9;
        I_g_on_n[81] = 2.74e-9; I_g_on_n[82] = 2.74e-9; I_g_on_n[83] = 2.74e-9; I_g_on_n[84] = 2.74e-9; I_g_on_n[85] = 2.74e-9; 
        I_g_on_n[86] = 2.74e-9; I_g_on_n[87] = 2.74e-9; I_g_on_n[88] = 2.74e-9; I_g_on_n[89] = 2.74e-9; I_g_on_n[90] = 2.74e-9;
        I_g_on_n[91] = 2.74e-9; I_g_on_n[92] = 2.74e-9; I_g_on_n[93] = 2.74e-9; I_g_on_n[94] = 2.74e-9; I_g_on_n[95] = 2.74e-9; 
        I_g_on_n[96] = 2.74e-9; I_g_on_n[97] = 2.74e-9; I_g_on_n[98] = 2.74e-9; I_g_on_n[99] = 2.74e-9; I_g_on_n[100] = 2.74e-9;
        I_off_n[0] = 1.31e-8; 
        I_off_n[1] = 1.38e-8; I_off_n[2] = 1.47e-8; I_off_n[3] = 1.58e-8; I_off_n[4] = 1.70e-8; I_off_n[5] = 1.82e-8; 
        I_off_n[6] = 1.96e-8; I_off_n[7] = 2.11e-8; I_off_n[8] = 2.27e-8; I_off_n[9] = 2.43e-8; I_off_n[10] = 2.60e-8;
        I_off_n[11] = 2.27e-8; I_off_n[12] = 2.97e-8; I_off_n[13] = 3.19e-8; I_off_n[14] = 3.42e-8; I_off_n[15] = 3.66e-8; 
        I_off_n[16] = 3.92e-8; I_off_n[17] = 4.20e-8; I_off_n[18] = 4.50e-8; I_off_n[19] = 4.81e-8; I_off_n[20] = 5.14e-8;
        I_off_n[21] = 5.50e-8; I_off_n[22] = 5.88e-8; I_off_n[23] = 6.31e-8; I_off_n[24] = 6.76e-8; I_off_n[25] = 7.25e-8; 
        I_off_n[26] = 7.78e-8; I_off_n[27] = 8.33e-8; I_off_n[28] = 8.92e-8; I_off_n[29] = 9.54e-8; I_off_n[30] = 1.02e-7;
        I_off_n[31] = 1.09e-7; I_off_n[32] = 1.17e-7; I_off_n[33] = 1.25e-7; I_off_n[34] = 1.34e-7; I_off_n[35] = 1.44e-7; 
        I_off_n[36] = 1.54e-7; I_off_n[37] = 1.65e-7; I_off_n[38] = 1.77e-7; I_off_n[39] = 1.89e-7; I_off_n[40] = 2.02e-7;
        I_off_n[41] = 2.16e-7; I_off_n[42] = 2.31e-7; I_off_n[43] = 2.48e-7; I_off_n[44] = 2.65e-7; I_off_n[45] = 2.84e-7; 
        I_off_n[46] = 3.05e-7; I_off_n[47] = 3.26e-7; I_off_n[48] = 3.49e-7; I_off_n[49] = 3.73e-7; I_off_n[50] = 3.99e-7;
        I_off_n[51] = 4.28e-7; I_off_n[52] = 4.62e-7; I_off_n[53] = 5.01e-7; I_off_n[54] = 5.42e-7; I_off_n[55] = 5.85e-7; 
        I_off_n[56] = 6.29e-7; I_off_n[57] = 6.73e-7; I_off_n[58] = 7.15e-7; I_off_n[59] = 7.55e-7; I_off_n[60] = 7.91e-7;
        I_off_n[61] = 8.23e-7; I_off_n[62] = 8.51e-7; I_off_n[63] = 8.76e-7; I_off_n[64] = 9.01e-7; I_off_n[65] = 9.25e-7; 
        I_off_n[66] = 9.51e-7; I_off_n[67] = 9.79e-7; I_off_n[68] = 1.01e-6; I_off_n[69] = 1.05e-6; I_off_n[70] = 1.09e-6;
        I_off_n[71] = 1.14e-6; I_off_n[72] = 1.21e-6; I_off_n[73] = 1.29e-6; I_off_n[74] = 1.38e-6; I_off_n[75] = 1.48e-6; 
        I_off_n[76] = 1.59e-6; I_off_n[77] = 1.71e-6; I_off_n[78] = 1.83e-6; I_off_n[79] = 1.96e-6; I_off_n[80] = 2.09e-6;
        I_off_n[81] = 2.25e-6; I_off_n[82] = 2.44e-6; I_off_n[83] = 2.66e-6; I_off_n[84] = 2.90e-6; I_off_n[85] = 3.14e-6; 
        I_off_n[86] = 3.38e-6; I_off_n[87] = 3.60e-6; I_off_n[88] = 3.79e-6; I_off_n[89] = 3.94e-6; I_off_n[90] = 4.04e-6;
        I_off_n[91] = 4.11e-6; I_off_n[92] = 4.18e-6; I_off_n[93] = 4.24e-6; I_off_n[94] = 4.30e-6; I_off_n[95] = 4.35e-6; 
        I_off_n[96] = 4.39e-6; I_off_n[97] = 4.43e-6; I_off_n[98] = 4.46e-6; I_off_n[99] = 4.47e-6; I_off_n[100] = 4.48e-6;
        break;
      case LP_DRAM:
        cout << "ERROR: LP_DRAM for 22nm is undefined" << endl;
        set = false;
        break;
      case COMM_DRAM:
        Vpp = 2.3;
        vdd = 0.9;
        Vdsat = 0.0972;
        Vth = 1.0;    
        Vth_dram = 1.0;
        L_phy = 0.022;
        L_elec = 0.0181;
        W_dram = 0.022;
        t_ox = 3.5e-9;
        c_ox = 9.06e-15;
        mobility_eff = 367.29e8;
        C_g_ideal = 1.99e-16;
        C_fringe = 0.053e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        C_dram_cell = 30e-15;
        I_on_n = 910.5e-6;
        I_on_p = I_on_n/2.0;
        I_on_dram_cell = 20e-6;
        I_off_dram_cell = 1e-15;
        np_ratio = 1.95;
        Rn_channel_on = 1.69*Vpp/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 0.90;
        long_channel_leakage_reduction = 1.0;
        I_off_n[0] = 1.10e-13; 
        I_off_n[1] = 1.17e-13; I_off_n[2] = 1.24e-13; I_off_n[3] = 1.33e-13; I_off_n[4] = 1.42e-13; I_off_n[5] = 1.52e-13; 
        I_off_n[6] = 1.63e-13; I_off_n[7] = 1.74e-13; I_off_n[8] = 1.86e-13; I_off_n[9] = 1.98e-13; I_off_n[10] = 2.11e-13;
        I_off_n[11] = 2.24e-13; I_off_n[12] = 2.39e-13; I_off_n[13] = 2.54e-13; I_off_n[14] = 2.70e-13; I_off_n[15] = 2.88e-13; 
        I_off_n[16] = 3.06e-13; I_off_n[17] = 3.25e-13; I_off_n[18] = 3.45e-13; I_off_n[19] = 3.66e-13; I_off_n[20] = 3.88e-13;
        I_off_n[21] = 4.11e-13; I_off_n[22] = 4.36e-13; I_off_n[23] = 4.62e-13; I_off_n[24] = 4.90e-13; I_off_n[25] = 5.20e-13; 
        I_off_n[26] = 5.51e-13; I_off_n[27] = 5.83e-13; I_off_n[28] = 6.18e-13; I_off_n[29] = 6.53e-13; I_off_n[30] = 6.90e-13;
        I_off_n[31] = 7.29e-13; I_off_n[32] = 7.71e-13; I_off_n[33] = 8.15e-13; I_off_n[34] = 8.61e-13; I_off_n[35] = 9.11e-13; 
        I_off_n[36] = 9.62e-13; I_off_n[37] = 1.02e-12; I_off_n[38] = 1.07e-12; I_off_n[39] = 1.13e-12; I_off_n[40] = 1.19e-12;
        I_off_n[41] = 1.25e-12; I_off_n[42] = 1.32e-12; I_off_n[43] = 1.39e-12; I_off_n[44] = 1.46e-12; I_off_n[45] = 1.54e-12; 
        I_off_n[46] = 1.62e-12; I_off_n[47] = 1.71e-12; I_off_n[48] = 1.79e-12; I_off_n[49] = 1.89e-12; I_off_n[50] = 1.98e-12;
        I_off_n[51] = 2.08e-12; I_off_n[52] = 2.18e-12; I_off_n[53] = 2.30e-12; I_off_n[54] = 2.41e-12; I_off_n[55] = 2.53e-12; 
        I_off_n[56] = 2.66e-12; I_off_n[57] = 2.79e-12; I_off_n[58] = 2.93e-12; I_off_n[59] = 3.07e-12; I_off_n[60] = 3.22e-12;
        I_off_n[61] = 3.37e-12; I_off_n[62] = 3.53e-12; I_off_n[63] = 3.70e-12; I_off_n[64] = 3.88e-12; I_off_n[65] = 4.06e-12; 
        I_off_n[66] = 4.25e-12; I_off_n[67] = 4.45e-12; I_off_n[68] = 4.66e-12; I_off_n[69] = 4.87e-12; I_off_n[70] = 5.09e-12;
        I_off_n[71] = 5.32e-12; I_off_n[72] = 5.56e-12; I_off_n[73] = 5.81e-12; I_off_n[74] = 6.07e-12; I_off_n[75] = 6.34e-12; 
        I_off_n[76] = 6.62e-12; I_off_n[77] = 6.92e-12; I_off_n[78] = 7.22e-12; I_off_n[79] = 7.53e-12; I_off_n[80] = 7.85e-12;
        I_off_n[81] = 8.18e-12; I_off_n[82] = 8.53e-12; I_off_n[83] = 8.89e-12; I_off_n[84] = 9.27e-12; I_off_n[85] = 9.66e-12; 
        I_off_n[86] = 1.01e-11; I_off_n[87] = 1.05e-11; I_off_n[88] = 1.09e-11; I_off_n[89] = 1.13e-11; I_off_n[90] = 1.18e-11;
        I_off_n[91] = 1.23e-11; I_off_n[92] = 1.27e-11; I_off_n[93] = 1.33e-11; I_off_n[94] = 1.38e-11; I_off_n[95] = 1.43e-11; 
        I_off_n[96] = 1.49e-11; I_off_n[97] = 1.54e-11; I_off_n[98] = 1.60e-11; I_off_n[99] = 1.66e-11; I_off_n[100] = 1.72e-11;
        Wmemcella_dram = W_dram;
        Wmemcellpmos_dram = 0.0;
        Wmemcellnmos_dram = 0.0;
        area_cell_dram = 6*0.022*0.022;
        asp_ratio_cell_dram = 0.667;
        break;
      default: 
        cout << "ERROR: Unknown RAM type" << endl; 
        set = false; 
        break;
    }
    sense_amp_delay = 0.03e-9;
    sense_amp_power = 2.16e-15;
    Wmemcella_sram = 1.31 * feature_size*1e-3;
    Wmemcellpmos_sram = 1.23 * feature_size*1e-3;
    Wmemcellnmos_sram = 2.08 * feature_size*1e-3;
    area_cell_sram = 146 * feature_size*1e-3*feature_size*1e-3;
    asp_ratio_cell_sram = 1.46;
    Wmemcella_cam = 1.31 * feature_size*1e-3;
    Wmemcellpmos_cam = 1.23 * feature_size*1e-3;
    Wmemcellnmos_cam = 2.08 * feature_size*1e-3;
    area_cell_cam = 292 * feature_size*1e-3*feature_size*1e-3;
    asp_ratio_cell_cam = 2.92;
    logic_scaling_co_eff = 0.7*0.7*0.7*0.7;
    core_tx_density = 1.25/0.7/0.7;
    sckt_co_eff = 1.1296;
    chip_layout_overhead = 1.2;
    macro_layout_overhead = 1.1;
  }
/* ----- 32nm ----- */
  else if((size>30.0)&&(size<34.0)) // 32nm
  {
    feature_size = 32.0;
  }
/* ----- 45nm ----- */
  else if((size>43.0)&&(size<47.0))
  {
    feature_size = 45.0;

    fringe_cap[0][0] = 0.115e-15;
    miller_value[0][0] = 1.5;
    ild_thickness[0][0] = 0.315;
    horiz_dielectric_constant[0][0] = 1.958;
    vert_dielectric_constant[0][0] = 3.9;
    aspect_ratio[0][0] = 3.0;
    wire_pitch[0][0] = 2.5 * feature_size*1e-3;
    wire_r_per_micron[0][0] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][0],/*wire_thickness*/0.5*aspect_ratio[0][0]*wire_pitch[0][0],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][0] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][0],/*wire_thickness*/0.5*aspect_ratio[0][0]*wire_pitch[0][0],/*wire_spacing*/0.5*wire_pitch[0][0],/*ild_thickness*/ild_thickness[0][0],miller_value[0][0],horiz_dielectric_constant[0][0],vert_dielectric_constant[0][0],/*fringe_cap*/fringe_cap[0][0]);

    fringe_cap[0][1] = 0.115e-15;
    miller_value[0][1] = 1.5;
    ild_thickness[0][1] = 0.315;
    horiz_dielectric_constant[0][1] = 1.958;
    vert_dielectric_constant[0][1] = 3.9;
    aspect_ratio[0][1] = 3.0;
    wire_pitch[0][1] = 4.0 * feature_size*1e-3;
    wire_r_per_micron[0][1] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][1],/*wire_thickness*/0.5*aspect_ratio[0][1]*wire_pitch[0][1],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][1] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][1],/*wire_thickness*/0.5*aspect_ratio[0][1]*wire_pitch[0][1],/*wire_spacing*/0.5*wire_pitch[0][1],/*ild_thickness*/ild_thickness[0][1],miller_value[0][1],horiz_dielectric_constant[0][1],vert_dielectric_constant[0][1],/*fringe_cap*/fringe_cap[0][1]);

    fringe_cap[0][2] = 0.115e-15;
    miller_value[0][2] = 1.5;
    ild_thickness[0][2] = 0.63;
    horiz_dielectric_constant[0][2] = 1.958;
    vert_dielectric_constant[0][2] = 3.9;
    aspect_ratio[0][2] = 3.0;
    wire_pitch[0][2] = 8.0 * feature_size*1e-3;
    wire_r_per_micron[0][2] = wire_resistance(/*BULK_CU_RESISTIVITY*/bulk_copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[0][2],/*wire_thickness*/0.5*aspect_ratio[0][2]*wire_pitch[0][2],/*barrier_thickness*/0,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[0][2] = wire_capacitance(/*wire_width*/0.5*wire_pitch[0][2],/*wire_thickness*/0.5*aspect_ratio[0][2]*wire_pitch[0][2],/*wire_spacing*/0.5*wire_pitch[0][2],/*ild_thickness*/ild_thickness[0][2],miller_value[0][2],horiz_dielectric_constant[0][2],vert_dielectric_constant[0][2],/*fringe_cap*/fringe_cap[0][2]);

    fringe_cap[1][0] = 0.115e-15;
    miller_value[1][0] = 1.5;
    ild_thickness[1][0] = 0.315;
    horiz_dielectric_constant[1][0] = 2.46;
    vert_dielectric_constant[1][0] = 3.9;
    aspect_ratio[1][0] = 2.0;
    wire_pitch[1][0] = 2.5 * feature_size*1e-3;
    wire_r_per_micron[1][0] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][0],/*wire_thickness*/0.5*aspect_ratio[1][0]*wire_pitch[1][0],/*barrier_thickness*/0.004,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[1][0] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][0],/*wire_thickness*/0.5*aspect_ratio[1][0]*wire_pitch[1][0],/*wire_spacing*/0.5*wire_pitch[1][0],/*ild_thickness*/ild_thickness[1][0],miller_value[1][0],horiz_dielectric_constant[1][0],vert_dielectric_constant[1][0],/*fringe_cap*/fringe_cap[1][0]);

    fringe_cap[1][1] = 0.115e-15;
    miller_value[1][1] = 1.5;
    ild_thickness[1][1] = 0.315;
    horiz_dielectric_constant[1][1] = 2.46;
    vert_dielectric_constant[1][1] = 3.9;
    aspect_ratio[1][1] = 2.0;
    wire_pitch[1][1] = 4.0 * feature_size*1e-3;
    wire_r_per_micron[1][1] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][1],/*wire_thickness*/0.5*aspect_ratio[1][1]*wire_pitch[1][1],/*barrier_thickness*/0.004,/*dishing_thickness*/0,/*alpha_scatter*/1.0);
    wire_c_per_micron[1][1] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][1],/*wire_thickness*/0.5*aspect_ratio[1][1]*wire_pitch[1][1],/*wire_spacing*/0.5*wire_pitch[1][1],/*ild_thickness*/ild_thickness[1][1],miller_value[1][1],horiz_dielectric_constant[1][1],vert_dielectric_constant[1][1],/*fringe_cap*/fringe_cap[1][1]);

    fringe_cap[1][2] = 0.115e-15;
    miller_value[1][2] = 1.5;
    ild_thickness[1][2] = 0.55;
    horiz_dielectric_constant[1][2] = 2.104;
    vert_dielectric_constant[1][2] = 3.9;
    aspect_ratio[1][2] = 2.2;
    wire_pitch[1][2] = 8.0 * feature_size*1e-3;
    wire_r_per_micron[1][2] = wire_resistance(/*CU_RESISTIVITY*/copper_resistivity*1e6,/*wire_width*/0.5*wire_pitch[1][2],/*wire_thickness*/0.5*aspect_ratio[1][2]*wire_pitch[1][2],/*barrier_thickness*/0.004,/*dishing_thickness*/0.05*aspect_ratio[1][2]*wire_pitch[1][2],/*alpha_scatter*/1.0);
    wire_c_per_micron[1][2] = wire_capacitance(/*wire_width*/0.5*wire_pitch[1][2],/*wire_thickness*/0.5*aspect_ratio[1][2]*wire_pitch[1][2],/*wire_spacing*/0.5*wire_pitch[1][2],/*ild_thickness*/ild_thickness[1][2],miller_value[1][2],horiz_dielectric_constant[1][2],vert_dielectric_constant[1][2],/*fringe_cap*/fringe_cap[1][2]);

    fringe_cap[1][3] = 0.115e-15;
    wire_pitch[1][3] = 2.0 * feature_size*1e-3;
    wire_r_per_micron[1][3] = 12.0/(0.5*wire_pitch[1][3]);
    wire_c_per_micron[1][3] = 37.5e-15/(256.0*wire_pitch[1][3]);//F/micron

    switch(RAM_tech)
    {
      case HP:
        vdd = 1.0;
        Vdsat = 0.0938;
        Vth = 0.18035;
        L_phy = 0.018;
        L_elec = 0.01345;
        t_ox = 0.65e-9;    
        c_ox = 3.77e-14;
        mobility_eff = 266.68e8;
        C_g_ideal = 6.78e-16;
        C_fringe = 0.05e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 2046.6e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.41;
        Rn_channel_on = 1.51*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 1.38;
        long_channel_leakage_reduction = 1.0/3.546;
        // Leakage curve fitting
        for(int i = 0; i < TEMP_DEGREE_STEPS; i++)
        {
          double z = (double)(i-50.0)/33.0;
          I_g_on_n[i] = 3.59e-8;
          // 10th-order polynomial interpolation for leakage curve fitting
          I_off_n[i] = 5.6e-9*pow(z,10)-9.4e-10*pow(z,9)-2.8e-8*pow(z,8)+4.7e-9*pow(z,7)+4.7e-8*pow(z,6)-8.2e-9*pow(z,5)-3.2e-8*pow(z,4)+6e-9*pow(z,3)+3.6e-8*pow(z,2)+2.3e-7*z+5.7e-7;
        }
        break;
      case LSTP:
        vdd = 1.1;
        Vdsat = 0.0912;
        Vth = 0.50245;
        L_phy = 0.0212;
        L_elec = 0.008;
        t_ox = 1.4e-9;
        c_ox = 2.01e-14;
        mobility_eff = 363.96e8;
        C_g_ideal = 5.18e-16;
        C_fringe = 0.08e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 666.2e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.23;
        Rn_channel_on = 1.99*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 0.99;
        long_channel_leakage_reduction = 1.0/2.08;
        // Leakage curve fitting
        for(int i = 0; i < TEMP_DEGREE_STEPS; i++)
        {
          double z = (double)(i-50.0)/33.0;
          I_g_on_n[i] = 9.47e-12;
          // 10th-order polynomial interpolation for leakage curve fitting
          I_off_n[i] = 1.3e-12*pow(z,10)+1.6e-12*pow(z,9)-5.1e-12*pow(z,8)-6.9e-12*pow(z,7)+4.2e-12*pow(z,6)+5.6e-12*pow(z,5)-1.1e-12*pow(z,4)+1.3e-11*pow(z,3)+5.9e-11*pow(z,2)+1.1e-10*z+9e-11;
        }
        break;
      case LOP:
        vdd = 0.7;
        Vdsat = 0.0571;
        Vth = 0.22599;
        L_phy = 0.022;
        L_elec = 0.016;
        t_ox = 0.9e-9;
        c_ox = 2.82e-14;
        mobility_eff = 508.9e8;
        C_g_ideal = 6.2e-16;
        C_fringe = 0.073e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        I_on_n = 748.9e-6;
        I_on_p = I_on_n/2.0;
        np_ratio = 2.28;
        Rn_channel_on = 1.76*vdd/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 1.11;
        long_channel_leakage_reduction = 1.0/1.92;
        // Leakage curve fitting
        for(int i = 0; i < TEMP_DEGREE_STEPS; i++)
        {
          double z = (double)(i-50.0)/33.0;
          I_g_on_n[i] = -2.1e-9*pow(z,10)-1.4e-9*pow(z,9)+9.5e-9*pow(z,8)+6.7e-9*pow(z,7)-1.4e-8*pow(z,6)-1.1e-8*pow(z,5)+4.7e-9*pow(z,4)+3.7e-9*pow(z,3)+7.4e-9*pow(z,2)+4.5e-8*z+8.4e-8;
          // 10th-order polynomial interpolation for leakage curve fitting
          I_off_n[i] = -2.7e-10*pow(z,10)-5.6e-11*pow(z,9)+1.3e-9*pow(z,8)+2.7e-10*pow(z,7)-2.1e-9*pow(z,6)-3.5e-10*pow(z,5)+9.8e-10*pow(z,4)-2.8e-10*pow(z,3)+7e-10*pow(z,2)+6.1e-9*z+1.1e-8;
        }
        break;
      case LP_DRAM:
        Vpp = 1.5;
        vdd = 1.1;
        Vdsat = 0.181;
        Vth = 0.44559;
        Vth_dram = 0.44559;
        L_phy = 0.078;
        L_elec = 0.0504;
        W_dram = 0.079;
        t_ox = 2.1e-9;
        c_ox = 1.41e-14;
        mobility_eff = 426.30e8;
        C_g_ideal = 1.10e-15;
        C_fringe = 0.08e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        C_dram_cell = 20e-15;
        I_on_n = 456e-6;    
        I_on_p = I_on_n/2.0;
        I_on_dram_cell = 36e-6;
        I_off_dram_cell = 19.5e-12;
        np_ratio = 2.05;
        Rn_channel_on = 1.65*Vpp/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 0.90;
        long_channel_leakage_reduction = 1.0;
        // Leakage curve fitting
        for(int i = 0; i < TEMP_DEGREE_STEPS; i++)
        {
          double z = (double)(i-50.0)/33.0;
          //I_off_n[i] = ;
        }
        Wmemcella_dram = W_dram;
        Wmemcellpmos_dram = 0.0;
        Wmemcellnmos_dram = 0.0;
        area_cell_dram = W_dram*L_phy*10.0;        
        asp_ratio_cell_dram = 1.46;
        break;
      case COMM_DRAM:
        Vpp = 2.7;
        vdd = 1.1;
        Vdsat = 0.147;
        Vth = 1.0;
        Vth_dram = 1.0;
        L_phy = 0.045;
        L_elec = 0.0298;    
        W_dram = 0.045;
        t_ox = 4e-9;
        c_ox = 7.98e-15;
        mobility_eff = 368.58e8;
        C_g_ideal = 3.59e-16;
        C_fringe = 0.08e-15;
        C_junc = 1e-15;
        C_junc_sidewall = 0.25e-15;
        C_dram_cell = 30e-15;
        I_on_n = 999.4e-6;
        I_on_p = I_on_n/2.0;
        I_on_dram_cell = 20e-6;
        I_off_dram_cell = 1e-15;
        np_ratio = 1.95;
        Rn_channel_on = 1.69*Vpp/I_on_n;
        Rp_channel_on = np_ratio*Rn_channel_on;
        gmp_to_gmn_multiplier = 0.90;
        long_channel_leakage_reduction = 1.0;
        // Leakage curve fitting
        for(int i = 0; i < TEMP_DEGREE_STEPS; i++)
        {
          double z = (double)(i-50.0)/33.0;
          //I_off_n[i] = ;
        }
        Wmemcella_dram = W_dram;
        Wmemcellpmos_dram = 0.0;
        Wmemcellnmos_dram = 0.0;
        area_cell_dram = 6*0.022*0.022;
        asp_ratio_cell_dram = 1.5;
        break;
      default:
        cout << "ERROR: Unknown RAM type" << endl;
        set = false;
        break;
    }
    sense_amp_delay = 0.04e-9;
    sense_amp_power = 2.7e-15;
    Wmemcella_sram = 1.31 * feature_size*1e-3;
    Wmemcellpmos_sram = 1.23 * feature_size*1e-3;
    Wmemcellnmos_sram = 2.08 * feature_size*1e-3;
    area_cell_sram = 146 * feature_size*1e-3*feature_size*1e-3;
    asp_ratio_cell_sram = 1.46;
    Wmemcella_cam = 1.31 * feature_size*1e-3;
    Wmemcellpmos_cam = 1.23 * feature_size*1e-3;
    Wmemcellnmos_cam = 2.08 * feature_size*1e-3;
    area_cell_cam = 292 * feature_size*1e-3*feature_size*1e-3;
    asp_ratio_cell_cam = 2.92;
    logic_scaling_co_eff = 0.7*0.7;
    core_tx_density = 1.25;
    sckt_co_eff = 1.1387;
    chip_layout_overhead = 1.2;
    macro_layout_overhead = 1.1;
  }
  else if((size>63.0)&&(size<68.0))
  {
    feature_size = 65.0;
  }
  else if((size>88.0)&&(size<92.0))
  {
    feature_size = 90.0;
  }
  else
  {
    cout << "ERROR: invalid technology size" << endl;
    set = false;
  }
}


/************************************************
* Update floorplan area information by the area *
* estimation from power libraries		*
************************************************/
void Power::updateFloorplanAreaInfo(int fid, double area)
{
  // find the underlying floorplan
  map<int,floorplan_t>::iterator fit = p_chip.floorplan.find(fid);

  // Floorplanning error - heuristic floorplanning is not supported
  if(fit == p_chip.floorplan.end())
    std::cout << "ERROR: No matching floorplan is found" << std::endl;

  // update the floorplan area information - should be used for error checking (user input vs estimation)
  (*fit).second.area_estimate += area;

}

/*****************************************************
* compute temperature based on floorplan information *
******************************************************/
void Power::compute_temperature(ComponentId_t compID)
{
  boost::mpi::communicator world;
  
  std::cout << " I entered compute_temperature " << std::endl;

  if (p_tempMonitor == true && ((world.size() > 1 && p_SumNumCompNeedPower == 0) || (world.size() == 1 && p_NumCompNeedPower == 0) || p_ifParser ==true))
  {   ////std::cout << " world.size = " << world.size() << ", numCompNeedPower = " << p_NumCompNeedPower << std::endl;
  //first resume numCompNeedPower for next power updates
  p_NumCompNeedPower = chip.num_comps;
  p_TempSumNumCompNeedPower = chip.sumnum_comps;
  p_hasUpdatedTemp = true;

  //Perform DPM
  dynamic_power_management();

  //compute temperature
  p_chip.thermal_library->compute(&p_chip.floorplan);
  
 
  //map<pseudo_unit_id_t,pseudo_unit_t>::iterator uit;
  map<int,floorplan_t>::iterator fit;
  multimap<ptype,int>::iterator it;

#if 0

  for (it=subcompList.begin(); it!=subcompList.end(); it++)
  { 
    I updatedLeakagePower=0.0;
    fit = p_chip.floorplan.find((*it).second);
  
        ////cout << "ENERGY_INTERFACE_DEBUG: component ID " << compID << " subcompList size =" << subcompList.size() << ", comp type = " << (*it).first << " on fp_id " << (*it).second << ", feedback = " << (*fit).second.leakage_feedback <<endl;
   

    if( fit != p_chip.floorplan.end() && (*fit).second.leakage_feedback)
    {
      
        ////cout << "ENERGY_INTERFACE_DEBUG: updating component ID subcomp " << (*it).first << " with leakage feedback" << endl;
     
      switch((*it).first)
      {
	case CACHE_IL1:
	    leakage_feedback(p_powerModel.il1, (*fit).second.device_tech, (*it).first);
	    // get the updated unit energy after leakage feedback
      	    updatedLeakagePower = (I)icache.power.readOp.leakage + (I)icache.power.readOp.gate_leakage;
	    //using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage after feedback = " << updatedLeakagePower << " W on fp_id " << (*it).second << std::endl;
	break;	
	case CACHE_DL1:
	    leakage_feedback(p_powerModel.dl1, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)dcache.power.readOp.leakage + (I)dcache.power.readOp.gate_leakage;
	break;
	case CACHE_ITLB:
	    leakage_feedback(p_powerModel.itlb, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)itlb->power.readOp.leakage + (I)itlb->power.readOp.gate_leakage;
	break;
	case CACHE_DTLB:
	    leakage_feedback(p_powerModel.dtlb, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)dtlb->power.readOp.leakage + (I)dtlb->power.readOp.gate_leakage;
	break;
	case CLOCK:
	break;
	case BPRED:
	    if (bpred_tech.prediction_width > 0){
	       leakage_feedback(p_powerModel.bpred, (*fit).second.device_tech, (*it).first); 
      	       updatedLeakagePower = (I)BPT->power.readOp.leakage + (I)BPT->power.readOp.gate_leakage;	   
	    }
	break;
	case RF:
	    leakage_feedback(p_powerModel.rf, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)rfu->power.readOp.leakage + (I)rfu->power.readOp.gate_leakage;
	//using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage after feedback = " << updatedLeakagePower << " W on fp_id " << (*it).second << std::endl;
	break;
	case IO:
	break;
	case LOGIC:
	break;
	case EXEU_ALU:
	    leakage_feedback(p_powerModel.alu, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)exeu->power.readOp.leakage + (I)exeu->power.readOp.gate_leakage;
	break;
	case EXEU_FPU:
	    leakage_feedback(p_powerModel.fpu, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)fp_u->power.readOp.leakage + (I)fp_u->power.readOp.gate_leakage;
	break;
	case MULT:
	break;
	case 14: //IB
	    leakage_feedback(p_powerModel.ib, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)IB->power.readOp.leakage + (I)IB->power.readOp.gate_leakage;
	break;
	case ISSUE_Q:
	break;
	case INST_DECODER:
	    leakage_feedback(p_powerModel.decoder, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I) ((bool)core_tech.core_long_channel? 
				(ID_inst->power.readOp.longer_channel_leakage +
				  ID_operand->power.readOp.longer_channel_leakage +
				  ID_misc->power.readOp.longer_channel_leakage):
				(ID_inst->power.readOp.leakage +
				  ID_operand->power.readOp.leakage +
				  ID_misc->power.readOp.leakage))  + 
			   	  (I)(ID_inst->power.readOp.gate_leakage +
				ID_operand->power.readOp.gate_leakage +
				ID_misc->power.readOp.gate_leakage) ;

	break;
	case BYPASS:
	    leakage_feedback(p_powerModel.bypass, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)bypass.power.readOp.leakage + (I)bypass.power.readOp.gate_leakage;
	break;
	case EXEU:
	break;
	case PIPELINE:
	    leakage_feedback(p_powerModel.pipeline, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)corepipe->power.readOp.leakage + (I)corepipe->power.readOp.gate_leakage;
	break;
	case 20: //LSQ
	    leakage_feedback(p_powerModel.lsq, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)LSQ->power.readOp.leakage + (I)LSQ->power.readOp.gate_leakage;
	break;
	case RAT:
	break;
	case 22:  //ROB
	break;
	case 23:  //BTB
	    if (bpred_tech.prediction_width > 0){
	        leakage_feedback(p_powerModel.btb, (*fit).second.device_tech, (*it).first);
      	        updatedLeakagePower = (I)BTB->power.readOp.leakage + (I)BTB->power.readOp.gate_leakage;
	    }
	break;
	case CACHE_L2:
	    leakage_feedback(p_powerModel.L2, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)l2array->power.readOp.leakage + (I)l2array->power.readOp.gate_leakage;
	break;
	case MEM_CTRL:
	    leakage_feedback(p_powerModel.mc, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)mc->power.readOp.leakage + (I)mc->power.readOp.gate_leakage;
	break;
	case ROUTER:
	    leakage_feedback(p_powerModel.router, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)nocs->power.readOp.leakage + (I)nocs->power.readOp.gate_leakage;
	    //using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage after feedback = " << updatedLeakagePower << " W on fp_id " << (*it).second << std::endl;
	break;
	case LOAD_Q:
	    leakage_feedback(p_powerModel.loadQ, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)LoadQ->power.readOp.leakage + (I)LoadQ->power.readOp.gate_leakage;
	break;
	case RENAME_U:
	    ////using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage = " << rnu->power.readOp.leakage << " W; gate= " << rnu->power.readOp.gate_leakage << std::endl;

	    leakage_feedback(p_powerModel.rename, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)rnu->power.readOp.leakage + (I)rnu->power.readOp.gate_leakage;
	break;
	case SCHEDULER_U:
	    ////using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage = " << scheu->power.readOp.leakage << " W; gate= " << scheu->power.readOp.gate_leakage << std::endl;

	    leakage_feedback(p_powerModel.scheduler, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)scheu->power.readOp.leakage + (I)scheu->power.readOp.gate_leakage;
	break;
	case CACHE_L3:
	    leakage_feedback(p_powerModel.L3, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)l3array->power.readOp.leakage + (I)l3array->power.readOp.gate_leakage;
	break;
	case CACHE_L1DIR:
	    leakage_feedback(p_powerModel.L1dir, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)l1dirarray->power.readOp.leakage + (I)l1dirarray->power.readOp.gate_leakage;
	break;
	case CACHE_L2DIR:
	    leakage_feedback(p_powerModel.L2dir, (*fit).second.device_tech, (*it).first);
      	    updatedLeakagePower = (I)l2dirarray->power.readOp.leakage + (I)l2dirarray->power.readOp.gate_leakage;
	break;
	default: break;
      }

       // display the updated unit energy
        ////using namespace io_interval;  std::cout << "ENERGY_INTERFACE_DEBUG: CompID " << compID << ", subcomp type " << (*it).first << ", leakage after feedback = " << updatedLeakagePower << " W on fp_id " << (*it).second << std::endl;
      

      // update the floorplan leakage
      //(*fit).second.power.second = (*fit).second.power.second + (*uit).second.power;
      (*fit).second.p_usage_floorplan.currentPower += updatedLeakagePower;
	//using namespace io_interval;  std::cout << "Before: CompID " << compID << ", subcomp type " << (*it).first << ", leakage = " << (*fit).second.p_usage_floorplan.leakagePower << " W on fp_id " << (*fit).first << std::endl;
      (*fit).second.p_usage_floorplan.leakagePower += updatedLeakagePower;
	//using namespace io_interval;  std::cout << "After: CompID " << compID << ", subcomp type " << (*it).first << ", leakage = " << (*fit).second.p_usage_floorplan.leakagePower << " W on fp_id " << (*fit).first << std::endl;
      (*fit).second.p_usage_floorplan.totalEnergy += updatedLeakagePower;
	
      // Important! the overall comp total energy should be updated as well--->this fix seems buggy
      //p_usage_uarch.totalEnergy += updatedLeakagePower;
    } // end if leakage feedback   
  } // end for each subcomp

#endif
    
    //compute failure rate of each thermal block is now done offline at the end of simulation
    //here we store each floorplan temperature into a database. TODO: mpi reduce to get the min MTTF of the system.
    for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
    {
	//getFailureRate((*fit).second.device_tech.temperature);
	//std::cout << " total failure rate = " << p_TotalFailureRate << std::endl;
	(*fit).second.TDB.push_back( (*fit).second.device_tech.temperature );
	std::cout << "fit.TDB.size() = " << (*fit).second.TDB.size() << ", temp = " << (*fit).second.device_tech.temperature <<std::endl;
    }
  } //end if model temperature
}

/*****************************************************************
* leakage_feedback
* pass the new I_xx resulted from leakage feedback to the power
* library to get the new unit power 
******************************************************************/
void Power::leakage_feedback(pmodel power_model, parameters_tech_t device_tech, ptype power_type)
{
  switch(power_model)
  {
     case McPAT:
      switch(power_type)
      {
	  case CACHE_IL1:
	    icache.SSTleakage_feedback(device_tech.temperature);
	  break;	
	  case CACHE_DL1:
	    dcache.SSTleakage_feedback(device_tech.temperature);
	  break;
	  case CACHE_ITLB:
	    itlb->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case CACHE_DTLB:
	    dtlb->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case CLOCK:
	  break;
	  case BPRED:
	    BPT->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case RF:
	    rfu->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case IO:
	  break;
	  case LOGIC:
	  break;
	  case EXEU_ALU:
	    exeu->SSTleakage_feedback(device_tech.temperature);	    
	  break;
	  case EXEU_FPU:
	    fp_u->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case MULT:
	  break;
	  case 14: //IB
	    IB->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case ISSUE_Q:
	  break;
	  case INST_DECODER:
	    ID_inst->SSTleakage_feedback(device_tech.temperature);
	    ID_operand->SSTleakage_feedback(device_tech.temperature);
	    ID_misc->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case BYPASS:
	    //exu->SSTleakage_feedback(device_tech.temperature); not implement yet
	  break;
	  case EXEU:
	  break;
	  case PIPELINE:
	    //corepipe->SSTleakage_feedback(device_tech.temperature); not implement yet
	  break;
	  case 20:  //LSQ
	    LSQ->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case RAT:
	    //RAT->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case 22: //ROB
	    //ROB->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case 23:  //BTB
	    BTB->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case CACHE_L2:
	    //TODO: multiple l2 
	  break;
	  case MEM_CTRL:
	  break;
	  case ROUTER:
	    nocs->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case LOAD_Q:
	    LoadQ->SSTleakage_feedback(device_tech.temperature);
	  break;
	  case RENAME_U:
	    //rnu->SSTleakage_feedback(device_tech.temperature); not implement yet
	  break;
	  case SCHEDULER_U:
	    //scheu->SSTleakage_feedback(device_tech.temperature); not implement yet
	  break;
	  case CACHE_L3:
	    //TODO: multiple l3
	  break;
	  case CACHE_L1DIR:
	    //TODO: multiple l1dir
	  break;
	  case CACHE_L2DIR:
	    //TODO: multiple l2dir
	  break;
	  default: break;
      } // end switch ptype
      break;
      case IntSim:
	#ifdef INTSIM_H
	switch(power_type)
        {
	  case CACHE_IL1:
	    intsim_il1->leakage_feedback(device_tech);
	  break;	
	  case CACHE_DL1:
	    intsim_dl1->leakage_feedback(device_tech);
	  break;
	  case CACHE_ITLB:
	    intsim_itlb->leakage_feedback(device_tech);
	  break;
	  case CACHE_DTLB:
	    intsim_dtlb->leakage_feedback(device_tech);
	  break;
	  case CLOCK:
	    intsim_clock->leakage_feedback(device_tech);
	  break;
	  case BPRED:
	    intsim_bpred->leakage_feedback(device_tech);
	  break;
	  case RF:
	    intsim_rf->leakage_feedback(device_tech);
	  break;
	  case IO:
	    intsim_io->leakage_feedback(device_tech);
	  break;
	  case LOGIC:
	    intsim_logic->leakage_feedback(device_tech);
	  break;
	  case EXEU_ALU:
	    intsim_alu->leakage_feedback(device_tech);	    
	  break;
	  case EXEU_FPU:
	    intsim_fpu->leakage_feedback(device_tech);
	  break;
	  case MULT:
	    intsim_mult->leakage_feedback(device_tech);
	  break;
	  case 14: //IB
	    intsim_ib->leakage_feedback(device_tech);
	  break;
	  case ISSUE_Q:
	    intsim_issueQ->leakage_feedback(device_tech);
	  break;
	  case INST_DECODER:
	    intsim_decoder->leakage_feedback(device_tech);
	  break;
	  case BYPASS:
	    intsim_bypass->leakage_feedback(device_tech);
	  break;
	  case EXEU:
	    intsim_exeu->leakage_feedback(device_tech);
	  break;
	  case PIPELINE:
	    intsim_pipeline->leakage_feedback(device_tech);
	  break;
	  case 20:  //LSQ
	    intsim_lsq->leakage_feedback(device_tech);
	  break;
	  case RAT:
	    intsim_rat->leakage_feedback(device_tech);
	  break;
	  case 22: //ROB
	    intsim_rob->leakage_feedback(device_tech);
	  break;
	  case 23:  //BTB
	    intsim_btb->leakage_feedback(device_tech);
	  break;
	  case CACHE_L2:
	    intsim_L2->leakage_feedback(device_tech);
	  break;
	  case MEM_CTRL:
	    intsim_mc->leakage_feedback(device_tech);
	  break;
	  case ROUTER:
	    intsim_router->leakage_feedback(device_tech);
	  break;
	  case LOAD_Q:
	    intsim_loadQ->leakage_feedback(device_tech);
	  break;
	  case RENAME_U:
	    intsim_rename->leakage_feedback(device_tech);
	  break;
	  case SCHEDULER_U:
	    intsim_scheduler->leakage_feedback(device_tech);
	  break;
	  case CACHE_L3:
	    intsim_L3->leakage_feedback(device_tech);
	  break;
	  case CACHE_L1DIR:
	    intsim_L1dir->leakage_feedback(device_tech);
	  break;
	  case CACHE_L2DIR:
	    intsim_L2dir->leakage_feedback(device_tech);
	  break;
	  default: break;
      } // end switch ptype
	#endif
      break;
      default:
      break;
  }//end switch pmodel
}



void Power::printFloorplanAreaInfo()
{
  map<int,floorplan_t>::iterator fit;

  for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
  {
	std::cout<<"floorplan id " <<(*fit).second.id<<" has estimated area " <<(*fit).second.area_estimate*1e-6<< " mm^2" << std::endl; 
  }
}

void Power::printFloorplanPowerInfo()
{
  map<int,floorplan_t>::iterator fit;

  for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
  {
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has current total power = " << (*fit).second.p_usage_floorplan.currentPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has leakage power = " << (*fit).second.p_usage_floorplan.leakagePower << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has runtime power = " << (*fit).second.p_usage_floorplan.runtimeDynamicPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has TDP = " << (*fit).second.p_usage_floorplan.TDP << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has total energy = " << (*fit).second.p_usage_floorplan.totalEnergy << " J" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has peak power = " << (*fit).second.p_usage_floorplan.peak << " J" << std::endl;
  }
}

void Power::printFloorplanThermalInfo()
{
    map<int,floorplan_t>::iterator fit;

    for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
    {
        std::cout <<"floorplan id " <<(*fit).second.id<<" has temperature = " << (*fit).second.device_tech.temperature << " K" << std::endl;
    }
}

void Power::compute_MTTF()
{
	Reliability *r;
	double t_min, t_max, t_avg, freq;
	double temp_TTF, total_TTF = 0;
	double minTTF = 99999;
	double maxTTF = 0;
	double min1TTF = 99999; //min TTF of the 4 cores on the 1st chip
	double min2TTF = 99999; //min TTF of the 4 cores on the 2nd chip
	int i, j, k, TDBsize = 0;
	int iterations = 1000;
	map<int,floorplan_t>::iterator fit;
	

	r = new Reliability();
	
	//p_TotalFailureRate += r->compute_failurerate(temp, t_min, t_max, t_avg, freq, false);
	//p_NumSamples += 1;

	fit = p_chip.floorplan.begin();
	TDBsize = (*fit).second.TDB.size();
	std::vector<double> systemMTTF(TDBsize);
	std::cout << "TDB.size = " << TDBsize << ", floorplan id = " << (*fit).second.id << std::endl;
	getTemperatureStatistics();

	// for each time instance
	for (j=0; j < TDBsize; j++){
            std::cout << "j = " << j << ", TDB.size = " << TDBsize << std::endl;
	    //Monte-Carlo
	    for (i=0; i<iterations; i++){
		k = 0; //block id

	        //calculate global(system-wide) TTF 
	        for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
	        {
	           temp_TTF = r->compute_localMinTTF((*fit).second.TDB.at(j) , p_minTemperature.at(k), p_maxTemperature.at(k), p_maxTemperature.at(k) - p_minTemperature.at(k), p_thermalFreq[k][j], false);
		   (*fit).second.TTF += temp_TTF;


	           
		   if (k >=0 && k <=3) //cores on the 1st chip
		   {
		        if (min1TTF > temp_TTF)
		            min1TTF = temp_TTF;
			if (minTTF > temp_TTF)
		            minTTF = temp_TTF;
		        if (maxTTF < temp_TTF)
		            maxTTF = temp_TTF;
		   }
		   if (k >=8 && k <=11) //cores on the 2nd chip
		   {
			if (min2TTF > temp_TTF)
		            min2TTF = temp_TTF;
			if (minTTF > temp_TTF)
		            minTTF = temp_TTF;
		        if (maxTTF < temp_TTF)
		            maxTTF = temp_TTF;
		   }

		   //proceed to the next block
		    k++;
    	        }

		//Min-Max Method
		switch(p_systemTopology) 
      		{
		    case SERIES:	
	               total_TTF = total_TTF + minTTF;
		    break;
		    case SERIES_PARALLEL:
			if (min1TTF >= min2TTF)
		       	    total_TTF = total_TTF + min1TTF;
			else
		       	    total_TTF = total_TTF + min2TTF;
		    break;		
		    case PARALLEL:
		       total_TTF = total_TTF + maxTTF;
		    break;		
		}
		minTTF = 99999; //reset
		min1TTF = 99999; //reset
		min2TTF = 99999; //reset
		maxTTF = 0; //reset
	    }

	    systemMTTF.at(j) = (total_TTF/iterations);
            std::cout << "total_TTF = " << total_TTF << std::endl;
	    total_TTF = 0; //reset

	    std::cout << "At time step " << j << ", system MTTF = " << systemMTTF.at(j) << std::endl;
	    
            //print individual block(core)'s MTTF
	    k = 0; //block id
	    for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
	    {
		std::cout << "Block id " << k << " 's MTTF = " << (*fit).second.TTF/iterations << std::endl;
		k++;
		(*fit).second.TTF = 0; //reset
	    }
	} //end for each time instance
	delete r;

}

void Power::getTemperatureStatistics()
{
    map<int,floorplan_t>::iterator fit;
    int i = 0; //block id
    int j; // time instance
    int freq, TDBsize = 0, numFloorplan = 0;
    double minTemp = 99999, maxTemp = 0;
    double upperband = 0, lowerband = 0;
    int upcycle, downcycle, cyclesdown, cyclesup;

    fit = p_chip.floorplan.begin();
    TDBsize = (*fit).second.TDB.size();
    numFloorplan = p_chip.floorplan.size();

    p_minTemperature.resize( numFloorplan );
    p_maxTemperature.resize( numFloorplan );
    p_thermalFreq.resize( numFloorplan , vector<double>( TDBsize , 0 ) );

    for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
    {
	// go over each time instance to get min/max temperature
	for (j=0; j < TDBsize; j++){
	   if (minTemp > (*fit).second.TDB.at(j))
	        minTemp = (*fit).second.TDB.at(j);
	   if (maxTemp < (*fit).second.TDB.at(j))
	        maxTemp = (*fit).second.TDB.at(j);
	}
	p_minTemperature.at(i) = minTemp;
	p_maxTemperature.at(i) = maxTemp;
	std::cout << "min temp at block " << i << " is " << p_minTemperature.at(i) << std::endl;
	std::cout << "max temp at block " << i << " is " << p_maxTemperature.at(i) << std::endl;
	

	//compute thermal cycles for the block
        upperband=minTemp + ((maxTemp - minTemp)*0.80);
        lowerband=minTemp + ((maxTemp - minTemp)*0.20);
	////std::cout << "upper band temp at block " << i << " is " << upperband << std::endl;
	////std::cout << "lower band temp at block " << i << " is " << lowerband << std::endl;

        upcycle=0; downcycle=0; //0-1 variables
        cyclesdown=0; cyclesup=0; //variables to compute number of cycles
	minTemp = 99999; maxTemp = 0; //re-set the values for the next block
        for (j=0; j < TDBsize; j++) {
            if ((*fit).second.TDB.at(j) == 0) break;  
            if ((*fit).second.TDB.at(j) >= upperband && upcycle==0) {
                  upcycle=1;
                  downcycle=0;
                  cyclesup++;
            }
            else if ((*fit).second.TDB.at(j) <= lowerband && downcycle==0) {
                  upcycle=0;
                  downcycle=1;
                  cyclesdown++;
            }
        
            if (cyclesdown < cyclesup)
                p_thermalFreq[i][j] = cyclesdown/((j+1)*0.0001);  //assume temperature is gathered every 100us
            else
                p_thermalFreq[i][j] = cyclesup/((j+1)*0.0001);
	    ////std::cout << "cycling freq at block " << i << " and time " << j << " is " << p_thermalFreq[i][j] << std::endl;
	    ////std::cout << "cyclesdown is " << cyclesdown << ", and cyclesup = " << cyclesup << " division = " << (j+1)*0.0001 << std::endl;
	}
	i++;

    }
                 
}


/*void Power::compute_MTTF()
{
	std::cout << "MTTF = " << p_TotalFailureRate/p_NumSamples << std::endl;

}*/


// DPM
void Power::setupDPM(int block_id, power_state pstate)
{
    map<int,floorplan_t>::iterator fit;

    fit = p_chip.floorplan.find(block_id);

    if( fit != p_chip.floorplan.end())
        (*fit).second.pstate = pstate;

}

void Power::dynamic_power_management()
{
    map<int,floorplan_t>::iterator fit;

  for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
  {
	//reset the original value of total energy at the previous step
	(*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							- (*fit).second.p_usage_floorplan.runtimeDynamicPower
							- (*fit).second.p_usage_floorplan.leakagePower;

	if ((*fit).second.pstate == P_SLEEPING)
	    (*fit).second.p_usage_floorplan.runtimeDynamicPower = (I)0.05;
	else if ((*fit).second.pstate == P_GOTOSLEEP)
	    (*fit).second.p_usage_floorplan.runtimeDynamicPower = (I)10.0;
	else if ((*fit).second.pstate == P_WAKEUP)
	    (*fit).second.p_usage_floorplan.runtimeDynamicPower = (I)10.0;
	//the new current power/total power after DPM
	(*fit).second.p_usage_floorplan.currentPower = (*fit).second.p_usage_floorplan.runtimeDynamicPower
							+ (*fit).second.p_usage_floorplan.leakagePower;
	(*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							+ (*fit).second.p_usage_floorplan.currentPower;

	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has runtime power = " << (*fit).second.p_usage_floorplan.runtimeDynamicPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" has leakage power = " << (*fit).second.p_usage_floorplan.leakagePower << " W" << std::endl;
	using namespace io_interval; std::cout <<"floorplan id " <<(*fit).second.id<<" current power = " << (*fit).second.p_usage_floorplan.currentPower << " W" << std::endl;	
  }

}


void Power::calibrate_for_clovertown(IntrospectedComponent *c)
{
    map<int,floorplan_t>::iterator fit;
    unsigned i = 0;
    unsigned j = 0;
    I executionTime = 1.0;
    executionTime = getExecutionTime(c);

  using namespace io_interval;  std::cout << "executionTime = " << executionTime << std::endl;
  std::cout << "number of cores: " << p_Mp1->sys.number_of_cores << ", number of L2 " << p_Mp1->sys.number_of_L2s << std::endl;

  for(fit = p_chip.floorplan.begin(); fit != p_chip.floorplan.end(); fit++)
  {
    ////std::cout << " i = " << i << ", j = " << j << std::endl;
    //cores
    if (((*fit).second.id >=0 && (*fit).second.id <=3) || ((*fit).second.id >=8 && (*fit).second.id <=11)){
	//reset the original value of total energy at the previous step
	(*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							- (*fit).second.p_usage_floorplan.runtimeDynamicPower
							- (*fit).second.p_usage_floorplan.leakagePower;

	p_Mcore = p_Mproc.SSTreturnCore(i); //return core i

	//calibrate the rumtime power [power = power*calibrate rate:7.76] 
        std::cout << "for core " << i << ", rt_power = " << p_Mcore->rt_power.readOp.dynamic << std::endl;
	(*fit).second.p_usage_floorplan.runtimeDynamicPower = (I)p_Mcore->rt_power.readOp.dynamic *7.76 / executionTime;

	//calculate leakage power based on the industry model (0.5W/mm2; 36mm2 per core; leakage feedback)
	(*fit).second.p_usage_floorplan.leakagePower = (I)0.5 * 36.0 * exp(0.017*((*fit).second.device_tech.temperature-383));

         //the new current power/total power 
	(*fit).second.p_usage_floorplan.currentPower = (*fit).second.p_usage_floorplan.runtimeDynamicPower
							+ (*fit).second.p_usage_floorplan.leakagePower;
	
        (*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							+ (*fit).second.p_usage_floorplan.currentPower;
	i++; //proceed to the next core
    }

    //L2 caches
    else if (((*fit).second.id >=4 && (*fit).second.id <=5) || ((*fit).second.id >=12 && (*fit).second.id <=13)){
        //reset the original value of total energy at the previous step
	(*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							- (*fit).second.p_usage_floorplan.runtimeDynamicPower
							- (*fit).second.p_usage_floorplan.leakagePower;

	l2array = p_Mproc.SSTreturnL2(j); //return L2 j

	//dynamic rumtime power  
	(*fit).second.p_usage_floorplan.runtimeDynamicPower = (I)l2array->rt_power.readOp.dynamic / executionTime;

	//leakage power
	(*fit).second.p_usage_floorplan.leakagePower = (I)l2array->power.readOp.longer_channel_leakage + (I)l2array->power.readOp.gate_leakage;

         //the new current power/total power 
	(*fit).second.p_usage_floorplan.currentPower = (*fit).second.p_usage_floorplan.runtimeDynamicPower
							+ (*fit).second.p_usage_floorplan.leakagePower;
	
        (*fit).second.p_usage_floorplan.totalEnergy = (*fit).second.p_usage_floorplan.totalEnergy
							+ (*fit).second.p_usage_floorplan.currentPower;
	j++; //proceed to the next L2
    }
	
  }

}


}  // namespace SST


