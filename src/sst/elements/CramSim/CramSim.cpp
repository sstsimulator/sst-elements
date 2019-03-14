// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SST includes
#include "sst_config.h"

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"

// local includes
#include "c_MemhBridge.hpp"
#include "c_AddressHasher.hpp"
#include "c_TxnConverter.hpp"
#include "c_TxnScheduler.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"
#include "c_DeviceDriver.hpp"
#include "c_Dimm.hpp"
#include "c_Controller.hpp"
#include "c_TxnDispatcher.hpp"
#include "c_TxnGen.hpp"
#include "c_TraceFileReader.hpp"


// namespaces
using namespace SST;
using namespace SST::n_Bank;
using namespace SST::Statistics;

