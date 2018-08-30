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

#include "sst_config.h"

#include <sst/core/params.h>
#include <sst/core/link.h>

#include "nicTester.h"
#include "ioVec.h"
#include "virtNic.h"

using namespace SST;
using namespace SST::Firefly;

const char* NicTester::m_stateName[] = {
    FOREACH_STATE(GENERATE_STRING)
};

struct Hdr {
    int pid;
};

#define BODY_SIZE 1000 

struct DmaEntry {
    Hdr hdr;
    unsigned char body[BODY_SIZE];
};

typedef DmaEntry PioEntry;

#define SHORT_MSG   0xdeadbee0

NicTester::NicTester(ComponentId_t id, Params &params) :
    Component( id ),
    m_state( PostSend )
{
    // this has to come first 
    registerTimeBase( "100 ns", true);

    m_dbg.init("@t:NicTester::@p():@l " + getName() + ": ",
            params.find_integer("verboseLevel",0),0,
            (Output::output_location_t)params.find_integer("debug", 0));

    params.print_all_params( std::cout );

    std::string name = params.find_string("nicModule");
    assert( ! name.empty() );

    m_dbg.debug(CALL_INFO,1,0,"nicModule `%s`\n",name.c_str()); 

    Params nicParams = params.find_prefix_params("nicParams." );

    m_vNic = dynamic_cast<VirtNic*>(
                loadModuleWithComponent( name, this, nicParams ));
    assert( m_vNic );


    char buffer[100];
    snprintf(buffer,100,"@t:%d:NicTester::@p():@l ", m_vNic->getVirtNicId() );
    m_dbg.setPrefix(buffer);


    m_vNic->setNotifyOnSendPioDone( 
        new VirtNic::Handler<NicTester,void*>(this, &NicTester::notifySendPioDone )
    );
    m_vNic->setNotifyOnSendDmaDone( 
        new VirtNic::Handler<NicTester,void*>(this, &NicTester::notifySendDmaDone )
    );

    m_vNic->setNotifyOnRecvDmaDone( 
        new VirtNic::Handler4Args<NicTester,int,int,size_t,void*>(
                                this, &NicTester::notifyRecvDmaDone )
    );
    m_vNic->setNotifyNeedRecv( 
        new VirtNic::Handler3Args<NicTester,int,int,size_t>(
                    this, &NicTester::notifyNeedRecv)
    );

    m_selfLink = configureSelfLink("NicTester::selfLink", "1 ps",
        new Event::Handler<NicTester>(this,&NicTester::handleSelfEvent));
    assert( m_selfLink );

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

void NicTester::init( unsigned int phase )
{
    m_vNic->init( phase );
}

void NicTester::setup()
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    m_selfLink->send(0,NULL);
}

bool NicTester::notifySendDmaDone( void* key )
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    handleSelfEvent(NULL);
    return true;
}

bool NicTester::notifyRecvDmaDone( int src, int tag, size_t len, void* key )
{
    DmaEntry* entry = static_cast<DmaEntry*>(key);
    m_dbg.debug(CALL_INFO,1,0,"src=%d tag=%#x len=%lu pid=%d\n",src, 
            tag, len, entry->hdr.pid);

    for ( unsigned int i = 0; i < len - sizeof(entry->hdr); i++ ) {
        if ( entry->body[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,entry->body[i]);
        }
    }
    primaryComponentOKToEndSim();
    return true;
}

bool NicTester::notifySendPioDone( void* key)
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    handleSelfEvent(NULL);
    return true;
}

bool NicTester::notifyNeedRecv( int nid, int tag, size_t len )
{
    m_dbg.debug(CALL_INFO,1,0,"nid=%d tag=%d len=%lu\n",nid,tag,len);
    postRecv();
    return true;
}

void NicTester::handleSelfEvent( SST::Event* e )
{
    m_dbg.debug(CALL_INFO,1,0,"%s\n",m_stateName[m_state]);

    switch( m_state ) {
      case PostRecv:
        postRecv();
        break;
      case PostSend:
        postSend();
        break;
      case WaitSend:
        waitSend();
        break;
      case WaitRecv:
        break;
    }
}

void NicTester::postRecv()
{
    m_dbg.debug(CALL_INFO,1,0,"\n");

    DmaEntry* entry = new DmaEntry;
    std::vector<IoVec> iovec(2);

    iovec[0].ptr = &entry->hdr; 
    iovec[0].len = sizeof(entry->hdr);
    iovec[1].ptr = &entry->body; 
    iovec[1].len = sizeof(entry->body);
    m_vNic->dmaRecv( -1, SHORT_MSG, iovec, entry );

    m_state = PostSend; 

    m_selfLink->send(0,NULL);
}

void NicTester::postSend()
{
    m_dbg.debug(CALL_INFO,1,0,"\n");

    PioEntry* entry = new PioEntry;
    std::vector<IoVec> iovec(2);

    iovec[0].ptr = &entry->hdr; 
    iovec[0].len = sizeof(entry->hdr);
    iovec[1].ptr = &entry->body; 
    iovec[1].len = 10;//sizeof(entry->body);

    entry->hdr.pid = 0xf00d;
    for ( unsigned int i = 0; i < BODY_SIZE; i++ ) {
        entry->body[i] = i;
    }

    m_vNic->pioSend( (m_vNic->getVirtNicId() + 1 ) % 2, SHORT_MSG, iovec, entry );

    m_state = WaitSend; 

    m_selfLink->send(0,NULL);
}

void NicTester::waitSend()
{
    m_dbg.debug(CALL_INFO,1,0,"\n");
    m_state = WaitRecv;
}
