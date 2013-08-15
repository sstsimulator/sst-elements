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


#ifndef COMPONENTS_FIREFLY_MERLINIO_H
#define COMPONENTS_FIREFLY_MERLINIO_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>

#include "sst/elements/merlin/linkControl.h"

#include "ioapi.h"
#include "ioEvent.h"

namespace SST {
namespace Firefly {

class MerlinIO : public IO::Interface {

  public:
    MerlinIO(Params&);

    static const int FlitSize = 8;

     virtual void _componentInit(unsigned int phase );

    virtual IO::NodeId getNodeId() { return m_myNodeId; }
    virtual void setDataReadyFunc(IO::Functor2*);

    virtual size_t peek(IO::NodeId& src);
    virtual bool isReady(IO::NodeId src);
    virtual bool sendv(IO::NodeId dest, std::vector<IO::IoVec>&,
                                                        IO::Entry::Functor*);
    virtual bool recvv(IO::NodeId src, std::vector<IO::IoVec>&,
                                                        IO::Entry::Functor*);

    struct Entry { 
        Entry( IO::NodeId _node, std::vector<IO::IoVec> _ioVec, 
                                    IO::Entry::Functor* _functor ) : 
            node( _node ),
            ioVec( _ioVec ),
            functor( _functor ),
            currentVec(0),
            currentPos(0) 
        { 
        } 

        IO::NodeId              node;
        std::vector<IO::IoVec>  ioVec;
        IO::Entry::Functor*     functor; 
        size_t                  currentVec;    
        size_t                  currentPos;
    };
  private:

    bool clockHandler( Cycle_t cycle );

    struct IN {
        IN() : nbytes(0) {}
        std::deque<Event*>  queue;
        size_t              nbytes;
    };
    std::map<IO::NodeId, IN >       m_eventMap;
    

    IO::Functor2*           m_dataReadyFunc;
    IO::NodeId              m_myNodeId;
    Output                  m_dbg;
    Merlin::LinkControl*    m_linkControl;

    std::deque<Entry*>      m_recvQ;
    std::deque<Entry*>      m_sendQ;
    int                     m_numVC;
    int                     m_lastVC;
};

}
}

#endif
