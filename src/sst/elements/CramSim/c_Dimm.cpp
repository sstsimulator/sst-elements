// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
 * c_Dimm.cpp
 *
 *  Created on: June 27, 2016
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

//SST includes
#include "sst_config.h"

// C++ includes
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include <sst/core/stringize.h>

// CramSim includes
#include "c_Dimm.hpp"
#include "c_CmdReqEvent.hpp"
#include "c_CmdResEvent.hpp"

#include <cmath>

using namespace SST;
using namespace SST::CramSim;

c_Dimm::c_Dimm(SST::ComponentId_t x_id, SST::Params& x_params) :
		Component(x_id) {


	m_simCycle=0;

	// / configure links
	// DIMM <-> Controller Links
	//// DIMM <-> CmdUnit (Req) (Cmd)
	m_ctrlLink = configureLink("ctrlLink",
			new Event::Handler<c_Dimm>(this,
					&c_Dimm::handleInCmdUnitReqPtrEvent));

        output = new Output("", 1, 0, SST::Output::STDOUT);

	// read params here
	bool l_found = false;

	k_numChannels = (uint32_t)x_params.find<uint32_t>("numChannels", 1,
															 l_found);
	if (!l_found) {
		output->fatal(CALL_INFO, -1, "[c_Dimm] numChannelsPerDimm value is missing... exiting\n");
	}

	k_numPChannelsPerChannel = (uint32_t)x_params.find<uint32_t>("numPChannelsPerChannel", 1,
															l_found);
	if (!l_found) {
		output->output("[c_Dimm] numPChannelsPerChannel value is missing... disabled\n");
	}

	k_numRanksPerChannel = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 100,
			l_found);
	if (!l_found) {
		output->fatal(CALL_INFO, -1, "[c_Dimm] numRanksPerChannel value is missing... exiting\n");
	}

	k_numBankGroupsPerRank = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank", 100,
			l_found);
	if (!l_found) {
		output->fatal(CALL_INFO, -1, "[c_Dimm] numBankGroupsPerRank value is missing... exiting\n");
	}

	k_numBanksPerBankGroup = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup", 100,
			l_found);
	if (!l_found) {
		output->fatal(CALL_INFO, -1, "[c_Dimm] numBanksPerBankGroup value is missing... exiting\n");
	}

	k_boolPowerCalc = (bool) x_params.find<bool> ("boolPowerCalc", false, l_found);
	if(!l_found){
		output->output("[c_Dimm] boolPowerCalc value is missing... disabled\n");
	}


	m_numRanks = k_numChannels * k_numPChannelsPerChannel * k_numRanksPerChannel;
	m_numBanks = m_numRanks* k_numBankGroupsPerRank * k_numBanksPerBankGroup;

	if(k_boolPowerCalc)
	{
		int l_numRank=k_numRanksPerChannel*k_numChannels*k_numPChannelsPerChannel;
		m_actpreEnergy.resize(m_numRanks);
		m_refreshEnergy.resize(m_numRanks);
		m_readEnergy.resize(m_numRanks);
		m_writeEnergy.resize(m_numRanks);
		m_refreshEnergy.resize(m_numRanks);
		m_backgroundEnergy.resize(m_numRanks);

		k_IDD0 = (uint32_t)x_params.find<uint32_t>("IDD0", 0, l_found);
		if (!l_found) {
			output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD0 value is missing... exiting\n");
		}

		k_IDD2N = (uint32_t)x_params.find<uint32_t>("IDD2N", 0, l_found);
		if (!l_found) {
                        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD2N value is missing... exiting\n");
		}

		k_IDD2P = (uint32_t)x_params.find<uint32_t>("IDD2P", 0, l_found);
		if (!l_found) {
                        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD2P value is missing... exiting\n");
		}

		k_IDD3N = (uint32_t)x_params.find<uint32_t>("IDD3N", 0, l_found);
		if (!l_found) {
		        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD3N value is missing... exiting\n");
		}

		k_IDD4R = (uint32_t)x_params.find<uint32_t>("IDD4R", 0, l_found);
		if (!l_found) {
		        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD4R value is missing... exiting\n");
		}
		k_IDD4W = (uint32_t)x_params.find<uint32_t>("IDD4W", 0, l_found);
		if (!l_found) {
		        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD4W value is missing... exiting\n");
		}

		k_IDD5 = (uint32_t)x_params.find<uint32_t>("IDD5", 0, l_found);
		if (!l_found) {
		    output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but IDD5 value is missing... exiting\n");
		}

		k_nRAS = (uint32_t) x_params.find<uint32_t>("nRAS", 39, l_found);
		if (!l_found) {
			output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but nRAS value is missing ... exiting\n");
		}

		k_nRP = (uint32_t) x_params.find<uint32_t>("nRP", 16, l_found);
		if (!l_found) {
			output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but nRP value is missing ... exiting\n");
		}

		k_nRFC = (uint32_t) x_params.find<uint32_t>("nRFC", 420, l_found);
		if (!l_found) {
			output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but nRFC value is missing ... exiting\n");
		}

		k_nBL = (uint32_t) x_params.find<uint32_t>("nBL", 4, l_found);
		if (!l_found) {
                        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but nBL value is missing ... exiting\n");
		}

		k_VDD = (float) x_params.find<float>("VDD", 4, l_found);
		if (!l_found) {
			output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but VDD value is missing ... exiting\n");
		}

		k_numDevices = (uint32_t) x_params.find<uint32_t>("numDevices", 4, l_found);
		if (!l_found) {
                        output->fatal(CALL_INFO, -1, "[c_Dimm] Power calculation is enabled, but numDevices value is missing ... exiting\n");
		}

	}

	Statistic<uint64_t> *l_totalRowHits = registerStatistic<uint64_t>("totalRowHits");
	for (int l_i = 0; l_i != m_numBanks; ++l_i) {
		c_Bank* l_entry = new c_Bank(x_params,l_i);
		m_banks.push_back(l_entry);

                std::string l_bankNum = std::to_string(l_entry->getBankNum());
		c_BankStatistics *l_bankStats = new c_BankStatistics();

                l_bankStats->s_bankACTsRecvd = registerStatistic<uint64_t>("bankACTsRecvd", l_bankNum);
                l_bankStats->s_bankREADsRecvd = registerStatistic<uint64_t>("bankREADsRecvd", l_bankNum);
                l_bankStats->s_bankWRITEsRecvd = registerStatistic<uint64_t>("bankWRITEsRecvd", l_bankNum);
                l_bankStats->s_bankPREsRecvd = registerStatistic<uint64_t>("bankPREsRecvd", l_bankNum);
                l_bankStats->s_bankRowHits = registerStatistic<uint64_t>("bankRowHits", l_bankNum);

		l_bankStats->s_totalRowHits = l_totalRowHits;

		l_entry->acceptStatistics(l_bankStats);
		m_bankStatsVec.push_back(l_bankStats);
	}

	// get configured clock frequency
	std::string l_clockFreqStr = (std::string)x_params.find<std::string>("strControllerClockFrequency", "1GHz", l_found);

	//set our clock
	m_clockHandler=new Clock::Handler<c_Dimm>(this, &c_Dimm::clockTic);
	registerClock(l_clockFreqStr, m_clockHandler);

	// Statistics setup
	s_actCmdsRecvd     = registerStatistic<uint64_t>("actCmdsRecvd");
	s_readCmdsRecvd    = registerStatistic<uint64_t>("readCmdsRecvd");
	s_readACmdsRecvd   = registerStatistic<uint64_t>("readACmdsRecvd");
	s_writeCmdsRecvd   = registerStatistic<uint64_t>("writeCmdsRecvd");
	s_writeACmdsRecvd  = registerStatistic<uint64_t>("writeACmdsRecvd");
	s_preCmdsRecvd     = registerStatistic<uint64_t>("preCmdsRecvd");
	s_refCmdsRecvd     = registerStatistic<uint64_t>("refCmdsRecvd");

	m_actCmdsRecvd.resize(m_numRanks);
	m_readCmdsRecvd.resize(m_numRanks);
	m_readACmdsRecvd.resize(m_numRanks);
	m_writeCmdsRecvd.resize(m_numRanks);
	m_writeACmdsRecvd.resize(m_numRanks);
	m_preCmdsRecvd.resize(m_numRanks);
	m_refCmdsRecvd.resize(m_numRanks);

        for(unsigned i=0; i<m_numRanks;i++)
        {
            m_actCmdsRecvd[i]=0;
            m_readCmdsRecvd[i]=0;
            m_readACmdsRecvd[i]=0;
            m_writeCmdsRecvd[i]=0;
            m_writeACmdsRecvd[i]=0;
            m_preCmdsRecvd[i]=0;
            m_refCmdsRecvd[i]=0;
        }
}

c_Dimm::~c_Dimm() {


}

c_Dimm::c_Dimm() :
		Component(-1) {
	// for serialization only
}

void c_Dimm::printQueues() {
        std::stringstream str;
	str << __PRETTY_FUNCTION__ << std::endl;
	str << "m_cmdResQ.size() = " << m_cmdResQ.size() << std::endl;
	output->output("%s", str.str().c_str());
        for (auto& l_cmdPtr : m_cmdResQ)
		(l_cmdPtr)->print(m_simCycle);
}

bool c_Dimm::clockTic(SST::Cycle_t) {
	m_simCycle++;
	for (int l_i = 0; l_i != m_banks.size(); ++l_i) {

		c_BankCommand* l_resPtr = m_banks.at(l_i)->clockTic();
		if (nullptr != l_resPtr) {
			m_cmdResQ.push_back(l_resPtr);
		}
	}

	sendResponse();

	if(k_boolPowerCalc)
		updateBackgroundEnergy();

	return false;
}

void c_Dimm::handleInCmdUnitReqPtrEvent(SST::Event *ev) {

	c_CmdReqEvent* l_cmdReqEventPtr = dynamic_cast<c_CmdReqEvent*>(ev);
	if (l_cmdReqEventPtr) {

		c_BankCommand* l_cmdReq = l_cmdReqEventPtr->m_payload;
		unsigned l_rank=l_cmdReq->getHashedAddress()->getRankId();
		assert(l_rank<m_numRanks);

		switch(l_cmdReq->getCommandMnemonic()) {
		case e_BankCommandType::ACT:
		  s_actCmdsRecvd->addData(1);
				m_actCmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::READ:
		  s_readCmdsRecvd->addData(1);
				m_readCmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::READA:
		  s_readACmdsRecvd->addData(1);
				m_readACmdsRecvd[l_rank]+=1;
				break;
		case e_BankCommandType::WRITE:
		  s_writeCmdsRecvd->addData(1);
				m_writeCmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::WRITEA:
		  s_writeACmdsRecvd->addData(1);
				m_writeACmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::PRE:
		  s_preCmdsRecvd->addData(1);
				m_preCmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::REF:
		  s_refCmdsRecvd->addData(1);
				m_refCmdsRecvd[l_rank]+=1;
		  break;
		case e_BankCommandType::ERR:
		case e_BankCommandType::PREA:
		case e_BankCommandType::PDX:
		case e_BankCommandType::PDE:
		  break;
		}

		sendToBank(l_cmdReq);

		if(k_boolPowerCalc)
			updateDynamicEnergy(l_cmdReq);

		delete l_cmdReqEventPtr;
	} else {
                std::stringstream str;
		str<< __PRETTY_FUNCTION__ << "ERROR:: Bad event type!"
                    << std::endl;
                output->output("%s", str.str().c_str());
	}
}

void c_Dimm::updateDynamicEnergy(c_BankCommand* x_bankCommandPtr)
{
	double_t l_energy=0;
	int l_rank = x_bankCommandPtr->getHashedAddress()->getRankId();
	assert(l_rank<m_numRanks);

	switch(x_bankCommandPtr->getCommandMnemonic()) {
	    case e_BankCommandType::ACT:
			l_energy = ((k_IDD0 * (k_nRAS+k_nRP))-((k_IDD3N * k_nRAS) + (k_IDD2N * (k_nRP)))) * k_VDD * k_numDevices; //active and precharge
			m_actpreEnergy[l_rank]+=l_energy;
			break;
		case e_BankCommandType::READ:
		case e_BankCommandType::READA:
			l_energy = ((k_IDD4R-k_IDD3N) * k_nBL) * k_VDD * k_numDevices;
			m_readEnergy[l_rank]+=l_energy;
			break;
		case e_BankCommandType::WRITE:
		case e_BankCommandType::WRITEA:
			l_energy = ((k_IDD4W-k_IDD3N) * k_nBL) * k_VDD * k_numDevices;
			m_writeEnergy[l_rank]+=l_energy;
			break;
		case e_BankCommandType::PRE:
			break;
		case e_BankCommandType::REF:
			//Todo: per-bank refresh
			l_energy = ((k_IDD5-k_IDD3N) * k_nRFC) * k_VDD * k_numDevices;
			m_refreshEnergy[l_rank]+=l_energy;
			break;
		case e_BankCommandType::ERR:
		case e_BankCommandType::PREA:
		case e_BankCommandType::PDX:
		case e_BankCommandType::PDE:
		  break;
	}
}

void c_Dimm::updateBackgroundEnergy()
{
	//Todo: update background energy depeding on bank status

	for(unsigned i=0;i<m_numRanks;i++)
	{
		m_backgroundEnergy[i]+= k_IDD3N * k_VDD * k_numDevices;
	}
}
void c_Dimm::sendToBank(c_BankCommand* x_bankCommandPtr) {
	unsigned l_bankNum = 0;
	if (x_bankCommandPtr->getBankIdVec().size() > 0) {
		for (auto &l_bankid:x_bankCommandPtr->getBankIdVec()) {
			c_BankCommand *l_cmd = new c_BankCommand(x_bankCommandPtr->getSeqNum(), x_bankCommandPtr->getCommandMnemonic(),
													 x_bankCommandPtr->getAddress(), l_bankid);
			l_cmd->setResponseReady();
			m_banks.at(l_bankid)->handleCommand(l_cmd);
		}
		delete x_bankCommandPtr;
	} else {
		l_bankNum = x_bankCommandPtr->getBankId();
		m_banks.at(l_bankNum)->handleCommand(x_bankCommandPtr);
	}
}


void c_Dimm::sendResponse() {

	// check if ResQ has cmds
	while (!m_cmdResQ.empty()) {

	  c_CmdResEvent* l_cmdResEventPtr = new c_CmdResEvent();
	  l_cmdResEventPtr->m_payload = m_cmdResQ.front();
	  m_cmdResQ.erase(
			  std::remove(m_cmdResQ.begin(), m_cmdResQ.end(),
				      m_cmdResQ.front()), m_cmdResQ.end());
		m_ctrlLink->send(l_cmdResEventPtr);
	}
}

void c_Dimm::finish(){

	double l_actprePower=0;
	double l_readPower =0;
	double l_writePower =0;
	double l_refreshPower=0;
	double l_backgroundPower=0;

	uint64_t l_actRecvd=0;
	uint64_t l_readRecvd=0;
	uint64_t l_writeRecvd=0;
	uint64_t l_refreshRecvd=0;
	uint64_t l_prechRecvd=0;
	uint64_t l_totalRecvd=0;

	output->output("Deleting DIMM\n");
	output->output("======= CramSim Simulation Report [Memory Device] ===================================\n");

	for(unsigned i=0; i<m_numRanks;i++)
	{
		l_actRecvd+=m_actCmdsRecvd[i];
		l_readRecvd+=(m_readCmdsRecvd[i]+m_readACmdsRecvd[i]);
		l_writeRecvd+=(m_writeCmdsRecvd[i]+m_writeACmdsRecvd[i]);
		l_prechRecvd+=(m_preCmdsRecvd[i]);
		l_refreshRecvd+=(m_refCmdsRecvd[i]);
	}
	l_totalRecvd=l_actRecvd+l_readRecvd+l_writeRecvd+l_prechRecvd+l_refreshRecvd;

    output->output(" 1. Received Commands\n");
	output->output("  - Total : %" PRIu64 "\n", l_totalRecvd);
	output->output("  - Active : %" PRIu64 "(%.2f%%)\n",
			  l_actRecvd,
			  (double)l_actRecvd/(double)l_totalRecvd*100);
	output->output("  - Read : %" PRIu64 "(%.2f%%)\n",
			  l_readRecvd,
              (double)l_readRecvd/(double)l_totalRecvd*100);
	output->output("  - Write : %" PRIu64 "(%.2f%%\n",
			  l_writeRecvd,
              (double)l_writeRecvd/(double)l_totalRecvd*100);
	output->output("  - Precharge : %" PRIu64 "(%.2f%%)\n",
			  l_prechRecvd,
              (double)l_prechRecvd/(double)l_totalRecvd*100);
	output->output("  - Refresh : %" PRIu64 " (%.2f%%)\n",
			  l_refreshRecvd,
              (double)l_refreshRecvd/(double)l_totalRecvd*100);

    if(k_boolPowerCalc)
    {
        for(unsigned i=0;i<m_numRanks;i++) {
            l_actprePower+= m_actpreEnergy[i]/m_simCycle;
            l_readPower+=m_readEnergy[i]/m_simCycle;
            l_writePower+=m_writeEnergy[i]/m_simCycle;
            l_refreshPower+=m_refreshEnergy[i]/m_simCycle;
            l_backgroundPower+=m_backgroundEnergy[i]/m_simCycle;
        }
        double l_totalPower=l_actprePower+l_readPower+l_writePower+l_refreshPower+l_backgroundPower;
        output->output("\n");
        output->output(" 2. Power Consumption\n");
        output->output("  - Total Power (mW) : %.2f\n", l_totalPower);
        output->output("  - Active/Precharge Power (mW) : %.2f(%.2f%%)\n",
                  l_actprePower,
                  l_actprePower/l_totalPower*100);
        output->output("  - Read Power (mW) : %.2f(%.2f%%)\n",
                  l_readPower,
                  l_readPower/l_totalPower*100);
        output->output("  - Write Power (mW) : %.2f(%.2f%%)\n",
                  l_writePower,
                  l_writePower/l_totalPower*100);
        output->output("  - Refresh Power (mW) : %.2f(%.2f%%)\n",
                  l_refreshPower,
                  l_refreshPower/l_totalPower*100);
        output->output("  - Background Power (mW) : %.2f(%.2f%%)\n",
                  l_backgroundPower,
                  l_backgroundPower/l_totalPower*100);
	}
	output->output("=====================================================================================\n");
}


