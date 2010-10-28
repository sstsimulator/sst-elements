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

#include <boost/foreach.hpp>
#include <boost/mpi.hpp>

#include "HotSpot-interface.h"

namespace SST {

RC_model_t *HotSpot_library::model;
flp_t *HotSpot_library::floorplan;
double *HotSpot_library::temperature;
double *HotSpot_library::power;
double *HotSpot_library::powerSum;

HotSpot_library::HotSpot_library(parameters_chip_t p_chip)
{
  // conversion from over-specified chip parameters to HotSpot thermal configuration
  thermal_config.thermal_threshold = p_chip.thermal_threshold;
  thermal_config.t_chip = p_chip.chip_thickness;
  thermal_config.k_chip = p_chip.chip_thermal_conduct;
  thermal_config.p_chip = p_chip.chip_heat;
  thermal_config.c_convec = p_chip.heatsink_convection_cap;
  thermal_config.r_convec = p_chip.heatsink_convection_res;
  thermal_config.s_sink = p_chip.heatsink_side;
  thermal_config.t_sink = p_chip.heatsink_thickness;
  thermal_config.k_sink = p_chip.heatsink_thermal_conduct;
  thermal_config.p_sink = p_chip.heatsink_heat;
  thermal_config.s_spreader = p_chip.spreader_side;
  thermal_config.t_spreader = p_chip.spreader_thickness;
  thermal_config.k_spreader = p_chip.spreader_thermal_conduct;
  thermal_config.p_spreader = p_chip.spreader_heat;
  thermal_config.t_interface = p_chip.interface_thickness;
  thermal_config.k_interface = p_chip.interface_thermal_conduct;
  thermal_config.p_interface = p_chip.interface_heat;
  thermal_config.model_secondary = p_chip.secondary_model;
  thermal_config.r_convec_sec = p_chip.secondary_convection_res;
  thermal_config.c_convec_sec = p_chip.secondary_convection_cap;
  thermal_config.n_metal = p_chip.metal_layers;
  thermal_config.t_metal = p_chip.metal_thickness;
  thermal_config.t_c4 = p_chip.c4_thickness;
  thermal_config.s_c4 = p_chip.c4_side;
  thermal_config.n_c4 = p_chip.c4_pads;
  thermal_config.s_sub = p_chip.substrate_side;
  thermal_config.t_sub = p_chip.substrate_thickness;
  thermal_config.s_solder = p_chip.solder_side;
  thermal_config.t_solder = p_chip.solder_thickness;
  thermal_config.s_pcb = p_chip.pcb_side;
  thermal_config.t_pcb = p_chip.pcb_thickness;
  thermal_config.ambient = p_chip.ambient;
  thermal_config.sampling_intvl = p_chip.sampling_interval;
  thermal_config.base_proc_freq = p_chip.clock_frequency;
  thermal_config.dtm_used = true;
  thermal_config.leakage_used = p_chip.leakage_used;
  thermal_config.leakage_mode = p_chip.leakage_mode;
  thermal_config.package_model_used = p_chip.package_model_used;
  thermal_config.block_omit_lateral = p_chip.block_omit_lateral;
  thermal_config.grid_rows = p_chip.num_grid_rows;
  thermal_config.grid_cols = p_chip.num_grid_cols;
  strcpy(thermal_config.model_type,"block");
  strcpy(thermal_config.grid_map_mode,"center");

  /* The followings are part of HotSpot algorithms */
  // create floorplan  
  //flp_t *floorplan = flp_alloc_init_mem(p_chip.num_floorplans);
  floorplan = flp_alloc_init_mem(p_chip.num_floorplans);

  // update floorplan information
  std::map<int,parameters_floorplan_t>::iterator fit = p_chip.floorplan.begin();
  for(int i = 0; (i < floorplan->n_units)||(fit != p_chip.floorplan.end()); i++)
  {
    floorplan->units[i].width = (*fit).second.feature.width;
    floorplan->units[i].height = (*fit).second.feature.length;
    floorplan->units[i].leftx = (*fit).second.feature.x_position;
    floorplan->units[i].bottomy = (*fit).second.feature.y_position;
    floorplan->units[i].id = (*fit).first;
    strcpy(floorplan->units[i].name,(*fit).second.name.c_str());
    fit++;
  }

  // update thermal correlation between floorplans
  for(int i = 0; i < floorplan->n_units; i++)
    for(int j = 0; j < floorplan->n_units; j++)
    {
      floorplan->wire_density[i][j] = 0.0;
      fit = p_chip.floorplan.find(floorplan->units[i].id);
      if((*fit).second.thermal_correlation.find(floorplan->units[j].id) != (*fit).second.thermal_correlation.end())
        floorplan->wire_density[i][j] =(*(*fit).second.thermal_correlation.find(floorplan->units[j].id)).second;
    }

  model = alloc_RC_model(&thermal_config,floorplan);
  populate_R_model(model,floorplan);
  populate_C_model(model,floorplan);
  
  temperature = hotspot_vector(model);
  power = hotspot_vector(model);
  powerSum = (double *)calloc(floorplan->n_units, sizeof(double)); //Added by Genie H for parallelism
  if (!powerSum) exit(-1);


  // set initial temperature
  std::map<std::pair<int,int>,parameters_thermal_tile_t>::iterator tit;
  for(int n = 0; n < NL; n++)
  {
    for(int i = 0; i < floorplan->n_units; i++)
    {
      std::pair<int,int> id(n,floorplan->units[i].id);
      tit = p_chip.thermal_tile.find(id);
      if(tit != p_chip.thermal_tile.end())
        temperature[i+n*floorplan->n_units] = (*tit).second.temperature;
    }
    for(int i = 0; i < EXTRA; i++) // 4 spreader (side) nodes + 4 heatsink (inner side) nodes + 4 heatsink (outer side) nodes
    {
      std::pair<int,int> id(n,0-i);
      tit = p_chip.thermal_tile.find(id);
      if(tit != p_chip.thermal_tile.end())
        temperature[i+NL*floorplan->n_units] = (*tit).second.temperature;
    }
  }
}

void HotSpot_library::compute(std::map<int,floorplan_t> *floorplan)
{
  std::map<int,floorplan_t>::iterator fit;
  boost::mpi::communicator world;



  for(int i = 0; i < model->block->flp->n_units; i++)
  {
    fit = floorplan->find(model->block->flp->units[i].id);
    power[i] = (double)median((*fit).second.p_usage_floorplan.currentPower);

    //#ifdef TEMPERATURE_DEBUG
      //using namespace io_interval; std::cout << "TEMPERATURE_DEBUG: my rank = " << world.rank() << ": " << (*fit).second.p_usage_floorplan.currentSimTime << " sec; floorplan ID = " << (*fit).first << "     power[" << i << "] = " << power[i] << ",  dynamic power = " << (*fit).second.p_usage_floorplan.runtimeDynamicPower << " W     leakage power = " << (*fit).second.p_usage_floorplan.leakagePower << " W     median total power = " << median((*fit).second.p_usage_floorplan.currentPower) << " W     area = " << (*fit).second.feature.area << " m^2 (estimate = " << (*fit).second.area_estimate << " m^2)" <<  std::endl;
    //#endif
  }

  // get sum of power of each tiles among all ranks
  // use all_reduce because every chip needs to know the overall information to determine which tiles have leakage feedback
  for(int i = 0; i < model->block->flp->n_units; i++)
  {
	if (world.size() == 1){
	    powerSum[i] = power[i];
	}
	else {
	    all_reduce(world, power[i], powerSum[i], std::plus<double>());
	}
	//std::cout << "Rank: " << world.rank() << ", HotSpot_Library::The sum value of power[" << i << "] is " << powerSum[i] << std::endl;
  }

  // compute updated temperature based on the summarized power of each tile of all ranks
  compute_temp(model,powerSum,temperature,model->config->sampling_intvl);

  for(int i = 0; i < model->block->flp->n_units; i++)
  {
    fit = floorplan->find(model->block->flp->units[i].id);
    //std::cout << " my rank = " << world.rank() << ": current temp = " << (int)(*fit).second.device_tech.temperature << ", updated temp = " << (int)temperature[i] << std::endl;
    if((int)(*fit).second.device_tech.temperature != (int)temperature[i])
    {
        (*fit).second.device_tech.temperature = temperature[i];
        //reset leakage, dynamic and current power
	//TODO: how about total energy?! Check with Will
	(*fit).second.p_usage_floorplan.leakagePower = 0.0;
        (*fit).second.p_usage_floorplan.runtimeDynamicPower = 0.0;
	(*fit).second.p_usage_floorplan.currentPower = 0.0;	
        (*fit).second.leakage_feedback = true;
    }
    else
    {
      (*fit).second.leakage_feedback = false;
      (*fit).second.device_tech.temperature = temperature[i];
    }
    #ifdef TEMPERATURE_DEBUG
      //std::cout << "TEMPERATURE_DEBUG: floorplan ID = " << model->block->flp->units[i].id << "     temperature = " << (*fit).second.device_tech.temperature << " K" << std::endl;
    #endif
  }
}

} //end namespace

