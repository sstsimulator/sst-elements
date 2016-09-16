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



#ifndef C_TRANSACTION_HPP
#define C_TRANSACTION_HPP

#include <ostream>
#include <iostream>
#include <map>
#include <string>
#include <list>
#include <memory>


namespace SST {
namespace n_Bank {


class c_BankCommand;

enum class e_TransactionType { READ, WRITE };

class c_Transaction
{

private:
  unsigned m_seqNum;
  e_TransactionType m_txnMnemonic;
  unsigned m_addr;
  std::map<e_TransactionType,std::string> m_txnToString;

  bool m_isResponseReady;
  unsigned m_numWaitingCommands;
  unsigned m_dataWidth;
  bool m_processed; //<! flag that is set when this transaction is split into commands

  std::list<c_BankCommand*> m_cmdPtrList; //<! list of c_BankCommand shared_ptrs that compose this c_Transaction

public:

//  friend std::ostream& operator<< (std::ostream& x_stream, const c_Transaction& x_transaction);

  c_Transaction( unsigned x_seqNum, e_TransactionType x_cmdType , unsigned x_addr , unsigned x_dataWidth );
  ~c_Transaction();

  e_TransactionType getTransactionMnemonic() const;

  unsigned getAddress() const;         //<! returns the address accessed by this command
  std::string getTransactionString() const; //<! returns the mnemonic of command

  void setResponseReady(); //<! sets the flag that this transaction has received its response.
  bool isResponseReady();  //<! sets the flag that this transaction has received its response.

  void setWaitingCommands(const unsigned x_numWaitingCommands);
  unsigned getWaitingCommands() const;

  void addCommandPtr(c_BankCommand* x_cmdPtr);
  unsigned getDataWidth() const;
  unsigned getThreadId() const; // FIXME
  bool isProcessed() const;
  void isProcessed(bool x_processed);
  void print() const;
};

} // namespace n_Bank
} // namespace SST

#endif // C_TRANSACTION_HPP
