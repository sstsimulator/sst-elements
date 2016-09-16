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
#ifndef C_TRANSACTIONTOCOMMANDS_HPP
#define C_TRANSACTIONTOCOMMANDS_HPP

#include <memory>
#include <queue>

#include "c_Transaction.hpp"
#include "c_BankCommand.hpp"

//<! This class converts a transaction to sequence of bank commands
//<! This class will accept the width of each
//<! command access and then divide the transaction size by the width
//<! to determine the number of basic dram commands.

namespace SST {
namespace n_Bank {

class c_TransactionToCommands {

public:
	static c_TransactionToCommands* getInstance();
	// x_relCommandWidth is the width of commands in terms of transaction
	// for example, if transaction and command are for the same width, x_relCommandWidth
	// must be set to 1, to create one basic command for every transaction.
	std::vector<c_BankCommand*> getCommands(c_Transaction* x_txn,
			unsigned x_relCommandWidth, bool x_useReadA, bool x_useWriteA);
	std::queue<c_BankCommand*> getRefreshCommands(unsigned x_numBanks);


private:
	static c_TransactionToCommands* m_instance; //<! shared_ptr to instance of this class
	static unsigned m_cmdSeqNum;

	c_TransactionToCommands(const c_TransactionToCommands&) =delete;
	void operator=(const c_TransactionToCommands&) =delete;
	void construct();

	std::vector<c_BankCommand*> getReadCommands(c_Transaction* x_txn, unsigned x_relCommandWidth, bool x_useReadA);
	std::vector<c_BankCommand*> getWriteCommands(c_Transaction* x_txn, unsigned x_relCommandWidth, bool x_useWriteA);
};

} // namespace n_Bank
} // namespace SST

#endif // C_TRANSACTIONTOCOMMAND_HPP
