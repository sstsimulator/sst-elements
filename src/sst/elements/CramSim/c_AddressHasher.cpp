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

// Copyright 2015 IBM Corporation

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//   http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <regex>

#include "c_AddressHasher.hpp"

using namespace std;

c_AddressHasher* c_AddressHasher::m_instance = nullptr;

c_AddressHasher::c_AddressHasher(SST::Params& x_params) {
  // read params here
  bool l_found = false;
  k_addressMapStr = (string)x_params.find<string>("strAddressMapString", "__r:15__l:7__b:2__R:1__B:2__h:6__", l_found);
  cout << "Address map string: " << k_addressMapStr << endl;

  string l_mapCopy = k_addressMapStr;

  //first, remove all _'s
  l_mapCopy.erase(remove(l_mapCopy.begin(), l_mapCopy.end(), '_'), l_mapCopy.end());

  regex l_regex("([rlbRBCh])(:([[:digit:]]+))*$");
  smatch l_match;
  uint l_curPos = 0;

  while(!l_mapCopy.empty()) {
    if(regex_search(l_mapCopy,l_match,l_regex)) {
      //cout << "1 " << l_match[1] << " 3 " << l_match[3] << endl << endl;
      m_structureSizes[l_match[1]] = stoi(l_match[3]);
      for(int ii=0;ii<stoi(l_match[3]);ii++) {
	m_bitPositions[l_match[1]].push_back(l_curPos);
	l_curPos++;
      }
    } else {
      cerr << "Unable to parse address map string! Incorrectly formatted! Aborting\n";
      exit(-1);
    }
    l_mapCopy = regex_replace(l_mapCopy,l_regex,""); // remove the matched portion
  } // while !l_mapCopy.empty()

  
  //
  // now verify that the address map and other params make sense
  // Channels
  auto l_pNumChannels = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,l_found);
  auto l_it = m_structureSizes.find("C"); // channel
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumChannels > 1) {
      cerr << "Number of Channels (" << l_pNumChannels << ") is greater than 1, but no "
	   << "Channels were specified (C) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumChannels = (1 << l_it->second);
    if(l_aNumChannels > l_pNumChannels) {
      cerr << "Warning!: Number of address map channels is larger than numChannelsPerDimm." << endl;
      cerr << "Some channels will be unused" << endl << endl;
    }
    if(l_aNumChannels < l_pNumChannels) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map channels is smaller than numChannelsPerDimm. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Ranks
  auto l_pNumRanks = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 1,l_found);
  l_it = m_structureSizes.find("R");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumRanks > 1) {
      cerr << "Number of Ranks (" << l_pNumRanks << ") is greater than 1, but no "
	   << "Ranks were specified (R) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumRanks = (1 << l_it->second);
    if(l_aNumRanks > l_pNumRanks) {
      cerr << "Warning!: Number of address map Ranks is larger than numRanksPerChannel." << endl;
      cerr << "Some Ranks will be unused" << endl << endl;
    }
    if(l_aNumRanks < l_pNumRanks) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map Ranks is smaller than numRanksPerChannel. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // BankGroups
  auto l_pNumBankGroups = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank", 1,l_found);
  l_it = m_structureSizes.find("B");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumBankGroups > 1) {
      cerr << "Number of BankGroups (" << l_pNumBankGroups << ") is greater than 1, but no "
	   << "BankGroups were specified (B) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumBankGroups = (1 << l_it->second);
    if(l_aNumBankGroups > l_pNumBankGroups) {
      cerr << "Warning!: Number of address map bankGroups is larger than numBankGroupsPerRank." << endl;
      cerr << "Some BankGroups will be unused" << endl << endl;
    }
    if(l_aNumBankGroups < l_pNumBankGroups) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map bankGroups is smaller than numBankGroupsPerRank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Banks
  auto l_pNumBanks = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup", 1,l_found);
  l_it = m_structureSizes.find("b");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumBanks > 1) {
      cerr << "Number of Banks (" << l_pNumBanks << ") is greater than 1, but no "
	   << "Banks were specified (b) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumBanks = (1 << l_it->second);
    if(l_aNumBanks > l_pNumBanks) {
      cerr << "Warning!: Number of address map Banks is larger than numBanksPerBankGroup." << endl;
      cerr << "Some Banks will be unused" << endl << endl;
    }
    if(l_aNumBanks < l_pNumBanks) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map Banks is smaller than numBanksPerBankGroup. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Rows
  auto l_pNumRows = (uint32_t)x_params.find<uint32_t>("numRowsPerBank", 1,l_found);
  l_it = m_structureSizes.find("r");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumRows > 1) {
      cerr << "Number of Rows (" << l_pNumRows << ") is greater than 1, but no "
	   << "Rows were specified (r) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumRows = (1 << l_it->second);
    if(l_aNumRows > l_pNumRows) {
      cerr << "Warning!: Number of address map Rows is larger than numRowsPerBank." << endl;
      cerr << "Some Rows will be unused" << endl << endl;
    }
    if(l_aNumRows < l_pNumRows) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map Rows is smaller than numRowsPerBank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Cols
  auto l_pNumCols = (uint32_t)x_params.find<uint32_t>("numColsPerBank", 1,l_found);
  l_it = m_structureSizes.find("l");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pNumCols > 1) {
      cerr << "Number of Cols (" << l_pNumCols << ") is greater than 1, but no "
	   << "Cols were specified (l) [el] in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumCols = (1 << l_it->second);
    if(l_aNumCols > l_pNumCols) {
      cerr << "Warning!: Number of address map Cols is larger than numColsPerBank." << endl;
      cerr << "Some Cols will be unused" << endl << endl;
    }
    if(l_aNumCols < l_pNumCols) { // some addresses have nowhere to go
      cerr << "Error!: Number of addrss map Cols is smaller than numColsPerBank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Cacheline/Burst size
  auto l_pBurstSize = (uint32_t)x_params.find<uint32_t>("numBytesPerTransaction", 1,l_found);
  l_it = m_structureSizes.find("h");
  if(l_it == m_structureSizes.end()) { // if not found
    if(l_pBurstSize > 1) {
      cerr << "Burst size (" << l_pBurstSize << ") is greater than 1, but no "
	   << "Cachelines were specified (h) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumCachelines = (1 << l_it->second);
    if(l_aNumCachelines != l_pBurstSize) {
      cerr << "Error!: Number of address map Cachelines is not equal to numBytesPerTransaction." << endl;
      cerr << "Make sure that the address map cachelines (h) are equal to numBytesPerTransaction (i.e. 2**h == numByutesPerTransaction!" << endl;
      exit(-1);
    }
  } // else found in map

} // c_AddressHasher(SST::Params)

c_AddressHasher* c_AddressHasher::getInstance(SST::Params& x_params) {
  if (nullptr == m_instance) {
    m_instance = new c_AddressHasher(x_params);
  }
  return m_instance;
}

c_AddressHasher* c_AddressHasher::getInstance() {
  if (nullptr == m_instance) {
    cerr << "Error:: c_AddressHasher getInstance() called without first constructing it using getInstance(SST::Params)" << endl;
    cerr << "The simulation must first construct an object that constructs the addressHasher (e.g. CmdUnit)" << endl;
    exit(-1);
  }
  return m_instance;
}

const void c_AddressHasher::fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address) {
  ulong l_cur=0;
  ulong l_cnt=0;
  
  //channel
  auto l_bitPos = m_bitPositions.find("C");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_channel = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_channel = l_cur;
  }
  
  //rank
  l_bitPos = m_bitPositions.find("R");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_rank = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_rank = l_cur;
  }

  //bankGroup
  l_bitPos = m_bitPositions.find("B");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_bankgroup = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_bankgroup = l_cur;
  }

  //bank
  l_bitPos = m_bitPositions.find("b");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_bank = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_bank = l_cur;
  }

  //row
  l_bitPos = m_bitPositions.find("r");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_row = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_row = l_cur;
  }

  //col
  l_bitPos = m_bitPositions.find("l");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_col = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_col = l_cur;
  }

  //cacheline
  l_bitPos = m_bitPositions.find("h");
  if(l_bitPos == m_bitPositions.end()) { // not found
    x_hashAddr->m_cacheline = 0;
  } else {
    l_cur=0;
    for(l_cnt=0;l_cnt<l_bitPos->second.size();l_cnt++) {
      ulong l_val = l_bitPos->second[l_cnt];
      ulong l_tmp = ((1 << l_val) & x_address) >> (l_val - l_cnt);

      l_cur |= l_tmp;
    }
    x_hashAddr->m_cacheline = l_cur;
  }

  //cout << "0x" << std::hex << x_address << std::dec << "\t";
  //x_hashAddr->print();
} // fillHashedAddress(c_HashedAddress, x_address)


unsigned c_AddressHasher::getBankFromAddress(const ulong x_address,
					     const unsigned x_numBanks) {

  assert(0==((x_numBanks)&(x_numBanks-1)));
  // make sure that the number of banks is a power of 2
  unsigned l_bank = 0;
  ulong l_address = x_address >> 6;
  if (0 == (x_address) % 2) {
    // even
    l_bank = ((l_address / 2) % (x_numBanks / 2));
  } else {
    // odd
    l_bank = ((((l_address - 1) / 2) + (x_numBanks / 2)) % x_numBanks);
  }
  return (l_bank);
}

// FIXME::CRITICAL: Copied from USimm. Do not use in production code
unsigned c_AddressHasher::getBankFromAddress1(const ulong x_address,
					      const unsigned x_numBanks) {
  long long int input_a, temp_b, temp_a;

  int byteOffsetWidth = 5;
  int channelBitWidth = 0;
  int rankBitWidth = 1;
  int bankBitWidth = static_cast<int>(log2(x_numBanks));
  // int bankBitWidth = 4;
  int rowBitWidth = 16;
  int colBitWidth = 7;

  input_a = x_address;
  input_a = input_a >> byteOffsetWidth;

  temp_b = input_a;
  input_a = input_a >> bankBitWidth;
  temp_a = input_a << bankBitWidth;
  return temp_a ^ temp_b; // strip out the bank address

}

unsigned c_AddressHasher::getBankFromAddress2(const ulong x_address,
					      const unsigned x_numBytesPerTransaction, const unsigned x_numChannels,
					      const unsigned x_numBanks) {

  unsigned l_result = (x_address >> static_cast<int>(log2(x_numBytesPerTransaction))); // add a shift by log2(x_numChannels)
  l_result = l_result % x_numBanks; // FIXME: this could be made faster by eliminating the mod operation

  return (l_result);
}

unsigned c_AddressHasher::getRowFromAddress(const ulong x_address,
					    const unsigned x_numBytesPerTransaction, const unsigned x_numChannels,
					    const unsigned x_numColsPerBank, const unsigned x_numRowsPerBank,
					    const unsigned x_numBanks) {

  unsigned l_result = (((x_address >> static_cast<int>(log2(x_numBytesPerTransaction)))
			>> static_cast<int>(log2(x_numBanks))) >> static_cast<int>(log2(x_numColsPerBank)));

  //	cout << "l_result = " << hex << l_result << endl;

  l_result = l_result % x_numRowsPerBank; // FIXME: this could be made faster by eliminating the mod operation

  //	cout << "x_numRowsPerBank = " << dec << x_numRowsPerBank << endl;
  //	cout << "l_result = " << hex << l_result << endl;

  return (l_result);
}
