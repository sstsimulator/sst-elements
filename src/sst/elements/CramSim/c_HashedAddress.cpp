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
#include "c_HashedAddress.hpp"

using namespace std;

//c_HashedAddress::c_HashedAddress(unsigned x_channel, unsigned x_rank, unsigned x_bankgroup,
//				 unsigned x_bank, unsigned x_row, unsigned x_col) :
//  m_channel(x_channel), m_rank(x_rank), m_bankgroup(x_bankgroup), m_bank(x_bank),
//  m_row(x_row), m_col(x_col)
//{
//}

void c_HashedAddress::print() const {
  std::cout << "Channel: " << m_channel << " PseudoChannel: "<<m_pchannel<<" Rank: " << m_rank
	    << " BankGroup: " << m_bankgroup << " Bank: " << m_bank
	    << " Row: " << m_row << " Col: " << m_col
	    << " Cacheline: " << m_cacheline
	    << "\tBankId: " << m_bankId
	    << std::endl;
}
