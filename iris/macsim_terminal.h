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

#ifndef  _MACSIMTERMINAL_H_INC
#define  _MACSIMTERMINAL_H_INC

#include "genericHeader.h"
#include "SST_interface.h"

#include <vector>
#include <queue>

namespace SST {
namespace Iris {

class irisNPkt;
class macsim_c;
struct mem_req_s;

// class to include within macsim that creates the rtrEvent 
// for sending to the network
class MacsimTerminal
{
    public:
        MacsimTerminal(){}
        virtual ~MacsimTerminal(){};

        MacsimTerminal(macsim_c* simBase, int v )
        { owner=simBase; vcs = v;}
        void init(void)
        {
            interface_credits.resize(vcs);
        }

        bool send_packet(mem_req_s* macsim_req);

        mem_req_s* check_queue( void );
        void handle_interface_link_arrival (DES_Event* e, int port_id);


        macsim_c* owner;

        // for the interface
        
        // terminal->interface->router belong to the same 
        // node assign node ids accordingly
        int64_t node_id;        
        int vcs;
        std::vector<bool> interface_credits;
        std::queue <mem_req_s*> recvQ;

    private:
};

}
}

#endif   /* ----- #ifndef _MACSIMTERMINAL_H_INC  ----- */
