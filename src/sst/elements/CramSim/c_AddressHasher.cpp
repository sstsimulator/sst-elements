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

c_AddressHasher::c_AddressHasher(SST::ComponentId_t x_id, SST::Params& x_params) :
  Component(x_id) {

  getInstance(x_params);
  
}

c_AddressHasher::c_AddressHasher(SST::Params& x_params) {
  // read params here
  bool l_found = false;
  k_addressMapStr = (string)x_params.find<string>("strAddressMapStr", "_r_l_b_R_B_h_", l_found);
  cout << "Address map string: " << k_addressMapStr << endl;

  string l_mapCopy = k_addressMapStr;

  //first, remove all _'s
  l_mapCopy.erase(remove(l_mapCopy.begin(), l_mapCopy.end(), '_'), l_mapCopy.end());
  // remove trailing newline
  l_mapCopy.erase(remove(l_mapCopy.begin(), l_mapCopy.end(), '\n'), l_mapCopy.end());

  /* This code uses regex, not supported in gcc 4.8.x
   * use the crappy crap code I implemented to replace it until compiler versions advance
  regex l_regex("([rlbRBCh])(:([[:digit:]]+))*$");
  smatch l_match;
  uint l_curPos = 0;
  vector<string> l_simpleOrder;

  while(!l_mapCopy.empty()) {
    if(regex_search(l_mapCopy,l_match,l_regex)) {
      string l_bitstr = l_match[3];
      if(l_match.length() == 1) {
	l_bitstr = "1";
      }
      //cout << "1 " << l_match[1] << " 3 " << l_bitstr << " length " << l_match.length() << endl;
      m_structureSizes[l_match[1]] += stoi(l_bitstr);
      for(int ii=0; ii < stoi(l_bitstr); ii++) {
	m_bitPositions[l_match[1]].push_back(l_curPos);
	l_curPos++;
      }
      l_simpleOrder.push_back(l_match[1]);
    } else {
      cerr << "Unable to parse address map string! Incorrectly formatted! Aborting\n";
      cerr << "Parsed until " << l_mapCopy << endl;
      exit(-1);
    }
    l_mapCopy = regex_replace(l_mapCopy,l_regex,string("")); // remove the matched portion
  } // while !l_mapCopy.empty()
  */

  // this is replacement code for the above, which is generally safer than my implemention
  // of parsePattern below
  uint l_curPos = 0;
  vector<string> l_simpleOrder;
  pair<string,uint> l_parsedData;

  while(!l_mapCopy.empty()) {
    parsePattern(&l_mapCopy, &l_parsedData);
      m_structureSizes[l_parsedData.first] += l_parsedData.second;
      for(int ii=0; ii < l_parsedData.second; ii++) {
	m_bitPositions[l_parsedData.first].push_back(l_curPos);
	l_curPos++;
      }
      l_simpleOrder.push_back(l_parsedData.first);
    
  } // while !l_mapCopy.empty()


  // pull config file sizes
  k_pNumChannels = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,l_found);
  k_pNumRanks = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 1,l_found);
  k_pNumBankGroups = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank", 1,l_found);
  k_pNumBanks = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup", 1,l_found);
  k_pNumRows = (uint32_t)x_params.find<uint32_t>("numRowsPerBank", 1,l_found);
  k_pNumCols = (uint32_t)x_params.find<uint32_t>("numColsPerBank", 1,l_found);
  k_pBurstSize = (uint32_t)x_params.find<uint32_t>("numBytesPerTransaction", 1,l_found);
  
  // check for simple version address map
  bool l_allSimple = true;
  for(auto l_iter : m_bitPositions) {
    if(l_iter.second.size() > 1) {
      l_allSimple = false;
      break;
    }
  }
  
  if(l_allSimple) { // if simple address detected, reset the bitPositions structureSizes
    // reset bitPositions
    for(auto l_iter : l_simpleOrder) {
      m_bitPositions[l_iter].clear();
    }

    l_curPos = 0;
    map<string, uint> l_cfgBits;
    l_cfgBits["C"] = (uint)log2(k_pNumChannels);
    l_cfgBits["R"] = (uint)log2(k_pNumRanks);
    l_cfgBits["B"] = (uint)log2(k_pNumBankGroups);
    l_cfgBits["b"] = (uint)log2(k_pNumBanks);
    l_cfgBits["r"] = (uint)log2(k_pNumRows);
    l_cfgBits["l"] = (uint)log2(k_pNumCols);
    l_cfgBits["h"] = (uint)log2(k_pBurstSize);

    for(auto l_iter : l_simpleOrder) {
      m_structureSizes[l_iter] = l_cfgBits[l_iter];
      for(int ii = 0; ii < l_cfgBits[l_iter]; ii++) {
	m_bitPositions[l_iter].push_back(l_curPos);
	l_curPos++;
      }
    }
    
  } // if(allSimple)
  
  //
  // now verify that the address map and other params make sense
  //
  
  // Channels
  auto l_it = m_structureSizes.find("C"); // channel
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumChannels > 1) {
      cerr << "Number of Channels (" << k_pNumChannels << ") is greater than 1, but no "
	   << "Channels were specified (C) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumChannels = (1 << l_it->second);
    if(l_aNumChannels > k_pNumChannels) {
      cerr << "Warning!: Number of address map channels is larger than numChannelsPerDimm." << endl;
      cerr << "Some channels will be unused" << endl << endl;
    }
    if(l_aNumChannels < k_pNumChannels) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map channels is smaller than numChannelsPerDimm. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Ranks
  l_it = m_structureSizes.find("R");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumRanks > 1) {
      cerr << "Number of Ranks (" << k_pNumRanks << ") is greater than 1, but no "
	   << "Ranks were specified (R) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumRanks = (1 << l_it->second);
    if(l_aNumRanks > k_pNumRanks) {
      cerr << "Warning!: Number of address map Ranks is larger than numRanksPerChannel." << endl;
      cerr << "Some Ranks will be unused" << endl << endl;
    }
    if(l_aNumRanks < k_pNumRanks) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map Ranks is smaller than numRanksPerChannel. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // BankGroups
  l_it = m_structureSizes.find("B");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumBankGroups > 1) {
      cerr << "Number of BankGroups (" << k_pNumBankGroups << ") is greater than 1, but no "
	   << "BankGroups were specified (B) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumBankGroups = (1 << l_it->second);
    if(l_aNumBankGroups > k_pNumBankGroups) {
      cerr << "Warning!: Number of address map bankGroups is larger than numBankGroupsPerRank." << endl;
      cerr << "Some BankGroups will be unused" << endl << endl;
    }
    if(l_aNumBankGroups < k_pNumBankGroups) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map bankGroups is smaller than numBankGroupsPerRank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Banks
  l_it = m_structureSizes.find("b");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumBanks > 1) {
      cerr << "Number of Banks (" << k_pNumBanks << ") is greater than 1, but no "
	   << "Banks were specified (b) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumBanks = (1 << l_it->second);
    if(l_aNumBanks > k_pNumBanks) {
      cerr << "Warning!: Number of address map Banks is larger than numBanksPerBankGroup." << endl;
      cerr << "Some Banks will be unused" << endl << endl;
    }
    if(l_aNumBanks < k_pNumBanks) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map Banks is smaller than numBanksPerBankGroup. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Rows
  l_it = m_structureSizes.find("r");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumRows > 1) {
      cerr << "Number of Rows (" << k_pNumRows << ") is greater than 1, but no "
	   << "Rows were specified (r) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumRows = (1 << l_it->second);
    if(l_aNumRows > k_pNumRows) {
      cerr << "Warning!: Number of address map Rows is larger than numRowsPerBank." << endl;
      cerr << "Some Rows will be unused" << endl << endl;
    }
    if(l_aNumRows < k_pNumRows) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map Rows is smaller than numRowsPerBank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Cols
  l_it = m_structureSizes.find("l");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pNumCols > 1) {
      cerr << "Number of Cols (" << k_pNumCols << ") is greater than 1, but no "
	   << "Cols were specified (l) [el] in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumCols = (1 << l_it->second);
    if(l_aNumCols > k_pNumCols) {
      cerr << "Warning!: Number of address map Cols is larger than numColsPerBank." << endl;
      cerr << "Some Cols will be unused" << endl << endl;
    }
    if(l_aNumCols < k_pNumCols) { // some addresses have nowhere to go
      cerr << "Error!: Number of address map Cols is smaller than numColsPerBank. Aborting!" << endl;
      exit(-1);
    }
  } // else found in map

  // Cacheline/Burst size
  l_it = m_structureSizes.find("h");
  if(l_it == m_structureSizes.end()) { // if not found
    if(k_pBurstSize > 1) {
      cerr << "Burst size (" << k_pBurstSize << ") is greater than 1, but no "
	   << "Cachelines were specified (h) in the address map! Aborting!" << endl;
      exit(-1);
    }
  } else { // found in map
    auto l_aNumCachelines = (1 << l_it->second);
    if(l_aNumCachelines != k_pBurstSize) {
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

void c_AddressHasher::fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address) {
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
  
  x_hashAddr->m_bankId =
    x_hashAddr->m_bank
    + x_hashAddr->m_bankgroup * k_pNumBanks
    + x_hashAddr->m_rank      * k_pNumBanks * k_pNumBankGroups
    + x_hashAddr->m_channel   * k_pNumBanks * k_pNumBankGroups * k_pNumRanks;
    
  //cout << "0x" << std::hex << x_address << std::dec << "\t";  x_hashAddr->print();
  
} // fillHashedAddress(c_HashedAddress, x_address)

ulong c_AddressHasher::getAddressForBankId(const unsigned x_bankId) {
  // obtain the bank group rank and channel of this bankId;
  unsigned l_cur = x_bankId;
  unsigned l_chanSize = k_pNumBanks * k_pNumBankGroups * k_pNumRanks;
  unsigned l_rankSize = k_pNumBanks * k_pNumBankGroups;
  unsigned l_bankGroupSize = k_pNumBanks;

  unsigned l_chan=0,l_rank=0,l_bankgroup=0,l_bank=0;

  cout << "Getting an address for bankId " << x_bankId << endl;

  while(l_cur >= l_chanSize) {
    l_cur -= l_chanSize;
    l_chan++;
  }

  while(l_cur >= l_rankSize) {
    l_cur -= l_rankSize;
    l_rank++;
  }
  
  while(l_cur >= l_bankGroupSize) {
    l_cur -= l_bankGroupSize;
    l_bankgroup++;
  }

  l_bank = l_cur;

  cout << "Final " << l_chan << " " << l_rank << " " << l_bankgroup << " " << l_bank << endl;

  ulong l_address = 0;
  {
    ulong l_tmp = l_bank;
    ulong l_tOut = 0;
    unsigned l_curPos = 0;
    while(l_tmp) {
      l_tOut += (l_tmp & 0x1) << m_bitPositions["b"][l_curPos];
      l_tmp >>= 1;
      l_curPos++;
    }

    l_address += l_tOut;
  }

  {
    ulong l_tmp = l_bankgroup;
    ulong l_tOut = 0;
    unsigned l_curPos = 0;
    while(l_tmp) {
      l_tOut += (l_tmp & 0x1) << m_bitPositions["B"][l_curPos];
      l_tmp >>= 1;
      l_curPos++;
    }
    
    l_address += l_tOut;
  }

  {
    ulong l_tmp = l_rank;
    ulong l_tOut = 0;
    unsigned l_curPos = 0;
    while(l_tmp) {
      l_tOut += (l_tmp & 0x1) << m_bitPositions["R"][l_curPos];
      l_tmp >>= 1;
      l_curPos++;
    }
    
    l_address += l_tOut;
  }

  {
    ulong l_tmp = l_chan;
    ulong l_tOut = 0;
    unsigned l_curPos = 0;
    while(l_tmp) {
      l_tOut += (l_tmp & 0x1) << m_bitPositions["C"][l_curPos];
      l_tmp >>= 1;
      l_curPos++;
    }
    
    l_address += l_tOut;
  }

  cout << "Returning address 0x" << std::hex << l_address << std::dec << endl;
  
  return(l_address);
} // getAddressForBankId(const unsigned x_bankId)

// parsePattern
// takes in complete or partially parsed address map string
// returns a pair with the matched string and the size of the field
// also removes the matched portion of the pattern from l_inStr
//
// everything is parsed from the end of the string backwards
void c_AddressHasher::parsePattern(string *x_inStr, std::pair<string,uint> *x_outPair) {
  assert(x_inStr != nullptr);
  assert(x_outPair != nullptr);

  if(x_inStr->empty()) {
    x_outPair->first = "";
    return;
  }

  string l_sizeStr("");

  bool l_matched=false;
  bool l_sizeMatched = false;
  
  auto l_sIter = x_inStr->rbegin();
  while(!l_matched) {
    if(isdigit(*l_sIter)) {
      if(l_sizeMatched) {
	cerr << "Weird parsing detected!" << endl;
	cerr << "Parsing error at " << *l_sIter << " in address map string " << *x_inStr << endl;
	exit(-1);
      }
      l_sizeStr = *l_sIter + l_sizeStr;
    } else if(isalpha(*l_sIter)) {
      if(!(*l_sIter == 'r' || *l_sIter == 'l' || *l_sIter == 'R' || *l_sIter == 'B' ||
	   *l_sIter == 'b' || *l_sIter == 'C' || *l_sIter == 'h')) {
	cerr << "Parsing error at " << *l_sIter << " in address map string " << *x_inStr << endl;
	exit(-1);
      }

      x_outPair->first = *l_sIter;
      if(l_sizeMatched) {
	x_outPair->second = stoi(l_sizeStr);
      } else {
	x_outPair->second = 1;
      }
      
      x_inStr->erase(next(l_sIter).base(),x_inStr->end()); // remove the matched portion
      //cout << "Returning " << x_outPair->first << " " << x_outPair->second << endl;
      break;
    } else if(*l_sIter == ':') {
      l_sizeMatched = true;
    } else {
      cerr << "Parsing error at " << *l_sIter << " in address map string " << *x_inStr << endl;
      exit(-1);
    }
    l_sIter++;
    if(l_sIter == x_inStr->rend()) {
      break;
    }
  } // while(!l_matched)
} // parsePattern(string, pair<string,uint>)
