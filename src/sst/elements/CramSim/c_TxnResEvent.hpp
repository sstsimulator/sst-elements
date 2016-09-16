/*
 * c_TxnResEvent.hpp
 *
 *  Created on: May 18, 2016
 *      Author: tkarkha
 */

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

#ifndef C_TXNRESEVENT_HPP_
#define C_TXNRESEVENT_HPP_

#include "c_Transaction.hpp"

namespace SST {
namespace n_Bank {
class c_TxnResEvent: public SST::Event {
public:
	c_Transaction *m_payload; // FIXME: change this pointer to a unique_ptr

	c_TxnResEvent() :
			SST::Event() {
	}

	void serialize_order(SST::Core::Serialization::serializer &ser) {
		Event::serialize_order(ser);
//        ser & m_payload;
	}

	ImplementSerializable (SST::n_Bank::c_TxnResEvent);

};

}
}

#endif /* C_TXNRESEVENT_HPP_ */
