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
#ifndef C_BANKCOMMAND_HPP
#define C_BANKCOMMAND_HPP

#include <ostream>
#include <map>
#include <string>

//sst includes
#include <sst/core/serialization/serializable.h>

//local includes
//#include "c_Transaction.hpp"
#include "c_HashedAddress.hpp"

typedef unsigned long ulong;

namespace SST {
namespace n_Bank {

class c_Transaction;

enum class e_BankCommandType {
	ERR, ACT, READ, READA, WRITE, WRITEA, PRE, PREA, REF, PDX, PDE
};

class c_BankCommand : public SST::Core::Serialization::serializable {

private:

	unsigned m_seqNum;
	ulong    m_addr;
	unsigned m_row;
        unsigned m_bankId;
        std::vector<unsigned> m_bankIdVec;
	e_BankCommandType m_cmdMnemonic;
	std::map<e_BankCommandType, std::string> m_cmdToString;
	bool m_isResponseReady;
        c_HashedAddress m_hashedAddr;

public:

	//    friend std::ostream& operator<< (std::ostream& x_stream, const c_BankCommand& x_bankCommand);

	explicit c_BankCommand(unsigned x_seqNum, e_BankCommandType x_cmdType,
			       ulong x_addr);
        c_BankCommand(unsigned x_seqNum, e_BankCommandType x_cmdType,
		      ulong x_addr, unsigned x_bankId); // only to be used for Refresh commands!
        c_BankCommand(unsigned x_seqNum, e_BankCommandType x_cmdType,
		      ulong x_addr, std::vector<unsigned> &x_bankIdVec); // only to be used for Refresh commands!
        c_BankCommand(unsigned x_seqNum, e_BankCommandType x_cmdType,
		      ulong x_addr, c_HashedAddress &x_hashedAddr); 
        c_BankCommand() {} // required for ImplementSerializable

	c_BankCommand(c_BankCommand&) = delete;
	c_BankCommand(c_BankCommand&&) = delete;
	c_BankCommand& operator=(c_BankCommand) = delete;

	void print() const;

        const c_HashedAddress *getHashedAddress() const {
	  return (&m_hashedAddr);
	}

        inline unsigned getBankId() const {
	  return (m_bankId);
	}

        inline std::vector<unsigned> *getBankIdVec() {
	  return(&m_bankIdVec);
	}
  
	inline bool isResponseReady() const
	{
		return (m_isResponseReady);
	}

	inline void setResponseReady()
	{
		m_isResponseReady = true;
	}

	inline unsigned getSeqNum() const
	{
		return (m_seqNum);
	}

	e_BankCommandType getCommandMnemonic() const;

	ulong getAddress() const; //<! returns the address accessed by this command
	std::string getCommandString() const;//<! returns the mnemonic of command

	// FIXME: implement operator<<
	//    friend inline std::ostream& operator<< (
	//        std::ostream&        x_stream,
	//        const c_BankCommand& x_bankCommand
	//    )
	//    {
	//        x_stream<<"[CMD: "<<x_bankCommand.getCommandString()<<", SEQNUM: "<<std::dec<<x_bankCommand.getSeqNum()<<" , ADDR: "<<std::hex<<x_bankCommand.getAddress()<<" , isResponseReady: "<<std::boolalpha<<x_bankCommand.isResponseReady()<<"]";
	//        return x_stream;
	//    }

        void serialize_order(SST::Core::Serialization::serializer &ser) override ;
  
        ImplementSerializable(c_BankCommand);

}; // class c_BankCommand

} // namespace n_Bank

} // namespace SST
#endif // C_BANKCOMMAND_HPP
