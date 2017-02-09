// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include <sst/core/params.h>

#include <memory>
#include <map>

// local includes
#include "c_BankCommand.hpp"
#include "c_HashedAddress.hpp"

//<! This class holds information about global simulation state
//<! any object in the simulator can access this class

typedef unsigned long ulong;

class c_AddressHasher {

public:
  // Below is for calling in generic locations to obtain a pointer to the singleton instance
  static c_AddressHasher* getInstance(); 
  static c_AddressHasher* getInstance(SST::Params& x_params); // This reads the parameters and constructs the hash function

  const void fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address);
  const ulong getAddressForBankId(const unsigned x_bankId);

  unsigned getBankFromAddress(const ulong x_address,
			      const unsigned x_numBanks);
  unsigned getBankFromAddress1(const ulong x_address,
			       const unsigned x_numBanks);
  unsigned getBankFromAddress2(const ulong x_address,
			       const unsigned x_numBytesPerTransaction,
			       const unsigned x_numChannels, const unsigned x_numBanks);
  unsigned getRowFromAddress(const ulong x_address,
			     const unsigned x_numBytesPerTransaction, const unsigned x_numRows,
			     const unsigned x_numCols, const unsigned x_numChannels,
			     const unsigned x_numBanks);
private:
  static c_AddressHasher* m_instance; //<! shared_ptr to instance of this class

  c_AddressHasher()=delete;
  c_AddressHasher(const c_AddressHasher&)=delete;
  void operator=(const c_AddressHasher&)=delete;

  c_AddressHasher(SST::Params& x_params);

  unsigned k_pNumChannels;
  unsigned k_pNumRanks;
  unsigned k_pNumBankGroups;
  unsigned k_pNumBanks;
  unsigned k_pNumRows;
  unsigned k_pNumCols;
  unsigned k_pBurstSize;

  std::string k_addressMapStr = "rlbRBh";
  std::map<std::string, std::vector<uint> > m_bitPositions;
  std::map<std::string, uint> m_structureSizes;  // Used for checking that params agree
};

#endif // c_ADDRESSHASHER_HPP
