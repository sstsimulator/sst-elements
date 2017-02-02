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
#ifndef c_HASHEDADDRESS_HPP
#define c_HASHEDADDRESS_HPP

#include <iostream>

// local includes

class c_HashedAddress {

public:
  //c_HashedAddress(unsigned x_channel, unsigned x_rank, unsigned x_bankgroup,
  //		  unsigned x_bank, unsigned x_row, unsigned x_col);
  friend class c_AddressHasher;
  
  unsigned getChannel()   {return m_channel;}
  unsigned getRank()      {return m_row;}
  unsigned getBankGroup() {return m_bankgroup;}
  unsigned getBank()      {return m_bank;}
  unsigned getRow()       {return m_row;}
  unsigned getCol()       {return m_col;}
  unsigned getCacheline() {return m_cacheline;}

  void print() const;

private:
  unsigned m_channel;
  unsigned m_rank;
  unsigned m_bankgroup;
  unsigned m_bank;
  unsigned m_row;
  unsigned m_col;
  unsigned m_cacheline;
};

#endif // c_HASHEDADDRESS_HPP
