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

#ifndef HotSpot_H
#define HotSpot_H

#include "interface.h"


/* HotSpot files */
extern "C"{
#ifdef HotSpot_H
#include "../sst/core/techModels/libHotSpot/flp.h"
#include "../sst/core/techModels/libHotSpot/npe.h"
#include "../sst/core/techModels/libHotSpot/package.h"
#include "../sst/core/techModels/libHotSpot/shape.h"
#include "../sst/core/techModels/libHotSpot/temperature.h"
#include "../sst/core/techModels/libHotSpot/temperature_block.h"
#include "../sst/core/techModels/libHotSpot/temperature_grid.h"
#include "../sst/core/techModels/libHotSpot/util.h"
#endif //hotspot_h
}


namespace SST {

class HotSpot_library : public thermal_library_t
{
 public:
  HotSpot_library(parameters_chip_t p_chip);
  ~HotSpot_library() {}

  private:
  thermal_config_t thermal_config;	// thermal configuration
  RC_model_t *model;			// RC model

  double *temperature, *power;//*steady_temperature, *overall_power;

  virtual void compute(std::map<int,floorplan_t> *floorplan);
};

}
#endif //HotSpot_H
