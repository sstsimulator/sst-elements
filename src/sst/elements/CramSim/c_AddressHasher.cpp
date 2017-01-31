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
  k_addressMapStr = (string)x_params.find<string>("strAddressMapString", "__r:15__l:6__b:2__R:1__B:1__C:3__h:7__", l_found);
  cout << "Read address map string as: " << k_addressMapStr << endl;

  string l_mapCopy = k_addressMapStr;

  //first, remove all _'s
  l_mapCopy.erase(remove(l_mapCopy.begin(), l_mapCopy.end(), '_'), l_mapCopy.end());

  regex l_regex("([rlbRBCh])(:([[:digit:]]+))*$");
  smatch l_match;
  uint l_curPos = 0;

  while(!l_mapCopy.empty()) {
    cout << "address map is currently: " << l_mapCopy << endl;
  
    if(regex_search(l_mapCopy,l_match,l_regex)) {
      for(int ii=0;ii<stoi(l_match[3]);ii++) {
	m_bitPositions[l_match[1]].push_back(l_curPos);
	l_curPos++;
      }
      cout << "1 " << l_match[1] << " 3 " << l_match[3] << endl << endl;
    } else {
      cout << "Doesn't Match!!\n";
    }
    l_mapCopy = regex_replace(l_mapCopy,l_regex,""); // remove the matched portion
  }

  cout << "address map is currently: " << l_mapCopy << endl << endl;
}

c_AddressHasher* c_AddressHasher::getInstance(SST::Params& x_params) {
  if (nullptr == m_instance) {
    m_instance = new c_AddressHasher(x_params);
  }
  return m_instance;
}

c_AddressHasher* c_AddressHasher::getInstance() {
  if (nullptr == m_instance) {
    cerr << "Error:: c_AddressHasher getInstance() called without first constructing it using getInstance(SST::Params)" << endl;
    cerr << "The simulation must first construct on object that constructs the addressHasher (e.g. CmdUnit)" << endl;
    exit(-1);
  }
  return m_instance;
}

unsigned c_AddressHasher::getBankFromAddress(const unsigned x_address,
					     const unsigned x_numBanks) {

  assert(0==((x_numBanks)&(x_numBanks-1)));
  // make sure that the number of banks is a power of 2
  unsigned l_bank = 0;
  unsigned l_address = x_address >> 6;
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
unsigned c_AddressHasher::getBankFromAddress1(const unsigned x_address,
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

unsigned c_AddressHasher::getBankFromAddress2(const unsigned x_address,
					      const unsigned x_numBytesPerTransaction, const unsigned x_numChannels,
					      const unsigned x_numBanks) {

  unsigned l_result = (x_address >> static_cast<int>(log2(x_numBytesPerTransaction))); // add a shift by log2(x_numChannels)
  l_result = l_result % x_numBanks; // FIXME: this could be made faster by eliminating the mod operation

  return (l_result);
}

unsigned c_AddressHasher::getRowFromAddress(const unsigned x_address,
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
