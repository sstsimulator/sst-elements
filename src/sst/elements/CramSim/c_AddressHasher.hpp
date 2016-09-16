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
#ifndef c_ADDRESSHASHER_HPP
#define c_ADDRESSHASHER_HPP

#include <memory>
#include <map>

// local includes
#include "c_BankCommand.hpp"

//<! This class holds information about global simulation state
//<! any object in the simulator can access this class

class c_AddressHasher {

public:
	static c_AddressHasher* getInstance();

	unsigned getBankFromAddress(const unsigned x_address,
			const unsigned x_numBanks);
	unsigned getBankFromAddress1(const unsigned x_address,
			const unsigned x_numBanks);
	unsigned getBankFromAddress2(const unsigned x_address,
			const unsigned x_numBytesPerTransaction,
			const unsigned x_numChannels, const unsigned x_numBanks);
	unsigned getRowFromAddress(const unsigned x_address,
			const unsigned x_numBytesPerTransaction, const unsigned x_numRows,
			const unsigned x_numCols, const unsigned x_numChannels,
			const unsigned x_numBanks);
private:
	static c_AddressHasher* m_instance; //<! shared_ptr to instance of this class

	c_AddressHasher(const c_AddressHasher&)=delete;
	void operator=(const c_AddressHasher&)=delete;
	void construct();

};

#endif // c_ADDRESSHASHER_HPP
