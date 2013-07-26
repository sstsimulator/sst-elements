// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization.h"

#include "IntSim-interface.h"
#include <iostream>
#include <math.h>


namespace SST {
namespace Power {
IntSim_library::IntSim_library(parameters_tech_t device_tech, double area, double num_transistors)
{

  chip = new intsim_chip_t();
  param = new intsim_param_t();

  // IntSim parameters setup
  param->Vdd = device_tech.vdd;
  param->Vt = device_tech.Vth;
  param->tox = device_tech.t_ox;
  param->drive_p_div_n = 1.0/device_tech.np_ratio;
  param->f = device_tech.clock_frequency;
  param->F = device_tech.feature_size*1e-9;
  param->A = area;
  param->ngates = num_transistors;
  param->rho = device_tech.copper_resistivity;
  param->R_coeff = device_tech.copper_reflectivity;
  param->Ileak_spec = device_tech.I_off_n[(int)device_tech.temperature-300]*device_tech.L_elec;
  param->Idsat_spec = device_tech.I_on_n*device_tech.L_elec;
  param->ncp = (double)device_tech.critical_path_depth;
  param->er = device_tech.horiz_dielectric_constant[AGGRESSIVE][GLOBAL];
  param->k = device_tech.Rents_const_k;
  param->p = device_tech.Rents_const_p;
  param->a = device_tech.activity_factor;
  param->alpha = device_tech.MOSFET_alpha;
  param->subvtslope_spec = device_tech.subvtslope;
  param->s = (double)device_tech.via_design_rule;
  param->ar = device_tech.wiring_aspect_ratio;
  param->p_size = device_tech.specularity_parameter;
  param->pad_to_pad_distance = device_tech.power_pad_distance;
  param->pad_length = device_tech.power_pad_length;
  param->ir_drop_limit = device_tech.IR_drop_limit;
  param->router_eff = device_tech.router_efficiency;
  param->rep_eff = device_tech.repeater_efficiency;
  param->fo = (double)device_tech.avg_fanouts;
  param->margin = device_tech.clock_lost_ratio;
  param->D = device_tech.max_H_tree_span;
  param->latches_per_buffer = (double)device_tech.avg_latches_per_buffer;
  param->clock_factor = device_tech.clock_factor;
  param->clock_gating_factor = device_tech.clock_gating_factor;
  param->kp = device_tech.power_signal_wire_ratio;
  param->kc = device_tech.clock_signal_wire_ratio;
  param->beta_clock = device_tech.max_clock_skew;

  // Fixed parameters - need to be changed...
  param->ew_power_ground = 0.15;
  param->F1 = param->F;
  param->Vdd_spec = param->Vdd/2.0;
  param->Vt_spec = param->Vt;
  param->kai = 4.0/(param->fo+3.0);
  param->alpha_wire = param->fo/(param->fo+1.0);
  param->ro = param->calc_ro(param->Vdd,param->Vt,param->Vdd_spec,param->Vt_spec,param->Idsat_spec,param->alpha);
  param->co = param->calc_co(param->tox,param->F);
  param->H = param->F;
  param->W = param->F;
  param->T = param->ar*param->F;
  param->S = param->F;
  param->cg = param->calc_cg(param->W,param->T,param->H,param->S,param->er);
  param->cm = param->calc_cm(param->W,param->T,param->H,param->S,param->er);
  param->ew = param->router_eff;
  param->W_global = param->kc*1e-6;
  param->T_global = param->ar*1e-6;
  param->H_global = param->ar*1e-6;
  param->S_global = 1e-6;
  param->c_clock = 2.0*param->calc_cg(param->W_global,param->T_global,param->H_global,param->S_global,param->er)+2.0*param->calc_cm(param->W_global,param->T_global,param->H_global,param->S_global,param->er);
  param->max_tier = 20;

  param->npower_pads = ceil(sqrt(area)/35.0*600.0); // linear scale from default value, 600.0


  intsim(chip,param);
}


void IntSim_library::leakage_feedback(parameters_tech_t device_tech)
{
  // update leakage currents
  param->Ileak_spec = device_tech.I_off_n[(int)device_tech.temperature-300]*device_tech.L_elec;
  chip->leakage_feedback(param);
}

/*double IntSim_library::get_area(void)
{
  switch(energy_model)
  {
    case CLOCK: return 0.1*param->A; // 10% overhead
    default: return param->A;
  }
}

double IntSim_library::get_length(void)
{
  return sqrt(param->A);
}*/

}//end namespace
}
