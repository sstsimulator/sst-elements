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

/*
 * c_BankReceiver.cpp
 *
 *  Created on: June 22, 2016
 *      Author: mcohen
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

// C++ includes
#include <string>
#include <assert.h>

// CramSim includes
#include "c_BankReceiver.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"

using namespace SST;
using namespace SST::n_BankReceiver;
using namespace SST::n_Bank;

c_BankReceiver::c_BankReceiver(SST::ComponentId_t x_id, SST::Params& x_params) :
	Component(x_id) {

	// tell the simulator not to end without us
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();

	// configure links

	// BankReceiver <-> CmdUnit Links
	//// BankReceiver <-> CmdUnit (Req) (Cmd)
	m_inCmdUnitReqPtrLink = configureLink(
			"inCmdUnitReqPtr",
			new Event::Handler<c_BankReceiver>(this,
					&c_BankReceiver::handleInCmdUnitReqPtrEvent));
	//// BankReceiver <-> CmdUnit (Res) (Cmd)
	m_outCmdUnitResPtrLink = configureLink(
			"outCmdUnitResPtr",
			new Event::Handler<c_BankReceiver>(this,
					&c_BankReceiver::handleOutCmdUnitResPtrEvent));

	//set our clock
	registerClock("1GHz",
			new Clock::Handler<c_BankReceiver>(this, &c_BankReceiver::clockTic));
}

c_BankReceiver::~c_BankReceiver() {

}

c_BankReceiver::c_BankReceiver() :
	Component(-1) {
	// for serialization only
}

bool c_BankReceiver::clockTic(SST::Cycle_t) {
	std::cout << std::endl << std::endl << "BankReceiver:: clock tic"
			<< std::endl;

	sendResponse();

	return false;
}

void c_BankReceiver::sendResponse() {
	if (m_cmdQ.size() > 0) {
		std::list<c_BankCommand*>::iterator l_iter = m_cmdQ.begin();
		while (!(*l_iter)->isResponseReady())
			l_iter++;

		if (l_iter != m_cmdQ.end()) {
			std::cout << std::endl << "@" << std::dec
					<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
					<< __PRETTY_FUNCTION__ << std::endl;
			(*l_iter)->print();
			std::cout << std::endl;

			SST::n_Bank::c_CmdResEvent* l_cmdResEventPtr = new c_CmdResEvent();
			l_cmdResEventPtr->m_payload = *l_iter;
			m_cmdQ.remove(*l_iter);
			m_outCmdUnitResPtrLink->send(l_cmdResEventPtr);

		} else {
			std::cout << __PRETTY_FUNCTION__ << ": No Cmd's are response ready yet" << std::endl;
		}

	}
}

void c_BankReceiver::handleInCmdUnitReqPtrEvent(SST::Event *ev) {

	c_CmdReqEvent* l_cmdReqEventPtr = dynamic_cast<c_CmdReqEvent*> (ev);
	if (l_cmdReqEventPtr) {
		c_BankCommand* l_cmdReq = l_cmdReqEventPtr->m_payload;

		std::cout << std::endl << "@" << std::dec
				<< Simulation::getSimulation()->getCurrentSimCycle() << ": "
				<< __PRETTY_FUNCTION__ << std::endl;
		l_cmdReq->print();
		std::cout << std::endl;

		m_cmdQ.push_back(l_cmdReq);

		delete l_cmdReqEventPtr;
	} else {
		std::cout << __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
				<< std::endl;
	}
}

void c_BankReceiver::handleOutCmdUnitResPtrEvent(SST::Event *ev) {
	// nothing to do here
	std::cout << __PRETTY_FUNCTION__ << " ERROR: Should not be here"
			<< std::endl;
}

