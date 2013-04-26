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
//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include "sst/core/element.h"
#include "components/routermodel/routermodel.h"
#include "components/bit_bucket/bit_bucket.h"
#include "end_points/allreduce_pattern.h"
#include "end_points/fft_pattern.h"
#include "end_points/msgrate_pattern.h"
#include "end_points/ghost_pattern.h"
#include "end_points/alltoall_pattern.h"
#include "end_points/pingpong_pattern.h"
#include "support/comm_pattern.h"

using namespace SST;

// THE CREATE ROUTINES FOR THE VARIOUS COMPONENTS WITHIN THE PATTERNS LIBRRY
static Component* create_routermodel(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Routermodel( id, params );
}

static Component* create_routermodel_power(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Routermodel( id, params );
}

static SST::Component* create_bit_bucket(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Bit_bucket(id, params);
}

static SST::Component* create_allreduce_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Allreduce_pattern(id, params);
}

static SST::Component* create_fft_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new FFT_pattern(id, params);
}

static SST::Component* create_msgrate_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Msgrate_pattern(id, params);
}

static SST::Component* create_ghost_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Ghost_pattern(id, params);
}

static SST::Component* create_alltoall_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Alltoall_pattern(id, params);
}

static SST::Component* create_pingpong_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Pingpong_pattern(id, params);
}

static SST::Component* create_comm_pattern(SST::ComponentId_t id, SST::Component::Params_t& params)
{
    return new Comm_pattern(id, params);
}

// THE LIST OF COMPONENTS CONTAINED WITHIN THE PATTERNS LIBRRY
static const ElementInfoComponent patterns_components[] = {
    { "routermodel",       "router model without power",        NULL, create_routermodel       },
    { "routermodel_power", "router model with power",           NULL, create_routermodel_power },
    { "bit_bucket",        "Bit bucket",                        NULL, create_bit_bucket        },
    { "allreduce_pattern", "Allreduce pattern",                 NULL, create_allreduce_pattern },
    { "fft_pattern",       "FFT pattern",                       NULL, create_fft_pattern       },
    { "msgrate_pattern",   "Message rate pattern",              NULL, create_msgrate_pattern   },
    { "ghost_pattern",     "Ghost pattern",                     NULL, create_ghost_pattern     },
    { "alltoall_pattern",  "Alltoall pattern",                  NULL, create_alltoall_pattern  },
    { "pingpong_pattern",  "Ping-pong pattern",                 NULL, create_pingpong_pattern  },
    { "comm_pattern",      "Communication pattern base object", NULL, create_comm_pattern      },
    { NULL, NULL, NULL, NULL }
};

// THE ONE AND ONLY ELEMENTLIBRARYINFO FOR THE PATTERNS LIBRARY
extern "C" {
    ElementLibraryInfo patterns_eli = {
        "patterns",
        "Patterns Components Library",
        patterns_components,
    };
}

