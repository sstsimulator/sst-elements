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



// Our system has three different kind of "NICs"
typedef enum {Net= 0, NoC, Far} NIC_model_t;
#define NUM_NIC_MODELS  (3)                     // Make sure this correponds to the above!
const char *type_name(NIC_model_t nic);		// Defined in NIC_model.cc


#endif  // _PATTERNS_H_
