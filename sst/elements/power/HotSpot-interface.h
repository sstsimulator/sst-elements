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
#include "flp.h"
#include "npe.h"
#include "package.h"
#include "shape.h"
#include "temperature.h"
#include "temperature_block.h"
#include "temperature_grid.h"
#include "util.h"
#endif //hotspot_h
}


namespace SST {

class HotSpot_library : public thermal_library_t
{
 public:
  HotSpot_library(parameters_chip_t p_chip);
  virtual ~HotSpot_library() {}

  private:
  thermal_config_t thermal_config;	// thermal configuration
  static RC_model_t *model;			// RC model
  static flp_t *floorplan;

  static double *temperature, *power, *powerSum;//*steady_temperature, *overall_power;
 

  virtual void compute(std::map<int,floorplan_t> *floorplan);

  friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


};

}
#endif //HotSpot_H
