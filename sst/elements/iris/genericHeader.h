/*
 * =====================================================================================
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
 * =====================================================================================
 */

#ifndef  _GENERICHEADER_H_INC
#define  _GENERICHEADER_H_INC

#include        <iostream>
#include        <sstream>
#include        <fstream>
#include        <string>
#include        <sys/time.h>
#include        <algorithm>
#include        <vector>
#include        <time.h>
#include        <signal.h>
#include        <sys/types.h>
#include        <unistd.h>
#include	<deque>
#include	<vector>
#include	<stdint.h>
#include        <cstdlib>

// cool trick to disable all asserts during a run from C++ in a nutshell
// to enable just comment the define to NDEBUG
//#define NDEBUG
#include	<cassert>

// SST related
#include	<sst/core/debug.h>
#include        <sst/core/serialization.h>
#include        <sst/core/component.h>
#include        <sst/core/sst_types.h>


namespace SST {
namespace Iris {

/*  Enums for router configurations.. if you change them modify the string in
 *  genericHeader.cc */
enum message_class_t { INVALID_PKT=0, PROC_REQ, MC_RESP, PROC_READ_REQ, PROC_WRITE_REQ };

enum routing_scheme_t{ 
    TWONODE_ROUTING,  // Simple send<--> recv between two nodes
    XY, // bidirectional mesh
    TORUS_ROUTING, // bidirectional torus
    RING_ROUTING // one-dimension unidirection torus.. NOT A RING!
};

enum ports_t{
    nic = 0,
    xPos,
    xNeg,
    yPos,
    yNeg,
    zPos,
    zNeg,
    INV
};

/*  Make swaping DES kernels cleaner */
typedef class SST::Component DES_Component;
typedef class SST::Link DES_Link;
typedef class SST::Event DES_Event;
typedef class SST::Clock DES_Clock;

/* Timing helpers */
#define _DES_SEND SST::Send
#define _NOC_FREQ "1GHz"
#define _TICK_NOW getCurrentSimTimeNano()

/* Name ports if its easy to track */
#define EJECT_PORT 0

#define iris_panic(...) printf("panic"); std::cerr << "warn"<< __FUNCTION__ \
   <<  __FILE__<< __LINE__<< __VA_ARGS__ ; exit(1)

#define iris_warn(...) \
  std::cerr << "warn"<< __FUNCTION__ <<  __FILE__<< __LINE__<< __VA_ARGS__ 

}
}
#endif   /* ----- #ifndef _GENERICHEADER_H_INC  ----- */
