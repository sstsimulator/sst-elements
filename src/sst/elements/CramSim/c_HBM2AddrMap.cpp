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

#include "c_HBM2AddrMap.hpp"

using namespace std;
using namespace SST;
using namespace n_Bank;


c_HBM2AddrMap::c_HBM2AddrMap(Component * comp, Params &params) : c_AddressHasher(comp, params) {

    // Init all to 0
    k_burstBits = 0;
    k_colFillBits = 0;
    k_chanBits = 0;
    k_pChanBits = 0;
    k_groupBits = 0;
    k_bankBits = 0;
    k_colRemainBits = 0;
    k_colFillMask = 0;
    k_chanMask = 0;
    k_pChanMask = 0;
    k_groupMask = 0;
    k_bankMask = 0;
    k_colRemainMask = 0;
    k_rowMask = 0;

    unsigned long temp;

    // Figure out how many address bits we need for a cache line
    unsigned long k_lineSize = params.find<unsigned long>("lineSize", 64); /* Actual line size */
    unsigned long k_lineBits = 0;
    temp = k_lineSize;
    while (temp >>= 1) 
        k_lineBits++;

    // Figure out how many address bits for the burst size
    temp = k_pBurstSize;
    while (temp >>= 1)
        k_burstBits++;
    
    // If burst bits < linesize then we need to fill with column bits
    k_colFillBits = k_lineBits - k_burstBits;
    for (unsigned int i = 0; i < k_colFillBits; i++)
        k_colFillMask = 1 + (k_colFillMask << 1);
    
    
    temp = k_pNumChannels;
    while(temp >>= 1) {
        k_chanBits++;
        k_chanMask = 1 + (k_chanMask << 1);
    }

    temp = k_pNumPseudoChannels;
    while(temp >>= 1) {
        k_pChanBits++;
        k_pChanMask = 1 + (k_pChanMask << 1);
    }

    temp = k_pNumBankGroups;
    while(temp >>=1) {
        k_groupBits++;
        k_groupMask = 1 + (k_groupMask << 1);
    }

    temp = k_pNumBanks;
    while(temp >>=1) {
        k_bankBits++;
        k_bankMask = 1 + (k_bankMask << 1);
    }

    unsigned long k_colBits = 0;
    temp = k_pNumCols;
    while (temp >>= 1) {
        k_colBits++;
    }
    if (k_colBits < k_colFillBits) {
        Output out("", 1, 0, Output::STDOUT);
        out.fatal(CALL_INFO, -1, "Error: Need %u column fill bits but only have %u column bits\n", k_colFillBits, k_colBits);
    }

    k_colRemainBits = k_colBits - k_colFillBits;
    for (unsigned int i = 0; i < k_colRemainBits; i++)
        k_colRemainMask = 1 + (k_colRemainMask << 1);

    temp = k_pNumRows;
    while (temp >>= 1)
        k_rowMask = 1 + (k_rowMask << 1);
    /*          Bits    Mask
     *  Burst:  5
     *  ColF:   1       1
     *  Chan:   3/4     7/F
     *  PChan:  1       1
     *  Group:  2       3
     *  Bank:   2       3
     *  ColR:   4       F
     *  Row:            3FFF
     */
    
    printf("Configuring with bits: %u,%u,%u,%u,%u,%u,%u and masks %lx,%lx,%lx,%lx,%lx,%lx,%lx\n",
            k_burstBits, k_colFillBits, k_chanBits, k_pChanBits, k_groupBits, k_bankBits, k_colRemainBits,
            k_colFillMask, k_chanMask, k_pChanMask, k_groupMask, k_bankMask, k_colRemainMask, k_rowMask);

} 


void c_HBM2AddrMap::fillHashedAddress(c_HashedAddress *x_hashAddr, const ulong x_address) {

    //cout << "Converting 0x" << std::hex << x_address;

    // Channel: bits 6-8
    unsigned long temp = x_address >> k_burstBits;
    //cout << "...(line): 0x" << temp;
    
    unsigned col = temp & k_colFillMask;
    temp = temp >> k_colFillBits;    // Pop lower bit
    //cout << "...(col): 0x" << col << "/" << temp;

    unsigned chan = temp & k_chanMask;
    temp = temp >> k_chanBits; // Pop lower 3 or 4
    //cout << "...(chan): 0x" <<chan << "/" <<  temp;
    
    unsigned pChan = temp & k_pChanMask;
    temp = temp >> k_pChanBits;
    //cout << "...(pchan): 0x" << pChan << "/" << temp;
    
    unsigned group = temp & k_groupMask;
    temp = temp >> k_groupBits;
    //cout << "...(bg): 0x" << group << "/" << temp;
    
    unsigned bank = temp & k_bankMask;
    temp = temp >> k_bankBits;
    ///cout << "...(bank): 0x" << bank << "/" << temp;
    
    unsigned colUpper = temp & k_colRemainMask;
    col = col | (colUpper << k_colFillBits);
    temp = temp >> k_colRemainBits;
    ///cout << "...(col): 0x" << col << "/" << colUpper << "/" << temp;
    
    unsigned row = temp & k_rowMask;
    //cout << "...(row): 0x" << row << "/" << temp << endl;
    
    x_hashAddr->setChannel(chan);
    x_hashAddr->setPChannel(pChan);
    x_hashAddr->setRank(0);
    x_hashAddr->setBankGroup(group);
    x_hashAddr->setBank(bank);
    x_hashAddr->setRow(row);
    x_hashAddr->setCol(col);
    x_hashAddr->setCacheline(0);
  
/*  unsigned l_bankId =
    x_hashAddr->getBank()
    + x_hashAddr->getBankGroup() * k_pNumBanks
    + x_hashAddr->getRank()    * k_pNumBanks * k_pNumBankGroups
    + x_hashAddr->getPChannel()  *  k_pNumBanks * k_pNumBankGroups * k_pNumRanks
    + x_hashAddr->getChannel()   * k_pNumPseudoChannels * k_pNumBanks * k_pNumBankGroups * k_pNumRanks;
*/
  unsigned l_bankId = 
      bank + (group << k_bankBits)
      + (pChan << (k_groupBits + k_bankBits))
      + (chan << (k_groupBits + k_bankBits + k_pChanBits));

/*  unsigned l_rankId =
            x_hashAddr->getRank()
          + x_hashAddr->getPChannel()  * k_pNumRanks
          + x_hashAddr->getChannel()   * k_pNumPseudoChannels * k_pNumRanks;
*/
  unsigned l_rankId = pChan + (chan << k_pChanBits);

  x_hashAddr->setBankId(l_bankId);
  x_hashAddr->setRankId(l_rankId);
    //cout << "0x" << std::hex << x_address << std::dec << "\t";  x_hashAddr->print();
  
} // fillHashedAddress(c_HashedAddress, x_address)
