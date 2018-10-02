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
    namespace n_Bank {
        class c_Controller;

        class c_AddressHasher : public SubComponent {

        public:

            SST_ELI_REGISTER_SUBCOMPONENT(
                c_AddressHasher,
                "CramSim",
                "c_AddressHasher",
                SST_ELI_ELEMENT_VERSION(1,0,0),
                "Hashes addresses based on config parameters",
                "SST::CramSim::Controller::AddressHasher"
            )

            SST_ELI_DOCUMENT_PARAMS(
                {"numChannelsPerDimm", "Total number of channels per DIMM", NULL},
                {"numRanksPerChannel", "Total number of ranks per channel", NULL},
                {"numBankGroupsPerRank", "Total number of bank groups per rank", NULL},
                {"numBanksPerBankGroup", "Total number of banks per group", NULL},
                {"numRowsPerBank" "Number of rows in every bank", NULL},
                {"numColsPerBank", "Number of cols in every bank", NULL},
                {"numBytesPerTransaction", "Number of bytes retrieved for every transaction", NULL},
                {"strAddressMapStr","String defining the address mapping scheme",NULL},
            )

            SST_ELI_DOCUMENT_PORTS(
            )

            SST_ELI_DOCUMENT_STATISTICS(
            )

            // Below is for calling in generic locations to obtain a pointer to the singleton instance
            c_AddressHasher(Component *comp, Params &params);

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

            c_Controller* m_owner;
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
        };
    }
}

#endif // c_ADDRESSHASHER_HPP
