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

#ifndef _SST_INTERFACE_H
#define _SST_INTERFACE_H


#include <stdlib.h>
#include <stdio.h>			
#include <unistd.h>
#include <math.h>
#include <string>
#include <map>
#include <sst/core/introspectedComponent.h>
#include <sst/core/debug.h>
#include <sst/core/sst_types.h>

#define ENERGY_INTERFACE_DEBUG
//#define TEMPERATURE_DEBUG
#define TEMP_DEGREE_STEPS 101 // 300~400K



namespace SST {
enum interconnect_type_t 
{ 
  /*aggressive interconection modeling*/AGGRESSIVE, /*conservative interconnect modeling*/CONSERVATIVE, NUM_INTERCONNECT_TYPES
};

enum wiring_type_t
{
  Local, Semi_Global, Global, Memory_DRAM, Num_Wiring_Types
};

enum ITRS_device_type_t 
{ 
  /*HP device type*/HP, /*low standby power device*/LSTP, /*low operating power device*/LOP, /*logic process dram*/LP_DRAM, /*commodity dram*/COMM_DRAM, NUM_TECH_TYPES
};

enum clock_option_type_t
{
  GLOBAL_CLOCK, LOCAL_CLOCK, TOTAL_CLOCK
};

/* The following is adopted from W Song's power interface */
class feature_t
{
  public:
  feature_t() :
  x_position(0), y_position(0), width(0), length(0), area(0)
  {}

  double x_position, y_position;
  double width, length;
  double area;

  void reset()
  {
    x_position = 0; y_position = 0; width = 0; length = 0; area = 0;
  }
};

// technology parameters; eventually this should be
//combined with device_param_t. Values are initialized
// by set_default(), but will be read from xml. Currently,
// the device param are duplicatedly initialized by reading
// McPAT xml and by the set_default() here. But they don't interfere
// with each other.  
// Note, set_default() actually set ups very few param. 
class parameters_tech_t
{
  public:
  parameters_tech_t() : set(false) {}
  ~parameters_tech_t() {}

  void set_default(double size, int RAM_tech_type);

  bool set;

  int RAM_tech;			// Transistor type: HP, LSTP, LOP, LP_DRAM, COMM_DRAM

  double temperature;

  double clock_frequency;			// clock frequency
  double feature_size;				// transistor feature size

  // Constants
  double bulk_copper_resistivity;		// Bulk resistivity of copper
  double copper_resistivity;			// Resistivity of copper
  double copper_reflectivity;			// Reflectivity coefficient at grain boundaries for copper
  double Rents_const_k;				// Rent's constant k
  double Rents_const_p;				// Rent's constant p
  double MOSFET_alpha;				// Alpha value of the power-law MOSFET model
  double subvtslope;				// Subthreshold slope at 85 degrees Celsius

  // IntSim tech parameters
  int critical_path_depth;			// Number of gates on a critical path
  int via_design_rule;				// Design rule for vias
  int avg_fanouts;				// Average fan-out of logic gates
  int avg_latches_per_buffer;			// Latches per buffer
  double activity_factor;			// Activity factor
  double wiring_aspect_ratio;			// Aspect ratio of wiring levels
  double specularity_parameter;			// Specularity parameter
  double power_pad_distance;			// Average distance from one power pad to the next
  double power_pad_length;			// Length of a pad  
  double IR_drop_limit;				// IR drop limit in percentage, half for each global and local
  double router_efficiency;			// Router efficiency
  double repeater_efficiency;			// Repeaters efficiency
  double clock_lost_ratio;			// Percentage of clock cycle lost due to the process variation and clock skew
  double max_H_tree_span;			// Max span of H tree that needs to be driven
  double clock_factor;				// Clock factor (number of latches = clock factor x total number of gates / number of gates on a critical path)
  double clock_gating_factor;			// Percentage of local clock power saved by clock gating
  double power_signal_wire_ratio;		// Ratio of widths of power and signal wires
  double clock_signal_wire_ratio;		// Ratio of widths of clock and signal wires
  double max_clock_skew;			// Max slew allowable on clock wire

  // McPAT parameters
  double sense_amp_delay;
  double sense_amp_power;
  double Wmemcella_sram;
  double Wmemcellpmos_sram;
  double Wmemcellnmos_sram;
  double area_cell_sram;
  double asp_ratio_cell_sram;
  double Wmemcella_cam;
  double Wmemcellpmos_cam;
  double Wmemcellnmos_cam;
  double area_cell_cam;
  double asp_ratio_cell_cam;
  double Wmemcella_dram;
  double Wmemcellpmos_dram;
  double Wmemcellnmos_dram;
  double area_cell_dram;
  double asp_ratio_cell_dram;
  double logic_scaling_co_eff;
  double core_tx_density;
  double sckt_co_eff;
  double chip_layout_overhead;
  double macro_layout_overhead;
  double fringe_cap[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double miller_value[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double ild_thickness[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double aspect_ratio[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double wire_pitch[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double wire_r_per_micron[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double wire_c_per_micron[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double horiz_dielectric_constant[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];
  double vert_dielectric_constant[NUM_INTERCONNECT_TYPES][Num_Wiring_Types];

  double Vpp;
  double vdd;
  double Vdsat;
  double Vth;
  double Vth_dram;
  double L_phy;
  double L_elec;
  double W_dram;
  double t_ox;
  double c_ox;
  double mobility_eff;
  double C_g_ideal;
  double C_fringe;
  double C_junc;
  double C_junc_sidewall;
  double C_dram_cell;
  double I_on_n;
  double I_on_p;
  double I_on_dram_cell;
  double I_off_dram_cell;
  double Rn_channel_on;
  double Rp_channel_on;
  double np_ratio;
  double gmp_to_gmn_multiplier;
  double long_channel_leakage_reduction;
  double I_off_n[TEMP_DEGREE_STEPS]; // 300~380K
  double I_g_on_n[TEMP_DEGREE_STEPS]; // 300~380K

  private:
  // McPAT parameters function
  double wire_resistance(double resistivity, double wire_width, double wire_thickness, double barrier_thickness,   double dishing_thickness, double alpha_scatter)
  {
    return alpha_scatter*resistivity/((wire_thickness-barrier_thickness-dishing_thickness)* (wire_width-2.0*barrier_thickness));
  }

  double wire_capacitance(double wire_width, double wire_thickness, double wire_spacing,
    double ild_thickness, double miller_value, double horiz_dielectric_constant,
    double vert_dielectric_constant, double fringe_cap)
  {
    double vertical_cap, sidewall_cap, total_cap;
    vertical_cap = 2.0 * 8.854e-18 * vert_dielectric_constant * wire_width / ild_thickness;
    sidewall_cap = 2.0 * 8.854e-18 * miller_value * horiz_dielectric_constant * wire_thickness / wire_spacing;
    total_cap = vertical_cap + sidewall_cap + fringe_cap;
    return total_cap;
  }
};

class parameters_thermal_tile_t
{
 public:
  int layer;
  int id;
  std::string name;
  double temperature;
};

class parameters_floorplan_t
{ 
 public:
  int id;				// floorplan ID - negative IDs are reserved
  std::string name;			// floorplan name
  feature_t feature;			// floorplan location and size information
  parameters_tech_t device_tech;	// device parameters per floorplan
  std::map<int,double> thermal_correlation;	// wire density between floorplans for thermal modeling
};

class parameters_chip_t
{
  public:
  parameters_chip_t() : is_set(false), num_comps(0) {}
  ~parameters_chip_t() { floorplan.clear(); }

   void insert(parameters_floorplan_t *input) {
    floorplan.insert(std::pair<int,parameters_floorplan_t>((*input).id,*input));
    *input = parameters_floorplan_t();
  }

  void insert(parameters_thermal_tile_t *input) {
    thermal_tile.insert(std::pair<std::pair<int,int>,parameters_thermal_tile_t>(std::pair<int,int>((*input).layer,(*input).id),(*input)));
    *input = parameters_thermal_tile_t();
  }


  int thermal_library;
  bool is_set;
  int num_comps;

   // HotSpot parameters
  double thermal_threshold;		// temperature threshold for DTM (Kelvin)
  double chip_thickness;		// chip thickness in meters
  double chip_thermal_conduct;		// chip thermal conductivity
  double chip_heat;			// chip specific heat
  double heatsink_convection_cap;	// convection capacitance in J/K
  double heatsink_convection_res;	// convection resistance in K/W
  double heatsink_side;			// heatsink side in meters
  double heatsink_thickness;		// heatsink thickness in meters
  double heatsink_thermal_conduct;	// heatsink thermal conductivity
  double heatsink_heat;			// heatsink specific heat
  double spreader_side;			// spreader side in meters
  double spreader_thickness;		// spreader thickness in meters
  double spreader_thermal_conduct;	// spreader thermal conductivity
  double spreader_heat;			// spreader specific heat
  double interface_thickness;		// interface material thickness in meters
  double interface_thermal_conduct;	// interface material thermal conductivity
  double interface_heat; 		// interface material specific heat
  int secondary_model;			// secondary model type
  double secondary_convection_res;	// secondary convection resistance
  double secondary_convection_cap;	// secondary convection capacitance
  int metal_layers;			// number of metal layers
  double metal_thickness;		// one layer metal thickness
  double c4_thickness;			// c4/underfill thickness
  double c4_side;			// c4 side in meters
  int c4_pads;				// number of c4 pads
  double substrate_side;		// substrate side in meters
  double substrate_thickness;		// substrate thickness
  double solder_side;			// solder side in thickness
  double solder_thickness;		// solder thickness
  double pcb_side;			// PCB side in meters
  double pcb_thickness;			// PCB thickness

  double ambient;			// ambient temperature in kelvin
  double sampling_interval;		// interval per call to compute_temp
  double clock_frequency;		// in Hz
  int leakage_used;			// temperature leakage loop
  int leakage_mode;			// temperature leakage loop
  int package_model_used; 		// flag to indicate whether package model is used
  int block_omit_lateral;		// omit lateral resistance?
  int num_grid_rows;			// grid resolution - no. of rows
  int num_grid_cols;			// grid resolution - no. of cols
  int num_floorplans;			// number of floorplans in the chip

  std::map<int,parameters_floorplan_t> floorplan;
  std::map<std::pair<int,int>,parameters_thermal_tile_t> thermal_tile;

};

class floorplan_t
{
 public:
  floorplan_t() : 
  area_estimate(0.0), leakage_feedback(false)
  { }
  ~floorplan_t() {}

  int id;				// floorplan ID - negative IDs are reserved
  std::string name;			// floorplan name			
  feature_t feature;			// location and size information
  parameters_tech_t device_tech;	// technology parameters per floorplan
  double area_estimate;
  bool leakage_feedback;

  Pdissipation_t p_usage_floorplan;	// instant power dissipation
};

class thermal_library_t
{
 public:
  thermal_library_t() {}

  virtual void compute(std::map<int,floorplan_t> *floorplan) = 0;
};

class thermal_tile_t
{
 public:
  thermal_tile_t() {}
  ~thermal_tile_t() {}

  std::string name;
  double temperature;
};

class chip_t
{
 public:
  chip_t() {}
  ~chip_t() {}

  thermal_library_t *thermal_library;			// link to thermal library

  std::map<int,floorplan_t> floorplan;		// pseudo floorplans on silicon layer
};

/*******end*****/ 

}
#endif // SST_INTERFACE_H


