//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
// Common declarations for the files in this directory

#ifndef _PATTERNS_H_
#define _PATTERNS_H_

// Make sure those two correspond
#define TIME_BASE		"ns"
#define TIME_BASE_FACTOR	1000000000.0

// Everybody needs those
#include <sst_config.h>
#include <sst/core/serialization.h>


// Our system has three different kind of "NICs"
typedef enum {Net= 0, NoC, Far} NIC_model_t;
#define NUM_NIC_MODELS  (3)                     // Make sure this correponds to the above!
const char *type_name(NIC_model_t nic);		// Defined in NIC_model.cc


/*
// THIS SECTION REMOVED TO FORCE ELI STRUCTURES TO BE BUILD IN patterns.cc FOR RELEASE 3.x OF SST - ALEVINE

// Macro to fill in eli structure needed in dynamic libraries
// Required for all components loadable by SST. In this
// directory, this means all components that inherit from Comm_pattern
// Use it like this: eli(Allreduce_pattern, allreduce_pattern, "Allreduce pattern")
// where the first argument is the name of the Comm_pattern object, the second
// argument is the name of the resulting library, and the last argument is
// a brief description.

#define eli(comp, lib, desc) \
static SST::Component *create_##comp(SST::ComponentId_t id, SST::Component::Params_t& params) \
{ \
    return new comp(id, params); \
} \
 \
static const SST::ElementInfoComponent components[]= { \
    {#lib, desc, NULL, create_##comp}, \
    {NULL, NULL, NULL, NULL} \
}; \
 \
extern "C" { \
    SST::ElementLibraryInfo lib##_eli= { \
       #lib, "Trying to figure this out", components \
    }; \
} \
*/

#endif  // _PATTERNS_H_
