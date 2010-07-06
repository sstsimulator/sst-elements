#include "power.h"
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

 




namespace SST {

/*********************
* Power:: constructor*
**********************/
Power::Power()
{
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

    

  if(p_ifReadEntireXML == false){
    //Save computational time for calls to McPAT. For McPAT's case, XML is read in once and all the params
    // are set up during the 1st setTech call. So there is no need to read the XML again if it has done so.

    //First, set up device parameter values
    setTech(params);

    //Then, set up architecture parameter values
    Component::Params_t::iterator it= params.begin();

    while (it != params.end()){
	//NOTE: params are NOT read in order as they apprears in XML
        if (!it->first.compare("power_monitor")){
	    if (!it->second.compare("NO"))
		p_powerMonitor = false;
	    else
		p_powerMonitor = true;
	}
	else if (!it->first.compare("power_level")){ //lv2
	        sscanf(it->second.c_str(), "%d", &p_powerLevel);
	}
	else if (!it->first.compare("McPAT_XMLfile")){ //Mc
	        p_McPATxmlpath = &it->second[0];
		//sscanf(it->second.c_str(), "%s", p_McPATxmlpath);
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
			else if(!it->first.compare("clock_skew")){ //lv2
				sscanf(it->second.c_str(), "%lf", &clock_tech.skew);
			}
			else if(!it->first.compare("clock_chip_area")){
				sscanf(it->second.c_str(), "%d", &clock_tech.chip_area);
			}
			else if(!it->first.compare("clock_node_cap")){
				sscanf(it->second.c_str(), "%lf", &clock_tech.node_cap);
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
			else if (!it->first.compare("router_has_global_link")){
			    sscanf(it->second.c_str(), "%d", &router_tech.has_global_link);
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
	     McPATSetup();	    
	     p_Mproc.initialize(p_Mp1); //unit energy is computed by McPAT in this step
	     p_ifGetMcPATUnitP = true;
	     p_Mcore = p_Mproc.SSTreturnCore(); //return core
	     ifu = p_Mcore->SSTreturnIFU();
	     lsu = p_Mcore->SSTreturnLSU();
	     mmu = p_Mcore->SSTreturnMMU();
	     exu = p_Mcore->SSTreturnEXU();
	     rnu = p_Mcore->SSTreturnRNU();
	  }
	  getUnitPower(power_type, 0, McPAT); //read
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
	/*MySimpleModel*/
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
			p_areaMcPAT = p_areaMcPAT + icache.area.get_area();
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
		    /*MySimpleModel*/ 
                      p_unitPower.il1_read = 0.5*cache_il1_tech.unit_icap*cache_il1_tech.vss*cache_il1_tech.vss*cache_il1_tech.op_freq;
		      p_unitPower.il1_write = p_unitPower.il1_read;
		    //p_usage.switchingPower = 0.5*cache_il1_tech.unit_scap*cache_il1_tech.vss*cache_il1_tech.vss*cache_il1_tech.op_freq;
                    //p_usage.leakagePower = 0.5*cache_il1_tech.unit_lcap*cache_il1_tech.vss*cache_il1_tech.vss*cache_il1_tech.op_freq;		    
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
		    /*MySimpleModel*/
                    p_unitPower.il2_read = 0.5*cache_il2_tech.unit_icap*cache_il2_tech.vss*cache_il2_tech.vss*cache_il2_tech.op_freq;
    		    p_unitPower.il2_write = p_unitPower.il2_read;
		   		    
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
			p_areaMcPAT = p_areaMcPAT + dcache.area.get_area();
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
		    /*MySimpleModel*/
                      p_unitPower.dl1_read = 0.5*cache_dl1_tech.unit_icap*cache_dl1_tech.vss*cache_dl1_tech.vss*cache_dl1_tech.op_freq;
		      p_unitPower.dl1_write = p_unitPower.dl1_read;
		    		    
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
		    /*MySimpleModel*/
                    p_unitPower.dl2_read = 0.5*cache_dl2_tech.unit_icap*cache_dl2_tech.vss*cache_dl2_tech.vss*cache_dl2_tech.op_freq;
		    p_unitPower.dl2_write = p_unitPower.dl2_read;
		    		    
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
		    /*MySimpleModel*/
                      p_unitPower.itlb_read = 0.5*cache_itlb_tech.unit_icap*cache_itlb_tech.vss*cache_itlb_tech.vss*cache_itlb_tech.op_freq;
		      p_unitPower.itlb_write = p_unitPower.itlb_read;		    		    
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
		    /*MySimpleModel*/
                      p_unitPower.dtlb_read = 0.5*cache_dtlb_tech.unit_icap*cache_dtlb_tech.vss*cache_dtlb_tech.vss*cache_dtlb_tech.op_freq;
		      p_unitPower.dtlb_write = p_unitPower.dtlb_read;
		   		    
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
		    /*MySimpleModel*/
                      p_unitPower.clock = 0.5*clock_tech.unit_icap*clock_tech.vss*clock_tech.vss*clock_tech.op_freq;
		   
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
			p_areaMcPAT = p_areaMcPAT + BPT->area.get_area();
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
		    /*MySimpleModel*/
                      p_unitPower.bpred = 0.5*bpred_tech.unit_icap*bpred_tech.vss*bpred_tech.vss*bpred_tech.op_freq;
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
			p_areaMcPAT = p_areaMcPAT + rfu->area.get_area();
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
		    /*MySimpleModel*/
                      p_unitPower.rf = 0.5*rf_tech.unit_icap*rf_tech.vss*rf_tech.vss*rf_tech.op_freq;
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
		    /*MySimpleModel*/
                    p_unitPower.aio = 0.5*io_tech.unit_icap*io_tech.vss*io_tech.vss*io_tech.op_freq;
		    p_unitPower.dio = p_unitPower.aio;
		    	    
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
		    /*MySimpleModel*/
                    p_unitPower.logic = 0.5*logic_tech.unit_icap*logic_tech.vss*logic_tech.vss*logic_tech.op_freq;
		    
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
			p_areaMcPAT = p_areaMcPAT + exeu->area.get_area();
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
		    /*MySimpleModel*/
                      p_unitPower.alu = 0.5*alu_tech.unit_icap*alu_tech.vss*alu_tech.vss*alu_tech.op_freq;
		       
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
		    /*MySimpleModel*/
                      p_unitPower.fpu = 0.5*fpu_tech.unit_icap*fpu_tech.vss*fpu_tech.vss*fpu_tech.op_freq;

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
		    /*MySimpleModel*/
                      p_unitPower.mult = 0.5*mult_tech.unit_icap*mult_tech.vss*mult_tech.vss*mult_tech.op_freq;

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
			p_areaMcPAT = p_areaMcPAT + IB->area.get_area();
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
			p_areaMcPAT = p_areaMcPAT + (ID_inst->area.get_area() +
				ID_operand->area.get_area() +
				ID_misc->area.get_area())*core_tech.core_decode_width; //*10-6mm^2
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
			p_areaMcPAT = p_areaMcPAT + LSQ->area.get_area();
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
			    p_areaMcPAT = p_areaMcPAT + BTB->area.get_area();
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
			p_areaMcPAT = p_areaMcPAT + LoadQ->area.get_area();
		    #endif  
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*MySimpleModel*/                    
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
			p_areaMcPAT = p_areaMcPAT + rnu->area.get_area();
		    #endif
		    break;
		    case 1:
		    /*SimPanalyzer*/	
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*MySimpleModel*/
                    
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
			p_areaMcPAT = p_areaMcPAT + scheu->area.get_area();
		    #endif 
		    break;
		    case 1:
		    /*SimPanalyzer*/			   
		    break;
                    case 2:
		    /*McPAT05*/
		    break;
	            case 3:
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
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
		    /*MySimpleModel*/
                    p_unitPower.uarch = 9.99;
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
		    /*MySimpleModel*/
                    p_unitPower.uarch = 9.99;
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
		    /*MySimpleModel*/
                    p_unitPower.uarch = 9.99;
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
Pdissipation_t& Power::getPower(Component* c, ptype power_type, usagecounts_t counts/*char *user_parms*/)
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
		dynamicPower = (I)icache.rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	   
	    updatePowUsage(c, power_type, &p_usage_cache_il1, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_cache_il2, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	   
	    updatePowUsage(c, power_type, &p_usage_cache_dl1, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_cache_dl2, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_cache_itlb, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_cache_dtlb, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_clock, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 7:
	  //bpred
	    switch(p_powerModel.bpred)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		if (bpred_tech.prediction_width > 0){
		  //executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		  BPT->SSTcomputeEnergy(false, counts.branch_read, counts.branch_write, counts.RAS_read, counts.RAS_write);		
		  leakage = (I)BPT->power.readOp.leakage + (I)BPT->power.readOp.gate_leakage;
		  dynamicPower = (I)BPT->rt_power.readOp.dynamic / executionTime;
		  totalPowerUsage = leakage + dynamicPower;
		  BPT->SSTcomputeEnergy(true, counts.branch_read, counts.branch_write, counts.RAS_read, counts.RAS_write);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_bpred, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 8:
	  //rf
	    switch(p_powerModel.rf)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		rfu->SSTcomputeEnergy(false, counts.int_regfile_reads, counts.int_regfile_writes, counts.float_regfile_reads, counts.float_regfile_writes, counts.RFWIN_read, counts.RFWIN_write);		
		leakage = (I)rfu->power.readOp.leakage + (I)rfu->power.readOp.gate_leakage;
		dynamicPower = (I)rfu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		rfu->SSTcomputeEnergy(true, counts.int_regfile_reads, counts.int_regfile_writes, counts.float_regfile_reads, counts.float_regfile_writes, counts.RFWIN_read, counts.RFWIN_write);
		TDP = (I)rfu->power.readOp.dynamic * (I)device_tech.clockRate;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_rf, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_io, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_logic, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 11:
	  //alu
	    switch(p_powerModel.alu)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		leakage = (I)exeu->power.readOp.leakage + (I)exeu->power.readOp.gate_leakage;
		dynamicPower = (I)exeu->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)exeu->power.readOp.dynamic * (I)device_tech.clockRate;
		/*using namespace io_interval; std::cout <<"ALU leakage =" << leakage <<", orig dynamic = " << exeu->rt_power.readOp.dynamic << ",dynamic= " << dynamicPower << ", total=" << totalPowerUsage << ", TDP= " << TDP << "executionTime= " << executionTime << std::endl;*/
		#endif      
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.alu_access * (I)p_unitPower.alu;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_alu, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 12:
	  //fpu
	    switch(p_powerModel.fpu)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		leakage = (I)fp_u->power.readOp.leakage + (I)fp_u->power.readOp.gate_leakage;
		dynamicPower = (I)fp_u->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		TDP = (I)fp_u->power.readOp.dynamic * (I)device_tech.clockRate;
		#endif    
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.fpu_access * (I)p_unitPower.fpu;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    
	    updatePowUsage(c, power_type, &p_usage_fpu, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 13:
	  //mult
	    switch(p_powerModel.mult)
	    {
	      case 0:
	      /* TODO Wattch*/
                  
	      break;
	      case 1:
	      /*SimPanalyzer*/
		#ifdef PANALYZER_H
		if (p_powerLevel == 1)
	            totalPowerUsage = (I)counts.mult_access * (I)p_unitPower.mult;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_mult, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 14:  
	  //ib	
	    switch(p_powerModel.ib)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[0], counts.IB_write[0], counts.BTB_read[0], counts.BTB_write[0]);
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
	      /*MySimpleModel*/
              break;
	    }	
	    
	    updatePowUsage(c, power_type, &p_usage_ib, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_rs, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;					
	  case 16:  
	  //decoder	
	    switch(p_powerModel.decoder)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[0], counts.IB_write[0], counts.BTB_read[0], counts.BTB_write[0], counts.ID_inst_read[0], counts.ID_operand_read[0], counts.ID_misc_read[0]);
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
	      /*MySimpleModel*/
              break;
	    }	
	    
	    updatePowUsage(c, power_type, &p_usage_decoder, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_bypass, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	    
	    updatePowUsage(c, power_type, &p_usage_exeu, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	
	    updatePowUsage(c, power_type, &p_usage_pipeline, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  case 20:  
	  //lsq	
	    switch(p_powerModel.lsq)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		lsu->SSTcomputeEnergy(false, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[0], counts.LSQ_write[0], counts.loadQ_read[0], counts.loadQ_write[0]);
		
		leakage = (I)LSQ->power.readOp.leakage + (I)LSQ->power.readOp.gate_leakage;
		dynamicPower = (I)LSQ->rt_power.readOp.dynamic / executionTime;
		totalPowerUsage = leakage + dynamicPower;
		lsu->SSTcomputeEnergy(true, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[0], counts.LSQ_write[0], counts.loadQ_read[0], counts.loadQ_write[0]);
		
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_lsq, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_rat, totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	    
	    updatePowUsage(c, power_type, &p_usage_rob, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;	
	  case 23:  
	  //btb	
	    switch(p_powerModel.btb)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		if (bpred_tech.prediction_width > 0){
		  //executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		  ifu->SSTcomputeEnergy(false, counts.il1_read[0], counts.il1_readmiss[0], counts.IB_read[0], counts.IB_write[0], counts.BTB_read[0], counts.BTB_write[0]);		
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_btb, totalPowerUsage, dynamicPower, leakage, TDP);
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
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i]);
		leakage = (I)l2array->power.readOp.leakage + (I)l2array->power.readOp.gate_leakage;
		dynamicPower = (I)l2array->rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
              break;
	    }	
	    updatePowUsage(c, power_type, &p_usage_cache_l2[i], totalPowerUsage, dynamicPower, leakage, TDP);
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
	      /*MySimpleModel*/
              break;
	    }	
	    
	    updatePowUsage(c, power_type, &p_usage_mc, totalPowerUsage, dynamicPower, leakage, TDP);
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
		leakage = (I)nocs->power.readOp.leakage + (I)nocs->power.readOp.gate_leakage;
		dynamicPower = (I)nocs->rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
              break;
	    }	
	   
	    updatePowUsage(c, power_type, &p_usage_router, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 27:
	  //load_Q
	    switch(p_powerModel.loadQ)
	    {
	      case 0:
	      /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		lsu->SSTcomputeEnergy(false, counts.dl1_read[0], counts.dl1_readmiss[0], counts.dl1_write[0], counts.dl1_writemiss[0], counts.LSQ_read[0], counts.LSQ_write[0], counts.loadQ_read[0], counts.loadQ_write[0]);		
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_loadQ, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 28:
	  //rename_U
	    switch(p_powerModel.rename)
	    {
	      case 0:
	       /* McPAT*/
	        #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		rnu->SSTcomputeEnergy(false, counts.iFRAT_read, counts.iFRAT_write, counts.iFRAT_search, counts.fFRAT_read, counts.fFRAT_write, counts.fFRAT_search, counts.iRRAT_read,
			counts.iRRAT_write, counts.fRRAT_read, counts.fRRAT_write,	counts.ifreeL_read, counts.ifreeL_write, counts.ffreeL_read, counts.ffreeL_write, counts.idcl_read, counts.fdcl_read);
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_renameU, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	  case 29:
	  //scheduler_U
	    switch(p_powerModel.scheduler)
	    {
	      case 0:
	      /*McPAT*/
	      #ifdef McPAT07_H
		//executionTime = 1.0 / (I)device_tech.clockRate * (I)total_cycles;
		scheu->SSTcomputeEnergy(false, counts.int_win_read, counts.int_win_write, counts.int_win_search, counts.fp_win_read, counts.fp_win_write, counts.fp_win_search, counts.ROB_read, counts.ROB_write);		
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_schedulerU, totalPowerUsage, dynamicPower, leakage, TDP);
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
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i]);
		leakage = (I)l3array->power.readOp.leakage + (I)l3array->power.readOp.gate_leakage;
		dynamicPower = (I)l3array->rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_cache_l3[i], totalPowerUsage, dynamicPower, leakage, TDP);
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
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i]);
		leakage = (I)l1dirarray->power.readOp.leakage + (I)l1dirarray->power.readOp.gate_leakage;
		dynamicPower = (I)l1dirarray->rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_cache_l1dir[i], totalPowerUsage, dynamicPower, leakage, TDP);
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
			counts.L2Dir_read[i], counts.L2Dir_readmiss[i], counts.L2Dir_write[i], counts.L2Dir_writemiss[i]);
		leakage = (I)l2dirarray->power.readOp.leakage + (I)l2dirarray->power.readOp.gate_leakage;
		dynamicPower = (I)l2dirarray->rt_power.readOp.dynamic / executionTime;
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
	      /*MySimpleModel*/
                totalPowerUsage = 9.99;
              break;
	    } // end switch power model
	    updatePowUsage(c, power_type, &p_usage_cache_l2dir[i], totalPowerUsage, dynamicPower, leakage, TDP);
	    }
	  break;										
	  case 33:
	  //uarch
	        totalPowerUsage = 1*p_unitPower.uarch; //usage_count*p_unitPower.uarch;
                updatePowUsage(c, power_type, &p_usage_uarch, totalPowerUsage, dynamicPower, leakage, TDP);
	  break;
	} // end switch power_type
	return p_usage_uarch;
    } // end else

}

/***************************************************************
* Update component's currentPower, totalEnergy, and peak power *
****************************************************************/
void Power::updatePowUsage(Component *c, ptype power_type, Pdissipation_t *comp_pusage, const I& totalPowerUsage, const I& dynamicPower, const I& leakage, const I& TDP)
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
		p_usage_uarch.itemizedRuntimeDynamicPower.il1 = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.il1 = leakage;
		p_usage_uarch.itemizedCurrentPower.il1 = totalPowerUsage;
		p_usage_uarch.itemizedTDP.il1 = TDP;
		p_usage_uarch.itemizedTotalPower.il1 = comp_pusage->totalEnergy;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.dl1 = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.dl1 = leakage;
		p_usage_uarch.itemizedCurrentPower.dl1 = totalPowerUsage;
		p_usage_uarch.itemizedTDP.dl1 = TDP;
		p_usage_uarch.itemizedTotalPower.dl1 = comp_pusage->totalEnergy;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.bpred = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.bpred = leakage;
		p_usage_uarch.itemizedCurrentPower.bpred = totalPowerUsage;
		p_usage_uarch.itemizedTDP.bpred = TDP;
		p_usage_uarch.itemizedTotalPower.bpred = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.bpred = comp_pusage->peak;
	  break;
	  case RF:
		p_usage_uarch.itemizedRuntimeDynamicPower.rf = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.rf = leakage;
		p_usage_uarch.itemizedCurrentPower.rf = totalPowerUsage;
		p_usage_uarch.itemizedTDP.rf = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.alu = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.alu = leakage;
		p_usage_uarch.itemizedCurrentPower.alu = totalPowerUsage;
		p_usage_uarch.itemizedTDP.alu = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.ib = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.ib = leakage;
		p_usage_uarch.itemizedCurrentPower.ib = totalPowerUsage;
		p_usage_uarch.itemizedTDP.ib = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.decoder = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.decoder = leakage;
		p_usage_uarch.itemizedCurrentPower.decoder = totalPowerUsage;
		p_usage_uarch.itemizedTDP.decoder = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.lsq = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.lsq = leakage;
		p_usage_uarch.itemizedCurrentPower.lsq = totalPowerUsage;
		p_usage_uarch.itemizedTDP.lsq = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.btb = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.btb = leakage;
		p_usage_uarch.itemizedCurrentPower.btb = totalPowerUsage;
		p_usage_uarch.itemizedTDP.btb = TDP;
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
		p_usage_uarch.itemizedRuntimeDynamicPower.loadQ = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.loadQ = leakage;
		p_usage_uarch.itemizedCurrentPower.loadQ = totalPowerUsage;
		p_usage_uarch.itemizedTDP.loadQ = TDP;
		p_usage_uarch.itemizedTotalPower.loadQ = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.loadQ = comp_pusage->peak;
	  break;
	  case RENAME_U:
		p_usage_uarch.itemizedRuntimeDynamicPower.renameU = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.renameU = leakage;
		p_usage_uarch.itemizedCurrentPower.renameU = totalPowerUsage;
		p_usage_uarch.itemizedTDP.renameU = TDP;
		p_usage_uarch.itemizedTotalPower.renameU = comp_pusage->totalEnergy;
		p_usage_uarch.itemizedPeak.renameU = comp_pusage->peak;
	  break;
	  case SCHEDULER_U:
		p_usage_uarch.itemizedRuntimeDynamicPower.schedulerU = dynamicPower;
		p_usage_uarch.itemizedLeakagePower.schedulerU = leakage;
		p_usage_uarch.itemizedCurrentPower.schedulerU = totalPowerUsage;
		p_usage_uarch.itemizedTDP.schedulerU = TDP;
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

	// update component overall (ALL) power
	p_usage_uarch.totalEnergy = p_usage_uarch.totalEnergy + totalPowerUsage;
	p_usage_uarch.currentPower = p_usage_cache_il1.currentPower + 
					p_usage_cache_il2.currentPower +
					p_usage_cache_dl1.currentPower +
					p_usage_cache_dl2.currentPower +
					p_usage_cache_itlb.currentPower +
					p_usage_cache_dtlb.currentPower +
					p_usage_clock.currentPower +
					p_usage_io.currentPower +
					p_usage_logic.currentPower +
					p_usage_alu.currentPower +
					p_usage_fpu.currentPower +
					p_usage_mult.currentPower +	
					p_usage_rf.currentPower +
					p_usage_bpred.currentPower +
					p_usage_ib.currentPower +
					p_usage_rs.currentPower +
					p_usage_decoder.currentPower +
					p_usage_bypass.currentPower +
					p_usage_exeu.currentPower +
					p_usage_pipeline.currentPower +
					p_usage_lsq.currentPower +
					p_usage_rat.currentPower +
					p_usage_rob.currentPower +
					p_usage_btb.currentPower +
					p_usage_uarch.itemizedCurrentPower.L2 +
					p_usage_mc.currentPower +
					p_usage_renameU.currentPower +
					p_usage_schedulerU.currentPower +
					p_usage_loadQ.currentPower +
					p_usage_uarch.itemizedCurrentPower.L3 +
					p_usage_uarch.itemizedCurrentPower.L1dir +
					p_usage_uarch.itemizedCurrentPower.L2dir +
					p_usage_router.currentPower;
	p_usage_uarch.leakagePower = p_usage_cache_il1.leakagePower + 
					p_usage_cache_il2.leakagePower +
					p_usage_cache_dl1.leakagePower +
					p_usage_cache_dl2.leakagePower +
					p_usage_cache_itlb.leakagePower +
					p_usage_cache_dtlb.leakagePower +
					p_usage_clock.leakagePower +
					p_usage_io.leakagePower +
					p_usage_logic.leakagePower +
					p_usage_alu.leakagePower +
					p_usage_fpu.leakagePower +
					p_usage_mult.leakagePower +	
					p_usage_rf.leakagePower +
					p_usage_bpred.leakagePower +
					p_usage_ib.leakagePower +
					p_usage_rs.leakagePower +
					p_usage_decoder.leakagePower +
					p_usage_bypass.leakagePower +
					p_usage_exeu.leakagePower +
					p_usage_pipeline.leakagePower +
					p_usage_lsq.leakagePower +
					p_usage_rat.leakagePower +
					p_usage_rob.leakagePower +
					p_usage_btb.leakagePower +
					p_usage_uarch.itemizedLeakagePower.L2 +
					p_usage_mc.leakagePower +
					p_usage_renameU.leakagePower +
					p_usage_schedulerU.leakagePower +
					p_usage_loadQ.leakagePower +
					p_usage_uarch.itemizedLeakagePower.L3 +
					p_usage_uarch.itemizedLeakagePower.L1dir +
					p_usage_uarch.itemizedLeakagePower.L2dir +
					p_usage_router.leakagePower;
/*using namespace io_interval; std:: cout << "SST total leakage " << p_usage_cache_il1.leakagePower << " "
	<< p_usage_cache_dl1.leakagePower << " " << p_usage_cache_itlb.leakagePower << " " 
	<< p_usage_cache_dtlb.leakagePower << " " << p_usage_alu.leakagePower << " "
	<< p_usage_fpu.leakagePower << " " << p_usage_rf.leakagePower << " "
	<< p_usage_bpred.leakagePower << " " << p_usage_ib.leakagePower << " "
	<< p_usage_bypass.leakagePower << " " << p_usage_lsq.leakagePower << " "
	<< p_usage_schedulerU.leakagePower << std::endl; */
	p_usage_uarch.runtimeDynamicPower = p_usage_cache_il1.runtimeDynamicPower + 
					p_usage_cache_il2.runtimeDynamicPower +
					p_usage_cache_dl1.runtimeDynamicPower +
					p_usage_cache_dl2.runtimeDynamicPower +
					p_usage_cache_itlb.runtimeDynamicPower +
					p_usage_cache_dtlb.runtimeDynamicPower +
					p_usage_clock.runtimeDynamicPower +
					p_usage_io.runtimeDynamicPower +
					p_usage_logic.runtimeDynamicPower +
					p_usage_alu.runtimeDynamicPower +
					p_usage_fpu.runtimeDynamicPower +
					p_usage_mult.runtimeDynamicPower +	
					p_usage_rf.runtimeDynamicPower +
					p_usage_bpred.runtimeDynamicPower +
					p_usage_ib.runtimeDynamicPower +
					p_usage_rs.runtimeDynamicPower +
					p_usage_decoder.runtimeDynamicPower +
					p_usage_bypass.runtimeDynamicPower +
					p_usage_exeu.runtimeDynamicPower +
					p_usage_pipeline.runtimeDynamicPower +
					p_usage_lsq.runtimeDynamicPower +
					p_usage_rat.runtimeDynamicPower +
					p_usage_rob.runtimeDynamicPower +
					p_usage_btb.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.L2 +
					p_usage_mc.runtimeDynamicPower +
					p_usage_renameU.runtimeDynamicPower +
					p_usage_schedulerU.runtimeDynamicPower +
					p_usage_loadQ.runtimeDynamicPower +
					p_usage_uarch.itemizedRuntimeDynamicPower.L3 +
					p_usage_uarch.itemizedRuntimeDynamicPower.L1dir +
					p_usage_uarch.itemizedRuntimeDynamicPower.L2dir +
					p_usage_router.runtimeDynamicPower;
	p_usage_uarch.TDP = p_usage_cache_il1.TDP + 
					p_usage_cache_il2.TDP +
					p_usage_cache_dl1.TDP +
					p_usage_cache_dl2.TDP +
					p_usage_cache_itlb.TDP +
					p_usage_cache_dtlb.TDP +
					p_usage_clock.TDP +
					p_usage_io.TDP +
					p_usage_logic.TDP +
					p_usage_alu.TDP +
					p_usage_fpu.TDP +
					p_usage_mult.TDP +	
					p_usage_rf.TDP +
					p_usage_bpred.TDP +
					p_usage_ib.TDP +
					p_usage_rs.TDP +
					p_usage_decoder.TDP +
					p_usage_bypass.TDP +
					p_usage_exeu.TDP +
					p_usage_pipeline.TDP +
					p_usage_lsq.TDP +
					p_usage_rat.TDP +
					p_usage_rob.TDP +
					p_usage_btb.TDP +
					p_usage_uarch.itemizedTDP.L2 +
					p_usage_mc.TDP +
					p_usage_renameU.TDP +
					p_usage_schedulerU.TDP +
					p_usage_loadQ.TDP +
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
* Return time in second                                   *
****************************************************************/
I Power::getExecutionTime(Component *c)
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

   
    while (it != deviceParams.end()){
	if (!it->first.compare("machine_type")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.machineType);
	}
	else if (!it->first.compare("number_of_IL1s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_il1);
	}
	else if (!it->first.compare("number_of_DL1s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_dl1);
	}
	else if (!it->first.compare("number_of_ITLBs")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_itlb);
	}
	else if (!it->first.compare("number_of_DTLBs")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_dtlb);
	}
	else if (!it->first.compare("number_of_L2s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L2);
	}
	else if (!it->first.compare("number_of_L3s")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L3);
	}
	else if (!it->first.compare("number_of_L1Directories")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L1dir);
	}
	else if (!it->first.compare("number_of_L2Directories")){ //Mc
	        sscanf(it->second.c_str(), "%d", &device_tech.number_L2dir);
	}
	else if (!it->first.compare("core_clock_rate")){  //Mc
	        sscanf(it->second.c_str(), "%f", &device_tech.clockRate);
	}
        it++;
    }
    
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
    p_usage_cache_l2.resize(device_tech.number_L2);
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
    p_usage_cache_l3.resize(device_tech.number_L3);
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
    p_usage_cache_l1dir.resize(device_tech.number_L1dir);
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
    p_usage_cache_l2dir.resize(device_tech.number_L2dir);
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
	p_Mp1->sys.target_core_clockrate=2200;
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
		p_Mp1->sys.core[0].x86 = false;
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
		p_Mp1->sys.core[0].decoded_stream_buffer_size=20;		
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
		p_Mp1->sys.L3[i].duty_cycle =1;

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
	p_Mp1->sys.NoC[0].horizontal_nodes=router_tech.horizontal_nodes;
	p_Mp1->sys.NoC[0].vertical_nodes=router_tech.vertical_nodes;
	p_Mp1->sys.NoC[0].input_ports=router_tech.input_ports;
	p_Mp1->sys.NoC[0].output_ports=router_tech.output_ports;
	p_Mp1->sys.NoC[0].virtual_channel_per_port=router_tech.virtual_channel_per_port;
	p_Mp1->sys.NoC[0].flit_bits=router_tech.flit_bits;
	p_Mp1->sys.NoC[0].input_buffer_entries_per_vc=router_tech.input_buffer_entries_per_vc;
	p_Mp1->sys.NoC[0].total_accesses=1;
	p_Mp1->sys.NoC[0].duty_cycle=1;
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
	p_Mp1->sys.number_cache_levels=3;
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
		p_Mp1->sys.core[0].decoded_stream_buffer_size=20;		
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

/* The following is no longer used*/
/***************************************************************
* Initialize basic tech params to prevent errors	       *
* They will be overwritten when individual components are init *
* McPAT05 interface				               * 
****************************************************************/
void Power::McPAT05initBasic()
{
  
  /**Basic parameters*/
  interface_ip.data_arr_ram_cell_tech_type    = 0; //device_type (HPC)
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;

  interface_ip.ic_proj_type     = 0; //interconnect_projection_type (aggresive)
  interface_ip.wire_is_mat_type = 2;
  interface_ip.wire_os_mat_type = 2;
  interface_ip.max_area_t_constraint_perc = 90;
  interface_ip.max_acc_t_constraint_perc = 50;
  interface_ip.max_perc_diff_in_delay_fr_best_delay_rptr_sol = 40;
  interface_ip.burst_len      = 1;
  interface_ip.int_prefetch_w = 1;
  interface_ip.page_sz_bits   = 0;
  interface_ip.temp = 360;
  interface_ip.F_sz_nm = 65; //core_tech_node
  interface_ip.F_sz_um = interface_ip.F_sz_nm / 1000;

  //***********This section of code does not have real meaning, they are just to ensure all data will have initial value to prevent errors.
  //They will be overridden  during each components initialization
  interface_ip.cache_sz            =64;
  interface_ip.line_sz             = 1;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = 64;
  interface_ip.access_mode         = 2;

  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;

  interface_ip.is_main_mem     = false;
  interface_ip.rpters_in_htree = true ;
  interface_ip.ver_htree_wires_over_array = 0;
  interface_ip.broadcast_addr_din_over_ver_htrees = 0;

  interface_ip.num_rw_ports        = 1;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
}

/*************************************************
* Re-initialize Icache tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitIcache()
{
  
  int size, line, assoc, banks, idx, tag, data;
  

  /*-----cache-----*/
  size                             = (int)cache_il1_tech.unit_scap; //capacity
  line                             = (int)cache_il1_tech.line_size; //block width
  assoc                            = (int)cache_il1_tech.assoc; //assoc
  banks                            = (int)cache_il1_tech.num_banks; //banks
  idx    			   = int(ceil(log2(size/line/assoc)));
  tag				   = cache_il1_tech.core_physical_address_width-idx-int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = (unsigned int)tag;
  interface_ip.cache_sz            = (unsigned int)size;
  interface_ip.line_sz             = (unsigned int)line;
  interface_ip.assoc               = (unsigned int)assoc;
  interface_ip.nbanks              = (unsigned int)banks;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = cache_il1_tech.throughput/device_tech.clockRate; // cycle time
  interface_ip.latency             = cache_il1_tech.latency/device_tech.clockRate;  //access time
  interface_ip.is_cache		   = true;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = cache_il1_tech.num_rwports; //number_instruction_fetch_ports
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  icache.caches.init_cache(&interface_ip); //re-set params values from SST xml
  
  /*-----miss buffer-----*/
  tag				   = (int)cache_il1_tech.core_physical_address_width + EXTRA_TAG_BITS;
  data				   = (int)cache_il1_tech.core_physical_address_width + int(ceil(log2(size/line))) + icache.caches.l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data/8.0));
  interface_ip.cache_sz            = cache_il1_tech.miss_buf_size*interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  //...the rest are the same as the ones in cache
  icache.missb.init_cache(&interface_ip);  //re-set params values from SST xml
	
  /*-----fill buffer-----*/  
  data				   = icache.caches.l_ip.line_sz;  
  interface_ip.line_sz             = data;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.cache_sz            = cache_il1_tech.fill_buf_size*data;
  //...the rest are the same as the ones in miss_buf
  icache.ifb.init_cache(&interface_ip);  //re-set params values from SST xml

  /*-----prefetch-----*/ 
  interface_ip.cache_sz            = cache_il1_tech.prefetch_buf_size*interface_ip.line_sz;
  //...the rest are the same as the ones in fill_buf
  icache.prefetchb.init_cache(&interface_ip);

  //icache does not have wbb
}

/*************************************************
* Re-initialize Dcache tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitDcache()
{
  int size, line, assoc, banks, idx, tag, data;
 
  /*-----cache-----*/
  size                             = (int)cache_dl1_tech.unit_scap; //capacity
  line                             = (int)cache_dl1_tech.line_size; //block width
  assoc                            = (int)cache_dl1_tech.assoc; //assoc
  banks                            = (int)cache_dl1_tech.num_banks; //banks
  idx    			   = int(ceil(log2(size/line/assoc)));
  tag				   = (int)cache_dl1_tech.core_physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = (unsigned int)tag;
  interface_ip.cache_sz            = (unsigned int)size;
  interface_ip.line_sz             = (unsigned int)line;
  interface_ip.assoc               = (unsigned int)assoc;
  interface_ip.nbanks              = (unsigned int)banks;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = cache_dl1_tech.throughput/device_tech.clockRate; // cycle time
  interface_ip.latency             = cache_dl1_tech.latency/device_tech.clockRate;  //access time
  interface_ip.is_cache		   = true;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = cache_dl1_tech.num_rwports; //memory_ports; usually In-order has 1 and OOO has 2 at least.
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;

  strcpy(dcache.caches.name,"dcache");
  dcache.caches.init_cache(&interface_ip);

  /*-----miss buf-----*/
  tag				   = (int)cache_dl1_tech.core_physical_address_width + EXTRA_TAG_BITS;
  data				   = (int)cache_dl1_tech.core_physical_address_width + int(ceil(log2(size/line))) + dcache.caches.l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data/8.0));//int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz            = cache_dl1_tech.miss_buf_size*interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  //...the rest are unchanged
  strcpy(dcache.missb.name,"dcacheMissB");
  dcache.missb.init_cache(&interface_ip);

  /*-----fill buf-----*/
  data				   = dcache.caches.l_ip.line_sz;  
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = cache_dl1_tech.fill_buf_size*data; 
  interface_ip.out_w               = interface_ip.line_sz*8;
  //...the rest are unchanged
  strcpy(dcache.ifb.name,"dcacheFillB");
  dcache.ifb.init_cache(&interface_ip);

  /*-----prefetch buf-----*/  
  interface_ip.cache_sz            = cache_dl1_tech.prefetch_buf_size*interface_ip.line_sz;
  //...the rest are unchanged
  strcpy(dcache.prefetchb.name,"dcacheprefetchB");
  dcache.prefetchb.init_cache(&interface_ip); 

  /*-----write back buf-----*/  
  interface_ip.cache_sz            = cache_dl1_tech.wbb_buf_size*interface_ip.line_sz;
  //...the rest are unchanged
  strcpy(dcache.wbb.name,"WBB");
  dcache.wbb.init_cache(&interface_ip);
}

/*************************************************
* Re-initialize ITLB tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitItlb()
{
  int tag, data;
 

  tag				   = cache_itlb_tech.core_virtual_address_width- int(floor(log2(cache_itlb_tech.core_virtual_memory_page_size))) 
				   + int(ceil(log2(cache_itlb_tech.core_number_hardware_threads)))+ EXTRA_TAG_BITS;
  data				   = cache_itlb_tech.core_physical_address_width- int(floor(log2(cache_itlb_tech.core_virtual_memory_page_size)));
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data/8.0));
  interface_ip.cache_sz            = cache_itlb_tech.number_entries*interface_ip.line_sz*cache_itlb_tech.core_number_hardware_threads;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = cache_il1_tech.throughput/device_tech.clockRate; // cycle time
  interface_ip.latency             = cache_il1_tech.latency/device_tech.clockRate;  //access time
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = cache_itlb_tech.num_rwports; //number_instruction_fetch_ports
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  interface_ip.is_cache        = true;
  strcpy(itlb.tlb.name,"ITLB");
  itlb.tlb.init_cache(&interface_ip);
}

/*************************************************
* Re-initialize DTLB tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitDtlb()
{
  int tag, data;
 
  tag				   = cache_dtlb_tech.core_virtual_address_width- int(floor(log2(cache_dtlb_tech.core_virtual_memory_page_size))) 
				   + int(ceil(log2(cache_dtlb_tech.core_number_hardware_threads)))+ EXTRA_TAG_BITS;
  data				   = cache_dtlb_tech.core_physical_address_width- int(floor(log2(cache_dtlb_tech.core_virtual_memory_page_size)));
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data/8.0));
  interface_ip.cache_sz            = cache_dtlb_tech.number_entries*interface_ip.line_sz*cache_dtlb_tech.core_number_hardware_threads;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = cache_dl1_tech.throughput/device_tech.clockRate; // cycle time
  interface_ip.latency             = cache_dl1_tech.latency/device_tech.clockRate;  //access time
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = cache_dtlb_tech.num_rwports; //memory_ports
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  interface_ip.is_cache        = true;
  strcpy(dtlb.tlb.name,"DTLB");
  dtlb.tlb.init_cache(&interface_ip);
}

/*************************************************
* Re-initialize IB tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitIB()
{
  int data, tag;
 

  tag				   = ib_tech.core_virtual_address_width- int(floor(log2(ib_tech.core_virtual_memory_page_size))) 
				   + int(ceil(log2(ib_tech.core_number_hardware_threads)))+ EXTRA_TAG_BITS;
  data				   = ib_tech.core_instruction_length*ib_tech.core_issue_width; //multiple threads timing sharing the instruction buffer.
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.is_cache		   = false;
  interface_ip.line_sz             = int(ceil(data/8.0));
  interface_ip.cache_sz            = ib_tech.core_number_hardware_threads*ib_tech.core_instruction_buffer_size*interface_ip.line_sz>64?
		                             ib_tech.core_number_hardware_threads*ib_tech.core_instruction_buffer_size*interface_ip.line_sz:64;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = 1.0/device_tech.clockRate;
  interface_ip.latency             = 1.0/device_tech.clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  //NOTE: Assuming IB is time slice shared among threads, every fetch op will at least fetch "fetch width" instructions.
  interface_ip.num_rw_ports    = ib_tech.num_rwports; //intr_fetch_ports
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;

  strcpy(IB.IB.name,"InstBuffer");
  IB.IB.init_cache(&interface_ip);

}

/*************************************************
* Re-initialize IRS tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitIRS()
{
  int tag, data;

  tag				   = int(log2(irs_tech.core_number_hardware_threads)*perThreadState);//This is the normal thread state bits based on Niagara Design
  data				   = irs_tech.core_instruction_length;//NOTE: x86 inst can be very lengthy, up to 15B
  interface_ip.is_cache		   = true;
  interface_ip.line_sz             = int(ceil(data/8.0));
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = irs_tech.core_instruction_window_size*interface_ip.line_sz>64?irs_tech.core_instruction_window_size*interface_ip.line_sz:64;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0/device_tech.clockRate;
  interface_ip.latency             = 1.0/device_tech.clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = 0;
  interface_ip.num_rd_ports    = irs_tech.core_issue_width;
  interface_ip.num_wr_ports    = irs_tech.core_issue_width;
  interface_ip.num_se_rd_ports = 0;
  strcpy(iRS.RS.name,"InstQueue");
  iRS.RS.init_cache(&interface_ip);
}

/*************************************************
* Re-initialize Reg file tech params from SST xml  *
* McPAT interface				 *  
*************************************************/
void Power::McPATinitRF()
{
  int data, tag;
  bool regWindowing;
 

  /*-----iRF-----*/
  tag				   = rf_tech.core_opcode_width + rf_tech.core_virtual_address_width 
					+int(ceil(log2(rf_tech.core_number_hardware_threads))) + EXTRA_TAG_BITS;
  data				   = rf_tech.machine_bits;
  interface_ip.is_cache		   = false;
  interface_ip.line_sz             = int(ceil(data/32.0))*4;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = rf_tech.archi_Regs_IRF_size*interface_ip.line_sz;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0/device_tech.clockRate;
  interface_ip.latency             = 1.0/device_tech.clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = 1;//this is the transfer port for saving/restoring states when exceptions happen.
  interface_ip.num_rd_ports    = 2*rf_tech.core_issue_width;
  interface_ip.num_wr_ports    = rf_tech.core_issue_width;
  interface_ip.num_se_rd_ports = 0;
  strcpy(IRF.RF.name,"ArchIntReg");
  IRF.RF.init_cache(&interface_ip);

  /*-----fRF-----*/
  data				   = int(ceil(ceil(rf_tech.machine_bits/32.0)*4*1.5/8.0));
  interface_ip.is_cache		   = false;
  interface_ip.line_sz             = data;//int(ceil(data/32.0))*4;
  interface_ip.cache_sz            = rf_tech.archi_Regs_FRF_size*interface_ip.line_sz;
  // the rest are unchanged
  strcpy(FRF.RF.name,"ArchFPReg");
  FRF.RF.init_cache(&interface_ip);

  /*-----RF WIN-----*/
  regWindowing= (int)rf_tech.core_register_windows_size>0?true:false;
  if (regWindowing == true)
  {
	  data				   = (rf_tech.machine_bits/8 + rf_tech.machine_bits)*2; 
	  interface_ip.is_cache		   = false;
	  interface_ip.line_sz             = int(ceil(data/8.0));
	  interface_ip.cache_sz            = rf_tech.core_register_windows_size*rf_tech.core_number_hardware_threads*interface_ip.line_sz;
	  interface_ip.assoc               = 1;
	  interface_ip.nbanks              = 1;
	  interface_ip.out_w               = interface_ip.line_sz*8;
	  interface_ip.access_mode         = 1;
	  interface_ip.throughput          = 4.0/device_tech.clockRate;
	  interface_ip.latency             = 4.0/device_tech.clockRate;
	  interface_ip.obj_func_dyn_energy = 0;
	  interface_ip.obj_func_dyn_power  = 0;
	  interface_ip.obj_func_leak_power = 0;
	  interface_ip.obj_func_cycle_t    = 1;
	  interface_ip.num_rw_ports    = 1;//this is the transfer port for saving/restoring states when exceptions happen.
	  interface_ip.num_rd_ports    = 0;
	  interface_ip.num_wr_ports    = 0;
	  interface_ip.num_se_rd_ports = 0;
	  strcpy(RFWIN.RF.name,"RegWindow");
	  RFWIN.RF.init_cache(&interface_ip);
  }

}

/***************************************************************
* Re-initialize interconnect (bypass) tech params from SST xml *
* McPAT interface				               * 
****************************************************************/
void Power::McPATinitBypass()
{
  int tag, data;
  bool is_default = true;


  //initialize LSQ here
  tag				   = bypass_tech.core_opcode_width+bypass_tech.core_virtual_address_width 
					+int(ceil(log2(bypass_tech.core_number_hardware_threads))) + EXTRA_TAG_BITS;
  data				   = bypass_tech.machine_bits;//64 is the data width
  interface_ip.is_cache		   = true;
  interface_ip.line_sz             = int(ceil(data/32.0))*4;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = bypass_tech.core_store_buffer_size*interface_ip.line_sz*bypass_tech.core_number_hardware_threads;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0/device_tech.clockRate;
  interface_ip.latency             = 1.0/device_tech.clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = 0;
  interface_ip.num_rd_ports    = bypass_tech.core_memory_ports;
  interface_ip.num_wr_ports    = bypass_tech.core_memory_ports;
  interface_ip.num_se_rd_ports = 0;
  strcpy(LSQ.LSQ.name,"LSQueue");
  LSQ.LSQ.init_cache(&interface_ip);

  //bypass interface_ip is inherented from rf
  McPATinitRF();

  //******************intra-core interconnects
  //Current McPAT only models intra-core interconnects inside integer/floating point unit.
  interface_ip.wire_is_mat_type = 1;//start from semi-global since local wires are already used
  interface_ip.wire_os_mat_type = 1;
  interface_ip.throughput       = 1.0/device_tech.clockRate;
  interface_ip.latency          = 1.0/device_tech.clockRate;

  //int-broadcast
  int_bypass.wires.init_wire_external(is_default, &interface_ip);
  
  //int_tag-broadcast
  intTagBypass.wires.init_wire_external(is_default, &interface_ip);  

  //fp-broadcast
  fp_bypass.wires.init_wire_external(is_default, &interface_ip);

  
}

/***************************************************************
* Re-initialize logic tech params from SST xml		       *
* McPAT interface				               * 
****************************************************************/
void Power::McPATinitLogic()
{

  int arch_ireg_width, arch_freg_width;
  bool is_default = true;

  arch_ireg_width =int(ceil(log2(logic_tech.archi_Regs_IRF_size)));
  arch_freg_width =int(ceil(log2(logic_tech.archi_Regs_FRF_size)));
 

  //logic interface_ip is inherented from bypass
  McPATinitBypass(); 

  //selection logic
  instruction_selection.win_entries	= logic_tech.core_instruction_window_size;
  instruction_selection.issue_width	= logic_tech.core_issue_width*logic_tech.core_number_hardware_threads;
  instruction_selection.init_selection_logic(is_default, &interface_ip);
  

  //integer dcl
  idcl.decode_width = logic_tech.core_decode_width;
  idcl.compare_bits = arch_ireg_width;
  idcl.init_dep_resource_conflict_check(is_default, &interface_ip);
 

  //fp dcl
  fdcl.decode_width = logic_tech.core_decode_width;
  fdcl.compare_bits = arch_freg_width;
  fdcl.init_dep_resource_conflict_check(is_default, &interface_ip);

  
}

/***************************************************************
* Re-initialize decoder tech params from SST xml	       *
* McPAT interface				               * 
****************************************************************/
void Power::McPATinitDecoder()
{

  bool is_default = true;

  //decoder interface_ip is inherented from bypass
  McPATinitBypass(); 

  inst_decoder.opcode_length = decoder_tech.core_opcode_width;
  inst_decoder.init_decoder(is_default, &interface_ip);
  
}


/***************************************************************
* Re-initialize pipeline tech params from SST xml	       *
* McPAT interface				               * 
****************************************************************/
void Power::McPATinitPipeline()
{

  int arch_ireg_width;
  bool is_default = true;

  //pipeline interface_ip is inherented from bypass
  McPATinitBypass(); 

  arch_ireg_width =int(ceil(log2(pipeline_tech.archi_Regs_IRF_size)));

  corepipe.num_thread	  = pipeline_tech.core_number_hardware_threads;
  corepipe.fetchWidth	  = pipeline_tech.core_fetch_width;
  corepipe.decodeWidth    = pipeline_tech.core_decode_width;
  corepipe.issueWidth     = pipeline_tech.core_issue_width;
  corepipe.commitWidth    = pipeline_tech.core_commit_width;
  corepipe.instruction_length  = pipeline_tech.core_instruction_length;
  corepipe.PC_width       = pipeline_tech.core_virtual_address_width;
  corepipe.opcode_length  = pipeline_tech.core_opcode_width;
  corepipe.pipeline_stages= pipeline_tech.core_int_pipeline_depth;
  corepipe.num_arch_reg_tag    = arch_ireg_width;
  corepipe.num_phsical_reg_tag = arch_ireg_width;
  corepipe.data_width  = int(ceil(pipeline_tech.machine_bits/32.0))*32;
  corepipe.address_width = pipeline_tech.core_virtual_address_width;
  corepipe.thread_clock_gated = true;
  corepipe.in_order    = true;
  corepipe.multithreaded = bool(corepipe.num_thread-1);
  corepipe.init_pipeline(is_default, &interface_ip);

  //undifferentiated core follows pipeline
  undifferentiatedCore.inOrder = true;
  undifferentiatedCore.opt_performance = true;
  undifferentiatedCore.embedded = false;
  undifferentiatedCore.pipeline_stage = pipeline_tech.core_int_pipeline_depth;
  undifferentiatedCore.num_hthreads = pipeline_tech.core_number_hardware_threads;
  undifferentiatedCore.issue_width = pipeline_tech.core_issue_width;
  undifferentiatedCore.initializeUndifferentiatedCore(is_default, &interface_ip);
}

/***************************************************************
* Re-initialize clock tech params from SST xml		       *
* McPAT interface				               * 
****************************************************************/
void Power::McPATinitClock()
{
  bool is_default = true;

  //clock interface_ip is inherented from bypass
  McPATinitBypass(); 
  
  interface_ip.temp = clock_tech.core_temperature;
  interface_ip.F_sz_nm = clock_tech.core_tech_node;
  interface_ip.F_sz_um = interface_ip.F_sz_nm / 1000;

  if ((int)rf_tech.core_register_windows_size>0){	
      interface_ip.throughput          = 4.0/device_tech.clockRate;  //RFWIN
      interface_ip.latency             = 4.0/device_tech.clockRate;  //RFWIN
  }else{
      interface_ip.throughput          = 1.0/device_tech.clockRate;  //FRF
      interface_ip.latency             = 1.0/device_tech.clockRate;  //FRF
  }	

  
  clockNetwork.init_wire_external(is_default, &interface_ip);
}

#endif //McPAT05_H


}  // namespace SST
