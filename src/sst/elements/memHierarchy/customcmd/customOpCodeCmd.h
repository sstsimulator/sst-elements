// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_CUSTOMOPCODECMD_H_
#define _MEMHIERARCHY_CUSTOMOPCODECMD_H_

#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

/* CustomCmdInfo with OpCode */
class CustomOpCodeCmdInfo : public CustomCmdInfo {
public:
    /* Constructors */
    CustomOpCodeCmdInfo() : CustomCmdInfo() { }

    CustomOpCodeCmdInfo(SST::Event::id_type id, std::string rqstr, Addr addr) :
        CustomCmdInfo(id, rqstr), addr_(addr), custOpc_(0xFFFF) { }

    CustomOpCodeCmdInfo(SST::Event::id_type id, std::string rqstr, Addr addr, uint32_t flags = 0) :
        CustomCmdInfo(id, rqstr, flags), addr_(addr), custOpc_(0xFFFF) { }

    CustomOpCodeCmdInfo(SST::Event::id_type id, std::string rqstr, Addr addr, uint32_t opc, uint32_t flags = 0) :
        CustomCmdInfo(id, rqstr, flags), addr_(addr), custOpc_(opc) { }

    virtual ~CustomOpCodeCmdInfo() = default;

    /* String-ify for debug */
    virtual std::string getString() {
        std::ostringstream str;
        str << std::hex << " Addr: 0x" << addr_;
        str << " OpCode: 0x" << custOpc_;
        return CustomCmdInfo::getString() + str.str();
    }
    
    /* Address getter/setter */
    void setAddr(Addr addr) { addr_ = addr; }
    Addr getAddr() { return addr_; }

    /* Op code getter/setter */
    void setOpCode(uint32_t opc) { custOpc_ = opc; }
    uint32_t getOpCode() { return custOpc_; }

protected:
    Addr addr_;          /* Target address */
    uint32_t custOpc_;   /* Custom op code */

};


} /* namespace MemHierarchy */
} /* namespace SST */
#endif
