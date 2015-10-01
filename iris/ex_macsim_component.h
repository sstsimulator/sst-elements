/*
 * =====================================================================================
// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
 * =====================================================================================
 */

#include	"genericHeader.h"
#include        "irisTerminal.h"

#define MEM_NOC_DONE 1

namespace SST {
namespace Iris {

struct mem_req_s
{
    int m_addr;
    int m_noc_type;
    int m_state;
    uint64_t m_msg_dst;
};

typedef struct mem_req_s mem_req_s;

class macsim_c: public IrisTerminal
{
    public:
        std::string name;
        DES_Link* interface_link;

};

}
}

