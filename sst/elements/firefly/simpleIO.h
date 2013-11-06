// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_FIREFLY_TESTIO_H
#define COMPONENTS_FIREFLY_TESTIO_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>

#include "ioapi.h"
#include "ioEvent.h"

namespace SST {
namespace Firefly {

class SimpleIO : public IO::Interface {

  public:
    SimpleIO(Params&);

     virtual void _componentInit(unsigned int phase );

    virtual IO::NodeId getNodeId() { return m_myNodeId; }

    virtual size_t peek(IO::NodeId& src);
    virtual bool sendv(IO::NodeId dest, std::vector<IoVec>&,
                                                        IO::Entry::Functor*);
    virtual bool recvv(IO::NodeId src, std::vector<IoVec>&,
                                                        IO::Entry::Functor*);
    bool _sendv(IO::NodeId dest, std::vector<IoVec>&, IO::Entry::Functor*);
    bool _recvv(IO::NodeId src, std::vector<IoVec>&, IO::Entry::Functor*);

  private:
    void handleEvent(SST::Event*);
    void handleSelfLink(SST::Event*);

    std::map<IO::NodeId, std::string > m_streamMap;

    IO::Functor2*           m_dataReadyFunc;
    SST::Link*              m_link;
    IO::NodeId              m_myNodeId;
    SST::Link*              m_selfLink;
    Output                  m_dbg;
};

}
}

#endif
