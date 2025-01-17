// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _DUMMY_PE_H
#define _DUMMY_PE_H

#include "pes/processingElement.h"

namespace SST {
namespace Llyr {

/**
 * @todo write docs
 */
class DummyProcessingElement : public ProcessingElement
{
public:
    DummyProcessingElement(opType op_binding, uint32_t processor_id, LlyrConfig* llyr_config) :
                    ProcessingElement(op_binding, processor_id, llyr_config)
    {
    }

    virtual bool doSend() { return 0; };
    virtual bool doReceive(LlyrData data) { return 0; };
    virtual bool doCompute() { return 0; };

    //TODO for testing only
    virtual void inputQueueInit() {};
    virtual void outputQueueInit() {};

};

}//SST
}//Llyr

#endif // _DUMMY_PE_H
