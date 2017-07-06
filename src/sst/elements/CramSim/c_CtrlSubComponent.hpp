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

using namespace std;
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

        protected:
            enum DEBUG_MASK{
                TXNCVT = 1,
                CMDSCH = 1<<1,
                ADDRHASH= 1<<2,
                DVCCTRL=1<<3
            };
            void debug(unsigned mask_bit, unsigned debug_level, const char* format, ...);
            void debug(const char* prefix, unsigned mask_bit, unsigned debug_level, const char* format, ...);
            unsigned parseDebugFlags(std::string debugFlags);
            bool isDebugEnabled(DEBUG_MASK x_debug_mask);


            //internal functions
            virtual void run() =0 ;
            virtual void send(){};

            // internal buffer
            std::deque<I> m_inputQ;                    //input queue
            std::deque<O> m_outputQ;

            // params for internal architecture
            int k_numCtrlIntQEntries;

            // debug output
            Output*         m_debugOutput;
            unsigned        m_debug_bits;
            bool m_debug_en;
            std::vector<std::string>m_debugFlags;


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
            int k_REFI;
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

            int m_numChannels;
            int m_numPseudoChannels;
            int m_numRanks;
            int m_numBankGroups;
            int m_numBanks;

        };

        template<class I, class O>
        c_CtrlSubComponent<I, O>::c_CtrlSubComponent(Component *owner, Params &x_params) : SubComponent(owner) {

            //set debug output
            unsigned l_debug_level = x_params.find<uint32_t>("debug_level", 0);
            std::string l_debug_flag = x_params.find<std::string>("debug_flag","");
            m_debug_bits = parseDebugFlags(l_debug_flag);
            m_debugOutput = new SST::Output("",
                                            l_debug_level,
                                            m_debug_bits,
                                            (Output::output_location_t)x_params.find<int>("debug_location", 1),
                                            x_params.find<std::string>("debug_file","debugLog"));

            if(l_debug_level>0 && m_debug_bits!=0)
                m_debug_en=true;
            else
                m_debug_en=false;

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

            k_REFI = (uint32_t) x_params.find<uint32_t>("nREFI", 1, l_found);
            if (!l_found) {
                std::cout << "nREFI value is missing ... exiting" << std::endl;
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
         /*   m_numChannelsPerDimm = (uint32_t)x_params.find<uint32_t>("numChannelsPerDimm", 1,
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
            }*/

            // configure the memory hierarchy
            m_numChannels =  k_numChannelsPerDimm;
            m_numPseudoChannels = m_numChannels * k_numPseudoChannels;
            m_numRanks = m_numPseudoChannels * k_numRanksPerChannel;
            m_numBankGroups = m_numRanks * k_numBankGroupsPerRank;
            m_numBanks = m_numBankGroups * k_numBanksPerBankGroup;

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

        template<class I, class O>
        void c_CtrlSubComponent<I, O>::debug(unsigned mask_bit, unsigned debug_level, const char* format, ...)
        {
           // m_debugOutput->verbosePrefix(prefix.c_str(),CALL_INFO,3,mask_bit,msg.c_str());
            if(m_debug_en==true) {
                va_list args;
                va_start(args,format);
                size_t size = std::snprintf(nullptr, 0, format, args)+ 1;
                std::unique_ptr<char[]> buf(new char[size]);
                std::vsnprintf(buf.get(),size, format, args);
                std::string msg = std::string(buf.get(),buf.get()+size -1);

                m_debugOutput->verbose(CALL_INFO, debug_level, mask_bit, msg.c_str());
                m_debugOutput->flush();
                va_end(args);
            }
        }

        template<class I, class O>
        void c_CtrlSubComponent<I, O>::debug(const char* prefix, unsigned mask_bit, unsigned debug_level, const char* format, ...) {
            // m_debugOutput->verbosePrefix(prefix.c_str(),CALL_INFO,3,mask_bit,msg.c_str());
            if (m_debug_en == true) {
                va_list args;
                va_start(args, format);
                size_t size = std::snprintf(nullptr, 0, format, args) + 1;
                std::unique_ptr<char[]> buf(new char[size]);
                std::vsnprintf(buf.get(), size, format, args);
                std::string msg = std::string(buf.get(), buf.get() + size - 1);

                m_debugOutput->verbosePrefix(prefix, CALL_INFO, debug_level, mask_bit, msg.c_str());
                m_debugOutput->flush();
                va_end(args);
            }
        }



            template<class I, class O>
        unsigned c_CtrlSubComponent<I, O>::parseDebugFlags(std::string x_debugFlags)
        {

            unsigned debug_bits=0;
            std::string delimiter = ",";
            size_t pos=0;
            std::string token;
			if(x_debugFlags!="")
			{
				while((pos=x_debugFlags.find(delimiter)) != std::string::npos){
					token = x_debugFlags.substr(0,pos);
					m_debugFlags.push_back(token);
					x_debugFlags.erase(0,pos+delimiter.length());
				}
				m_debugFlags.push_back(x_debugFlags);

				for(auto &it : m_debugFlags)
				{
					std::cout <<it<<std::endl;
					if(it=="dvcctrl")
							debug_bits|=DVCCTRL;
					else if(it=="txncvt")
							debug_bits|=TXNCVT;
					else if(it=="cmdsch")
							debug_bits|=CMDSCH;
					else if(it=="addrhash")
						debug_bits|=ADDRHASH;
					else {
						printf("debug flag error! (dvcctrl/txncvt/cmdsch/addrhash)\n");
						exit(1);
					}
				}
				return debug_bits;
			}
			return 0;
        }

        template<class I, class O>
        bool c_CtrlSubComponent<I, O>::isDebugEnabled(DEBUG_MASK x_debug_mask)
        {
            if(m_debug_bits & x_debug_mask)
                return true;
            else
                return false;
        };
    }
}
#endif //SRC_C_CMDSCHEDULER_H
