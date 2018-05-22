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

#ifndef COMPONENTS_FIREFLY_NICTESTER_H
#define COMPONENTS_FIREFLY_NICTESTER_H

#include <sst/core/sst_types.h>
#include <sst/core/component.h>

#include <sst/core/output.h>

class DmaEntry;

namespace SST {
namespace Firefly {

#define FOREACH_STATE(NAME) \
    NAME(PostRecv)\
    NAME(PostSend)\
    NAME(WaitSend)\
    NAME(WaitRecv)\

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,


class VirtNic;

class NicTester : public SST::Component {

  public:
    NicTester( SST::ComponentId_t id, SST::Params &params );
    ~NicTester() {};
    void setup( void );
    void init( unsigned int phase );
    void printStatus( Output& ) {};
    void handleSelfEvent( SST::Event* );

  private:
    enum {
        FOREACH_STATE(GENERATE_ENUM)
    } m_state;

    static const char* m_stateName[];
    bool notifySendPioDone( void* );
    bool notifySendDmaDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    void postRecv();
    void postSend();
    void waitSend();

    SST::Link*  m_selfLink;
    SST::Link*  m_hostLink;
    VirtNic*    m_vNic;
    Output      m_dbg;
};


}
}
#endif
