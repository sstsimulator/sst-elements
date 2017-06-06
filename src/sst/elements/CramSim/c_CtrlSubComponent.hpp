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

#ifndef C_CMDSCHEDULER_H
#define C_CMDSCHEDULER_H

#include <queue>
#include <fstream>
// SST includes

#include <sst/core/subcomponent.h>
#include <sst/core/component.h>
#include <sst/core/link.h>

// local includes
#include "c_BankCommand.hpp"

namespace SST {
    namespace n_Bank {
        class c_CmdUnit;

        template<class I, class O>
        class c_CtrlSubComponent : public SST::SubComponent {
        public:
            c_CtrlSubComponent(Component *comp, Params &x_params);

            ~c_CtrlSubComponent() {};

            void finish() {};

            void print() {}; // print internals

            //interfaces
            void push(I input);

            int getToken();

            virtual bool clockTic(SST::Cycle_t) =0;



        protected:
            //internal functions
            virtual void run() =0 ;
            virtual void send() = 0;

            // internal buffer
            std::vector<I> m_input2Q;                    //input queue
            std::deque<I> m_inputQ;                    //input queue
            std::deque<O> m_outputQ;

            // pointers of neighbor components
            std::deque<O> *m_nextSubCompInputQ;    //neighbor subcomponent

            // params for internal architecture
            int k_numCtrlIntQEntries;

            // params for bank structure
            int k_numBytesPerTransaction;
            int k_numChannelsPerDimm;
            int k_numPseudoChannels;
            int k_numRanksPerChannel;
            int k_numBankGroupsPerRank;
            int k_numBanksPerBankGroup;
            int k_numColsPerBank;
            int k_numRowsPerBank;
            bool k_allocateCmdResQACT;
            bool k_allocateCmdResQREAD;
            bool k_allocateCmdResQREADA;
            bool k_allocateCmdResQWRITE;
            bool k_allocateCmdResQWRITEA;
            bool k_allocateCmdResQPRE;
            bool k_useRefresh;
            bool k_cmdQueueFindAnyIssuable;
            int k_bankPolicy;
            int k_relCommandWidth;
            int m_cmdResQTokens;
            int k_useDualCommandBus;
            int k_multiCycleACT;
            bool k_printCmdTrace;
            std::string k_cmdTraceFileName;
            std::filebuf m_cmdTraceFileBuf;
            std::streambuf *m_cmdTraceStreamBuf;
            std::ofstream m_cmdTraceOFStream;
            std::ostream *m_cmdTraceStream;
            std::map<std::string, unsigned> m_bankParams;
            int m_numChannelsPerDimm;
            int m_numRanksPerChannel;
            int m_numBankGroupsPerRank;
            int m_numBanksPerBankGroup;
            int m_numBanks;
            int m_numRanks;
            int m_numBankGroups;



        };

        template<class I, class O>
        c_CtrlSubComponent<I, O>::c_CtrlSubComponent(Component *owner, Params &x_params) : SubComponent(owner) {
            m_inputQ.clear();
            m_outputQ.clear();
            bool l_found = false;

            k_numBytesPerTransaction = (uint32_t) x_params.find<uint32_t>("numBytesPerTransaction", 32, l_found);
            if (!l_found) {
                std::cout << "numBytesPerTransaction value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numChannelsPerDimm = (uint32_t) x_params.find<uint32_t>("numChannelsPerDimm", 1, l_found);
            if (!l_found) {
                std::cout << "numChannelsPerDimm value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numPseudoChannels = (uint32_t) x_params.find<uint32_t>("numPseudoChannels", 1, l_found);
            if (!l_found) {
                std::cout << "numPseudoChannel value is missing... " << std::endl;
                //exit(-1);
            }

            k_numRanksPerChannel = (uint32_t) x_params.find<uint32_t>("numRanksPerChannel", 2, l_found);
            if (!l_found) {
                std::cout << "numRanksPerChannel value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numBankGroupsPerRank = (uint32_t) x_params.find<uint32_t>("numBankGroupsPerRank", 100, l_found);
            if (!l_found) {
                std::cout << "numBankGroupsPerRank value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numBanksPerBankGroup = (uint32_t) x_params.find<uint32_t>("numBanksPerBankGroup", 100, l_found);
            if (!l_found) {
                std::cout << "numBanksPerBankGroup value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numColsPerBank = (uint32_t) x_params.find<uint32_t>("numColsPerBank", 100, l_found);
            if (!l_found) {
                std::cout << "numColsPerBank value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_numRowsPerBank = (uint32_t) x_params.find<uint32_t>("numRowsPerBank", 100, l_found);
            if (!l_found) {
                std::cout << "numRowsPerBank value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_relCommandWidth = (uint32_t) x_params.find<uint32_t>("relCommandWidth", 100, l_found);
            if (!l_found) {
                std::cout << "relCommandWidth value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_useRefresh = (uint32_t) x_params.find<uint32_t>("boolUseRefresh", 1, l_found);
            if (!l_found) {
                std::cout << "boolUseRefresh param value is missing... exiting" << std::endl;
                exit(-1);
            }


            /* BUFFER ALLOCATION PARAMETERS */
            k_allocateCmdResQACT = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResACT", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResACT value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_allocateCmdResQREAD = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResREAD", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResREAD value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_allocateCmdResQREADA = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResREADA", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResREADA value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_allocateCmdResQWRITE = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResWRITE", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResWRITE value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_allocateCmdResQWRITEA = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResWRITEA", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResWRITEA value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_allocateCmdResQPRE = (uint32_t) x_params.find<uint32_t>("boolAllocateCmdResPRE", 1, l_found);
            if (!l_found) {
                std::cout << "boolAllocateCmdResPRE value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_cmdQueueFindAnyIssuable = (uint32_t) x_params.find<uint32_t>("boolCmdQueueFindAnyIssuable", 1, l_found);
            if (!l_found) {
                std::cout << "boolCmdQueueFindAnyIssuable value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_bankPolicy = (uint32_t) x_params.find<uint32_t>("bankPolicy", 0, l_found);
            if (!l_found) {
                std::cout << "bankPolicy value is missing... exiting" << std::endl;
                exit(-1);
            }

            k_useDualCommandBus = (uint32_t) x_params.find<uint32_t>("boolDualCommandBus", 0, l_found);
            if (!l_found) {
                std::cout << "boolDualCommandBus value is missing... disabled" << std::endl;
            }

            k_multiCycleACT = (uint32_t) x_params.find<uint32_t>("boolMultiCycleACT", 0, l_found);
            if (!l_found) {
                std::cout << "boolMultiCycleACT value is missing... disabled" << std::endl;
            }



            // calculate total number of banks
            m_numChannelsPerDimm = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,
                                                                         l_found);
            if (!l_found) {
                std::cout << "numChannelsPerDimm value is missing... exiting"
                          << std::endl;
                exit(-1);
            }

            m_numRanksPerChannel = (uint32_t)x_params.find<uint32_t>("numRanksPerChannel", 100,
                                                                         l_found);
            if (!l_found) {
                std::cout << "numRanksPerChannel value is missing... exiting"
                          << std::endl;
                exit(-1);
            }

            m_numBankGroupsPerRank = (uint32_t)x_params.find<uint32_t>("numBankGroupsPerRank",
                                                                           100, l_found);
            if (!l_found) {
                std::cout << "numBankGroupsPerRank value is missing... exiting"
                          << std::endl;
                exit(-1);
            }

            m_numBanksPerBankGroup = (uint32_t)x_params.find<uint32_t>("numBanksPerBankGroup",
                                                                           100, l_found);
            if (!l_found) {
                std::cout << "numBanksPerBankGroup value is missing... exiting"
                          << std::endl;
                exit(-1);
            }

           // m_numBanks = m_numRanksPerChannel * m_numBankGroupsPerRank
             //            * m_numBanksPerBankGroup;

            /* BANK TRANSITION PARAMETERS */
            //FIXME: Move this param reading to inside of c_BankInfo
            m_bankParams["nRC"] = (uint32_t) x_params.find<uint32_t>("nRC", 55, l_found);
            if (!l_found) {
                std::cout << "nRC value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRRD"] = (uint32_t) x_params.find<uint32_t>("nRRD", 4, l_found);
            if (!l_found) {
                std::cout << "nRRD value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRRD_L"] = (uint32_t) x_params.find<uint32_t>("nRRD_L", 6, l_found);
            if (!l_found) {
                std::cout << "nRRD_L value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRRD_S"] = (uint32_t) x_params.find<uint32_t>("nRRD_S", 4, l_found);
            if (!l_found) {
                std::cout << "nRRD_S value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRCD"] = (uint32_t) x_params.find<uint32_t>("nRCD", 16, l_found);
            if (!l_found) {
                std::cout << "nRRD_L value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCCD"] = (uint32_t) x_params.find<uint32_t>("nCCD", 4, l_found);
            if (!l_found) {
                std::cout << "nCCD value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCCD_L"] = (uint32_t) x_params.find<uint32_t>("nCCD_L", 5, l_found);
            if (!l_found) {
                std::cout << "nCCD_L value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCCD_L_WR"] = (uint32_t) x_params.find<uint32_t>("nCCD_L_WR", 1, l_found);
            if (!l_found) {
                std::cout << "nCCD_L_WR value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCCD_S"] = (uint32_t) x_params.find<uint32_t>("nCCD_S", 4, l_found);
            if (!l_found) {
                std::cout << "nCCD_S value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nAL"] = (uint32_t) x_params.find<uint32_t>("nAL", 15, l_found);
            if (!l_found) {
                std::cout << "nAL value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCL"] = (uint32_t) x_params.find<uint32_t>("nCL", 16, l_found);
            if (!l_found) {
                std::cout << "nCL value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nCWL"] = (uint32_t) x_params.find<uint32_t>("nCWL", 16, l_found);
            if (!l_found) {
                std::cout << "nCWL value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nWR"] = (uint32_t) x_params.find<uint32_t>("nWR", 16, l_found);
            if (!l_found) {
                std::cout << "nWR value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nWTR"] = (uint32_t) x_params.find<uint32_t>("nWTR", 3, l_found);
            if (!l_found) {
                std::cout << "nWTR value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nWTR_L"] = (uint32_t) x_params.find<uint32_t>("nWTR_L", 9, l_found);
            if (!l_found) {
                std::cout << "nWTR_L value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nWTR_S"] = (uint32_t) x_params.find<uint32_t>("nWTR_S", 3, l_found);
            if (!l_found) {
                std::cout << "nWTR_S value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRTW"] = (uint32_t) x_params.find<uint32_t>("nRTW", 4, l_found);
            if (!l_found) {
                std::cout << "nRTW value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nEWTR"] = (uint32_t) x_params.find<uint32_t>("nEWTR", 6, l_found);
            if (!l_found) {
                std::cout << "nEWTR value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nERTW"] = (uint32_t) x_params.find<uint32_t>("nERTW", 6, l_found);
            if (!l_found) {
                std::cout << "nERTW value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nEWTW"] = (uint32_t) x_params.find<uint32_t>("nEWTW", 6, l_found);
            if (!l_found) {
                std::cout << "nEWTW value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nERTR"] = (uint32_t) x_params.find<uint32_t>("nERTR", 6, l_found);
            if (!l_found) {
                std::cout << "nERTR value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRAS"] = (uint32_t) x_params.find<uint32_t>("nRAS", 39, l_found);
            if (!l_found) {
                std::cout << "nRAS value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRTP"] = (uint32_t) x_params.find<uint32_t>("nRTP", 9, l_found);
            if (!l_found) {
                std::cout << "nRTP value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRP"] = (uint32_t) x_params.find<uint32_t>("nRP", 16, l_found);
            if (!l_found) {
                std::cout << "nRP value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nRFC"] = (uint32_t) x_params.find<uint32_t>("nRFC", 420, l_found);
            if (!l_found) {
                std::cout << "nRFC value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nREFI"] = (uint32_t) x_params.find<uint32_t>("nREFI", 9360, l_found);
            if (!l_found) {
                std::cout << "nREFI value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nFAW"] = (uint32_t) x_params.find<uint32_t>("nFAW", 16, l_found);
            if (!l_found) {
                std::cout << "nFAW value is missing ... exiting" << std::endl;
                exit(-1);
            }

            m_bankParams["nBL"] = (uint32_t) x_params.find<uint32_t>("nBL", 4, l_found);
            if (!l_found) {
                std::cout << "nBL value is missing ... exiting" << std::endl;
                exit(-1);
            }

            k_numCtrlIntQEntries = (uint32_t) x_params.find<uint32_t>("numCtrlIntQEntries", 100, l_found);
            if (!l_found) {
                std::cout << "k_numCtrlIntQEntries value is missing... exiting" << std::endl;
                exit(-1);
            }
        }


        template<class I, class O>
        void c_CtrlSubComponent<I, O>::push(I input) {
            m_inputQ.push_back(input);
            //std::cout<<"m_cmdReqQ.size():"<<m_cmdReqQ.size()<<std::endl;
        }

        template<class I, class O>
        int c_CtrlSubComponent<I, O>::getToken() {
            int l_QueueSize = m_inputQ.size();
            assert(k_numCtrlIntQEntries >= l_QueueSize);

            return k_numCtrlIntQEntries - l_QueueSize;
        }
    }
}
#endif //SRC_C_CMDSCHEDULER_H
