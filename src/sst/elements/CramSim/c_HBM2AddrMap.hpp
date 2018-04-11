// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2016 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef c_HBM2ADDRMAP_HPP
#define c_HBM2ADDRMAP_HPP

// sst includes
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/subcomponent.h>

#include <memory>
#include <map>

// local includes
//#include "c_BankCommand.hpp"
#include "c_HashedAddress.hpp"
#include "c_Controller.hpp"
#include "c_AddressHasher.hpp"

//<! This class holds information about global simulation state
//<! any object in the simulator can access this class

typedef unsigned long ulong;
namespace SST {
    namespace n_Bank {
        class c_Controller;

        class c_HBM2AddrMap : public c_AddressHasher {

        public:
            // Below is for calling in generic locations to obtain a pointer to the singleton instance
            c_HBM2AddrMap(Component *comp, Params &params);

            virtual void fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address);
        
        private:
            unsigned long k_burstBits;
            unsigned long k_colFillBits;
            unsigned long k_colFillMask;
            unsigned long k_chanBits;
            unsigned long k_chanMask;
            unsigned long k_pChanBits;
            unsigned long k_pChanMask;
            unsigned long k_groupBits;
            unsigned long k_groupMask;
            unsigned long k_bankBits;
            unsigned long k_bankMask;
            unsigned long k_colRemainBits;
            unsigned long k_colRemainMask;
            unsigned long k_rowMask;
        };
    }
}

#endif // c_ADDRESSHASHER_HPP
