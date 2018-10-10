/******************************************************************************
  Copyright 2018 NTESS. Under the terms of Contract DE-NA0003525 with
 NTESS, the U.S. Government retains certain rights in this software.

 Copyright (c) 2018, NTESS
 All rights reserved.

 Portions are copyright of other developers:
 See the file CONTRIBUTORS.TXT in the top level directory
 the distribution for more information.

 This file is part of the SST software package. For license
 information, see the LICENSE file in the top level directory of the
 distribution.
  
******************************************************************************/

/******************************************************************************

 Author     : Jonathon W. Donaldson, Arun Rodrigues

 Project    : Spiking Temporal Processing Unit (STPU) / Generic Neural Architecture (GNA)

 Description: STPU/GNA User API Library

 Special Req: 

 Notes      : All signals ending with _n are active low.

******************************************************************************/

#ifndef _GNA_LIB_H_ /* prevent circular inclusions */
#define _GNA_LIB_H_ /* by using protection macros  */

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// DATA TYPES ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace Neuron_Loader_Types {

  // Neuron Configuration Transmission (NCT) (Floating-Point Format)
  typedef struct {
    float NrnThr; // Neuron Firing Potential
    float NrnMin; // Neuron Minimum Allowed Potential
    float NrnLkg; // Neuron Leakage Value
  } T_NctFl;

} // Neuron_Loader_Types

namespace White_Matter_Types {

  // White Matter Entry (WME) Format
  // AFR: Changed to uint16
  typedef struct {
    uint16_t SynStr; // Synaptic Strength
    uint16_t TmpOff; // Temporal Offset
    uint16_t SubAdr; // Sub-Address
    uint16_t Valid;  // Valid Flag
  } T_Wme;

} // White_Matter_Types

namespace Ctx_Seq_Mem_Types {

  // Context Sequence Entry (CSE) Format
  typedef struct {
    int      CtxNum; // Context Number
    uint32_t LifCnt; // LIF Count 
  } T_Cse;

} // Ctx_Seq_Mem_Types

namespace Ctrl_And_Stat_Types {

  // Brain Wave Pulse (BWP) Format (Floating-Point Format)
  typedef struct {
    float InpValFl; // Input Value
    int   InpNrn; // Input Neuron Number
    int   TmpSft; // Temporal Shift
  } T_BwpFl;

  // Dump Tuple Format
  typedef struct {
    int NrnNum; // Neuron Number
    int TmpIdx; // Temporal Index
  } T_Dmp;

} // Ctrl_And_Stat_Types


#endif // _GNA_LIB_H_

///////////////////////////////////////////////////////////////////////////////
/////////////////////////// OFFICIAL USE ONLY /////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
