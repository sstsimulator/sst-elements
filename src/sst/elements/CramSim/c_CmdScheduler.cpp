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

#include "c_CmdScheduler.hpp"
#include "c_CmdUnit.hpp"

using namespace SST;
using namespace SST::n_Bank;

c_CmdScheduler::c_CmdScheduler(Component *comp, Params &x_params) : c_CtrlSubComponent <c_BankCommand*,c_BankCommand*> (comp, x_params){
    m_nextSubComponent=dynamic_cast<c_Controller*>(comp)->getDeviceController();
}


bool c_CmdScheduler::clockTic(SST::Cycle_t){
    run();
    send();
}


void c_CmdScheduler::run(){
    //bypassing
    if(m_outputQ.empty()) {
        while (!m_inputQ.empty()) {
            m_outputQ.push_back(m_inputQ.front());
            m_inputQ.pop_front();
        }
    }
}

void c_CmdScheduler::send() {
    int token=m_nextSubComponent->getToken();

    while(token>0 && !m_outputQ.empty()) {
        m_nextSubComponent->push(m_outputQ.front());
        m_outputQ.pop_front();
        token--;
    }
}




