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
#ifndef c_HASHEDADDRESS_HPP
#define c_HASHEDADDRESS_HPP

#include <iostream>

//sst includes
#include <sst/core/serialization/serializable.h>

//local includes

class c_HashedAddress : public SST::Core::Serialization::serializable {

public:
    c_HashedAddress(){};
  c_HashedAddress(unsigned x_channel, unsigned x_pchannel, unsigned x_rank, unsigned x_bankgroup,
  		  unsigned x_bank, unsigned x_row, unsigned x_col, unsigned x_bankid) : m_channel(x_channel), m_pchannel(x_pchannel), m_rank(x_rank), m_bankgroup(x_bankgroup),
                                                               m_bank(x_bank),m_row(x_row),m_col(x_col),m_bankId(x_bankid){};
  friend class c_AddressHasher;
  
  unsigned getChannel()   const {return m_channel;}
    unsigned getPChannel() const {return m_pchannel;}
  unsigned getRank()      const {return m_rank;}
  unsigned getBankGroup() const {return m_bankgroup;}
  unsigned getBank()      const {return m_bank;}
  unsigned getRow()       const {return m_row;}
  unsigned getCol()       const {return m_col;}
  unsigned getCacheline() const {return m_cacheline;}

  unsigned getBankId()    const {return m_bankId;} // linear bankId
    unsigned getRankId()  const {return m_rankId;}
    void setChannel(unsigned x_ch) {m_channel=x_ch;}
    void setPChannel(unsigned x_pch) {m_pchannel=x_pch;}
    void setRank(unsigned x_rank) {m_rank=x_rank;}
    void setBankGroup(unsigned x_bg) {m_bankgroup=x_bg;}
    void setBank(unsigned x_bank) {m_bank=x_bank;}
    void setRow(unsigned x_row) {m_row=x_row;}
    void setCol(unsigned x_col) {m_col=x_col;}
    void setCacheline(unsigned x_cacheline) {m_cacheline=x_cacheline;}
    void setBankId(unsigned x_bankid) {m_bankId=x_bankid;}
    void setRankId(unsigned x_rankid) {m_rankId=x_rankid;}


  void print() const;
  
  void serialize_order(SST::Core::Serialization::serializer &ser) override
  {
    ser & m_channel;
    ser & m_rank;
    ser & m_bankgroup;
    ser & m_bank;
    ser & m_row;
    ser & m_col;
    ser & m_cacheline;
    ser & m_bankId;
    ser & m_rankId;
  }
  
  ImplementSerializable(c_HashedAddress);

private:
  unsigned m_channel;
    unsigned m_pchannel;
  unsigned m_rank;
  unsigned m_bankgroup;
  unsigned m_bank;
  unsigned m_row;
  unsigned m_col;
  unsigned m_cacheline;

  unsigned m_bankId;
    unsigned m_rankId;
};

#endif // c_HASHEDADDRESS_HPP
