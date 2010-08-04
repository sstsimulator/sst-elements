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

#ifndef INTSIM_H
#define INTSIM_H

//#include "definitions.h"
//#include "parameters.h"
//#include "energy-interface.h"
#include "interface.h"

#include "../sst/core/techModels/libIntSim/intsim.h"
#include "../sst/core/techModels/libIntSim/chip.h"
#include "../sst/core/techModels/libIntSim/parameters.h"


namespace SST {

class IntSim_library 
{
  public:
   IntSim_library(parameters_tech_t device_tech, double area, double num_transistors);

  ~IntSim_library() {}

  //virtual double get_area(void);
  // virtual double get_length(void);
   virtual void leakage_feedback(parameters_tech_t device_tech);

  public:
  int subtype;
  int clock_option;
  intsim_chip_t *chip;
  intsim_param_t *param;
  

  // IntSim main algorithm
  void IntSim(intsim_chip_t *chip, intsim_param_t *param);
};

}
#endif
