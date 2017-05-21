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
#ifndef C_BANKRECEIVER_HPP
#define C_BANKRECEIVER_HPP

#include <list>

// SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_BankCommand.hpp"

namespace SST {
namespace n_BankReceiver {
class c_BankReceiver: public SST::Component {
public:

	c_BankReceiver(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_BankReceiver();

private:
	c_BankReceiver(); // for serialization only
	c_BankReceiver(const c_BankReceiver&); // do not implement
	void operator=(const c_BankReceiver&); // do not implement

	virtual bool clockTic(SST::Cycle_t); // called every cycle

	// BankReceiver <-> CmdUnit Handlers
	void handleInCmdUnitReqPtrEvent(SST::Event *ev); // receive a cmd req from CmdUnit
	void handleOutCmdUnitResPtrEvent(SST::Event *ev); // we do not need this function for functionality

	// BankReceiver <-> CmdUnit Links
	SST::Link* m_inCmdUnitReqPtrLink; // incoming cmdunit req ptr
	SST::Link* m_outCmdUnitResPtrLink; // outgoing cmdunit res ptr

	void sendResponse();

	std::list<SST::n_Bank::c_BankCommand*> m_cmdQ;

};
}
}

#endif // C_BANKRECEIVER_HPP
