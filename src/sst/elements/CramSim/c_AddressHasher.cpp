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

#include "c_AddressHasher.hpp"

using namespace std;

c_AddressHasher* c_AddressHasher::m_instance = nullptr;

c_AddressHasher* c_AddressHasher::getInstance() {
	if (nullptr == m_instance) {
		m_instance->construct();
	}
	return m_instance;
}

void c_AddressHasher::construct() {

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

//	std::cout << "l_result = " << std::hex << l_result << std::endl;

	l_result = l_result % x_numRowsPerBank; // FIXME: this could be made faster by eliminating the mod operation

//	std::cout << "x_numRowsPerBank = " << std::dec << x_numRowsPerBank << std::endl;
//	std::cout << "l_result = " << std::hex << l_result << std::endl;

	return (l_result);
}
