// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#ifndef c_ADDRESSHASHER_HPP
#define c_ADDRESSHASHER_HPP

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


//<! This class holds information about global simulation state
//<! any object in the simulator can access this class

typedef unsigned long ulong;
namespace SST {
    namespace CramSim {
        class c_Controller;

        class c_AddressHasher : public SubComponent {

        public:

            SST_ELI_REGISTER_SUBCOMPONENT_API(SST::CramSim::c_AddressHasher, Output*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned)

            SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
                c_AddressHasher,
                "CramSim",
                "c_AddressHasher",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Hashes addresses based on config parameters",
                SST::CramSim::c_AddressHasher
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"numBytesPerTransaction", "Number of bytes retrieved for every transaction", "1"},
                {"strAddressMapStr","String defining the address mapping scheme","_r_l_b_R_B_h_"},
            )

            SST_ELI_DOCUMENT_PORTS(
            )

            SST_ELI_DOCUMENT_STATISTICS(
            )

            // Below is for calling in generic locations to obtain a pointer to the singleton instance
            c_AddressHasher(ComponentId_t id, Params &params, Output* out, unsigned channels, unsigned ranks, unsigned bankGroups, unsigned banks, unsigned rows, unsigned cols, unsigned pChannels);
            void build(Params &params); // Temporary until subcomponent api transition is complete

            static c_AddressHasher *getInstance();

            static c_AddressHasher *
            getInstance(Params &x_params); // This reads the parameters and constructs the hash function

            void fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address);

        private:

            c_AddressHasher() = delete;

            c_AddressHasher(const c_AddressHasher &) = delete;

            void operator=(const c_AddressHasher &)= delete;

            c_AddressHasher(Params &x_params);
            ulong getAddressForBankId(const unsigned x_bankId);

            unsigned k_pNumChannels;
            unsigned k_pNumRanks;
            unsigned k_pNumBankGroups;
            unsigned k_pNumBanks;
            unsigned k_pNumRows;
            unsigned k_pNumCols;
            unsigned k_pBurstSize;
            unsigned k_pNumPseudoChannels;

            std::string k_addressMapStr = "rlbRBh";
            std::map<std::string, std::vector<uint> > m_bitPositions;
            std::map<std::string, uint> m_structureSizes;  // Used for checking that params agree

            // regex replacement stuff
            void parsePattern(std::string *x_inStr, std::pair<std::string, uint> *x_outPair);

            Output* output;
        };
    }
}

#endif // c_ADDRESSHASHER_HPP
