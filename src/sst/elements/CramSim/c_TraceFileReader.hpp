
// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef C_TRACEFILEREADER_HPP
#define C_TRACEFILEREADER_HPP

#include <stdint.h>
#include <queue>
#include <iostream>
#include <string>
#include <fstream>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

//local includes
#include "c_Transaction.hpp"
#include "c_TxnGen.hpp"


namespace SST {
    namespace n_Bank {
        class c_TraceFileReader: public c_TxnGenBase {
        public:
            c_TraceFileReader(SST::ComponentId_t x_id, SST::Params& x_params);
            ~c_TraceFileReader(){}
        private:
            enum e_TracefileType{
                DEFAULT,   //DRAMsim2 type
                USIMM
            };
            virtual void createTxn();

            //params for internal microarcitecture
            std::string m_traceFileName;
            std::ifstream *m_traceFileStream;

            e_TracefileType m_traceType;
        };
    }
}

#endif //SRC_C_TRACEFILEREADER_HPP
