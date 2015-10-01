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

#ifndef  _IRISTERMINAL_H_INC
#define  _IRISTERMINAL_H_INC

#include	"genericHeader.h"

namespace SST {
namespace Iris {

/*
 * =====================================================================================
 *        Class:  IrisTerminal
 *  Description:  abstract class for any terminal interfacing to iris
 *  Eg: pkt generator; core; cache; memory controller
 * =====================================================================================
 */
class IrisTerminal
{
    public:
        IrisTerminal (): terminal_recv(false){}                             /* constructor */

        bool terminal_recv;
        DES_Link* interface_link;
        virtual void handle_link_arrival(DES_Event* ev, int port_id);

}; /* -----  end of class IrisTerminal  ----- */

}
}

#endif   /* ----- #ifndef _IRISTERMINAL_H_INC  ----- */
