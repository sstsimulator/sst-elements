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
    MerlinIO(Component*, Params&);

    virtual void _componentInit(unsigned int phase );

    virtual IO::NodeId getNodeId() { return m_myNodeId; }

    virtual IO::NodeId peek();
    virtual bool sendv(IO::NodeId dest, std::vector<IoVec>&,
                                                        IO::Entry::Functor*);
    virtual bool recvv(IO::NodeId src, std::vector<IoVec>&,
                                                        IO::Entry::Functor*);

    virtual void wait();
    virtual void setReturnLink(SST::Link* link) { m_leaveLink = link; }

    struct Entry { 
        Entry( IO::NodeId _node, std::vector<IoVec> _ioVec, 
                                    IO::Entry::Functor* _functor ) : 
            node( _node ),
            ioVec( _ioVec ),
            functor( _functor ),
            currentVec(0),
            currentPos(0) 
        { } 

        IO::NodeId              node;
        std::vector<IoVec>      ioVec;
        IO::Entry::Functor*     functor; 
        size_t                  currentVec;    
        size_t                  currentPos;
    };

  private:

    class SelfEvent : public SST::Event {
      public:
        SelfEvent( Entry* e) : Event(), entry(e) {}
        Entry* entry;
    };

    void selfHandler( Event* );
    void enter();
    bool sendNotify(int); 
    bool recvNotify(int); 

    bool processRecv( );
    bool processRecvEvent( Event* );
    bool processSend();
    void drainInput();

    std::deque<Event*>      m_recvEventQ; 
    SST::Link*              m_leaveLink;
    SST::Link*              m_selfLink;

    std::map< IO::NodeId, Entry* > m_recvEntryM;
    Entry*                  m_sendEntry;
    
    IO::NodeId              m_myNodeId;
    Output                  m_dbg;
    Merlin::LinkControl*    m_linkControl;

    int                     m_numVC;
    int                     m_lastVC;
    bool                    m_haveCtrl;

    Merlin::LinkControl::Handler<MerlinIO> * m_recvNotifyFunctor;
    Merlin::LinkControl::Handler<MerlinIO> * m_sendNotifyFunctor;
};

}
}

#endif
