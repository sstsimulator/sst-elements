// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
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
#include <sst/core/serialization.h>
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
static Component* create_routermodel(SST::ComponentId_t id, SST::Params& params)
{
    return new Routermodel( id, params );
}

static Component* create_routermodel_power(SST::ComponentId_t id, SST::Params& params)
{
    return new Routermodel( id, params );
}

static SST::Component* create_bit_bucket(SST::ComponentId_t id, SST::Params& params)
{
    return new Bit_bucket(id, params);
}

static SST::Component* create_allreduce_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Allreduce_pattern(id, params);
}

static SST::Component* create_fft_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new FFT_pattern(id, params);
}

static SST::Component* create_msgrate_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Msgrate_pattern(id, params);
}

static SST::Component* create_ghost_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Ghost_pattern(id, params);
}

static SST::Component* create_alltoall_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Alltoall_pattern(id, params);
}

static SST::Component* create_pingpong_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Pingpong_pattern(id, params);
}

static SST::Component* create_comm_pattern(SST::ComponentId_t id, SST::Params& params)
{
    return new Comm_pattern(id, params);
}

static const SST::ElementInfoParam *routermodel_params = NULL;
static const SST::ElementInfoParam *routermodel_power_params = NULL;
static const SST::ElementInfoParam *bit_bucket_params = NULL;
static const SST::ElementInfoParam *allreduce_params = NULL;
static const SST::ElementInfoParam *fft_params = NULL;
static const SST::ElementInfoParam *msgrate_params = NULL;
static const SST::ElementInfoParam *ghost_params = NULL;
static const SST::ElementInfoParam *alltoall_params = NULL;
static const SST::ElementInfoParam *pingpong_params = NULL;
static const SST::ElementInfoParam *comm_params = NULL;

static SST::ElementInfoPort routermodel_ports[] = {
    {"R%dP%d", "Router Port", NULL},
    {"L%d", "Router Port", NULL},
    {NULL, NULL, NULL}
};


static SST::ElementInfoPort pattern_ports[] = {
    {"NoC", "OnChip Network", NULL},
    {"Net", "Network", NULL},
    {"Far", "Far Network", NULL},
    {"NVRAM", "NVRAM", NULL},
    {"STORAGE", "Storage", NULL},
    {NULL, NULL, NULL}
};


// THE LIST OF COMPONENTS CONTAINED WITHIN THE PATTERNS LIBRRY
static const ElementInfoComponent patterns_components[] = {
    { "routermodel",       "router model without power",        NULL, create_routermodel,       routermodel_params,         routermodel_ports,  COMPONENT_CATEGORY_NETWORK},
    { "routermodel_power", "router model with power",           NULL, create_routermodel_power, routermodel_power_params,   routermodel_ports,  COMPONENT_CATEGORY_NETWORK},
    { "bit_bucket",        "Bit bucket",                        NULL, create_bit_bucket,        bit_bucket_params,          pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "allreduce_pattern", "Allreduce pattern",                 NULL, create_allreduce_pattern, allreduce_params,           pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "fft_pattern",       "FFT pattern",                       NULL, create_fft_pattern,       fft_params,                 pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "msgrate_pattern",   "Message rate pattern",              NULL, create_msgrate_pattern,   msgrate_params,             pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "ghost_pattern",     "Ghost pattern",                     NULL, create_ghost_pattern,     ghost_params,               pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "alltoall_pattern",  "Alltoall pattern",                  NULL, create_alltoall_pattern,  alltoall_params,            pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "pingpong_pattern",  "Ping-pong pattern",                 NULL, create_pingpong_pattern,  pingpong_params,            pattern_ports,      COMPONENT_CATEGORY_NETWORK},
    { "comm_pattern",      "Communication pattern base object", NULL, create_comm_pattern,      comm_params,                pattern_ports,      COMPONENT_CATEGORY_NETWORK},
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

