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

//local includes
#include "c_Transaction.hpp"

namespace SST {
namespace n_Bank {

enum class e_BankCommandType {
	ERR, ACT, READ, READA, WRITE, WRITEA, PRE, PREA, REF, PDX, PDE
};

class c_BankCommand {

private:

	unsigned m_seqNum;
	unsigned m_addr;
	unsigned m_row;
	e_BankCommandType m_cmdMnemonic;
	std::map<e_BankCommandType, std::string> m_cmdToString;
	bool m_isResponseReady;
	c_Transaction* m_transactionPtr; //<! ptr to the c_Transaction that this c_BankCommand is part of

public:

	//    friend std::ostream& operator<< (std::ostream& x_stream, const c_BankCommand& x_bankCommand);

	explicit c_BankCommand(unsigned x_seqNum, e_BankCommandType x_cmdType,
			unsigned x_addr);

	c_BankCommand(c_BankCommand&) = delete;
	c_BankCommand(c_BankCommand&&) = delete;
	c_BankCommand& operator=(c_BankCommand) = delete;

	void print() const;

	inline void setRow(unsigned x_row) {
		m_row = x_row;
	}

	inline unsigned getRow() const {
		return (m_row);
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

	unsigned getAddress() const; //<! returns the address accessed by this command
	std::string getCommandString() const;//<! returns the mnemonic of command

	void acceptTransaction(c_Transaction* x_transaction);

	c_Transaction* getTransaction() const;

	// FIXME: implement operator<<
	//    friend inline std::ostream& operator<< (
	//        std::ostream&        x_stream,
	//        const c_BankCommand& x_bankCommand
	//    )
	//    {
	//        x_stream<<"[CMD: "<<x_bankCommand.getCommandString()<<", SEQNUM: "<<std::dec<<x_bankCommand.getSeqNum()<<" , ADDR: "<<std::hex<<x_bankCommand.getAddress()<<" , isResponseReady: "<<std::boolalpha<<x_bankCommand.isResponseReady()<<"]";
	//        return x_stream;
	//    }

};}
}
#endif // C_BANKCOMMAND_HPP
