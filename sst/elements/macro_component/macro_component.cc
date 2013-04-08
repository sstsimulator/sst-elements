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
#include <sst/core/serialization/element.h>

#include <sst/core/element.h>

#include <sst/elements/macro_component/macro_processor.h>
#include <sst/elements/macro_component/macro_network.h>



using namespace SST;

//BOOST_CLASS_EXPORT(macro_processor)
//BOOST_CLASS_EXPORT(macro_network)


static SST::Component*
create_macro_network(SST::ComponentId_t id,
    SST::Component::Params_t& params)
{
  return new macro_network( id, params );
}

static SST::Component*
 create_macro_processor(SST::ComponentId_t id, SST::Component::Params_t& params)
 {
   return new macro_processor(id, params);
 }



static const ElementInfoComponent components[] = {
    { "macro_network",
        "SST/macro network model",
        NULL,
        create_macro_network
    },
    { "macro_processor", "Processor which runs SST/macro applications", NULL,
        create_macro_processor },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
  ElementLibraryInfo macro_component_eli = {
      "macro_component",
      "SST/macro as micro components",
      components,
  };



}




