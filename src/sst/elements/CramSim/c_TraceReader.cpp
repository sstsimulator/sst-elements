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

//SST includes
#include "sst_config.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdlib.h>

#include <sst/core/stringize.h>

//local includes
//#include "c_TxnReqEvent.hpp"
//#include "c_TxnResEvent.hpp"
#include "c_TraceReader.hpp"

//using namespace std;
using namespace SST;
using namespace n_Bank;

c_TraceReader::c_TraceReader(SST::ComponentId_t x_id, SST::Params& x_params):c_TxnGen(x_id,x_params)
{
    // trace file param
    bool l_found=false;
    m_traceFileName = x_params.find<std::string>("traceFile", "nil", l_found);
    if (!l_found)
    {
        std::cout << "TxnGen:: traceFile name is missing... exiting"<<
        std::endl;
        exit(-1);
    }

    std::cout<<m_traceFileName<<std::endl;
    m_traceFileStream = new std::ifstream(m_traceFileName, std::ifstream::in);
    if(!(*m_traceFileStream))
    {
        std::cerr << "Unable to open trace file " << m_traceFileName << " Aborting!" <<
        std::endl;
        exit(-1);
    }
}


void c_TraceReader::createTxn()
{
// check if txn can fit inside Req q
    while(m_txnReqQ.size()<k_numTxnPerCycle)
    {
        std::string l_line;
        if (std::getline(*m_traceFileStream, l_line)) {
            char_delimiter sep(" ");
            Tokenizer<> l_tok(l_line, sep);
            unsigned l_numTokens = std::distance(l_tok.begin(), l_tok.end());
            unsigned l_tokNum = 0;
            unsigned l_txnInterval = 0;
            e_TransactionType l_txnType;
            ulong l_txnAddress = 0;
            unsigned l_txnDataWidth = 0;

            for (Tokenizer<>::iterator l_iter =
                    l_tok.begin(); l_iter != l_tok.end(); ++l_iter) {
                switch (l_tokNum) {
                    case 0:
                        l_txnAddress = (ulong) strtoul((*l_iter).c_str(), NULL,
                                                       0);
                        break;
                    case 1:
                        if ((*l_iter).find("WR") != std::string::npos)
                            l_txnType = e_TransactionType::WRITE;
                        else
                            l_txnType = e_TransactionType::WRITE;
                        break;
                    case 2:
                        l_txnInterval = std::atoi((*l_iter).c_str());
                        break;
                    default:
                        std::cout
                                << "TxnGen: Should not be in this stage of switch statment"
                                << std::endl;
                        exit(-1);
                        break;
                }
                ++l_tokNum;
            }


            c_Transaction* l_txn = new c_Transaction(m_seqNum, l_txnType, l_txnAddress, 1);
	        m_seqNum++;
            std::pair<c_Transaction *, unsigned> l_entry = std::make_pair(l_txn, l_txnInterval);
            m_txnReqQ.push_back(l_entry);

            //todo: record transaction
             std::cout << std::endl << std::endl
             		<< "TxnGen::createTxn() created txn seqNum = " << m_seqNum
             		<< " addr=" << l_txnAddress << std::endl;
        } else {
            break;
            //  std::cout << "TxnGen:: Ran out of txn's to read" << std::endl;
        }
    }
}
