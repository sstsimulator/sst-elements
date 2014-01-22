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

#include "sst_config.h"

#include <sst/core/debug.h>
#include <sst/core/params.h>

#include "nicTester.h"

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

    m_dbg.verbose(CALL_INFO,1,0,"nicModule `%s`\n",name.c_str()); 

    Params nicParams = params.find_prefix_params("nicParams." );

    m_nic = dynamic_cast<Nic*>(
                loadModuleWithComponent( name, this, nicParams ));
    assert( m_nic );

    char buffer[100];
    snprintf(buffer,100,"@t:%d:NicTester::@p():@l ", m_nic->getNodeId() );
    m_dbg.setPrefix(buffer);


    m_nic->setNotifyOnSendDmaDone( 
        new Nic::Handler<NicTester,Nic::XXX*>(this, &NicTester::notifySendDmaDone )
    );
    m_nic->setNotifyOnRecvDmaDone( 
        new Nic::Handler<NicTester,Nic::XXX*>(this, &NicTester::notifyRecvDmaDone )
    );
    m_nic->setNotifyOnSendPioDone( 
        new Nic::Handler<NicTester,Nic::XXX*>(this, &NicTester::notifySendPioDone )
    );
    m_nic->setNotifyNeedRecv( 
        new Nic::Handler<NicTester,Nic::XXX*>(this, &NicTester::notifyNeedRecv)
    );

    m_selfLink = configureSelfLink("NicTester::selfLink", "1 ps",
        new Event::Handler<NicTester>(this,&NicTester::handleSelfEvent));
    assert( m_selfLink );

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}

void NicTester::init( unsigned int phase )
{
    m_nic->init( phase );
}

void NicTester::setup()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_selfLink->send(0,NULL);
}

bool NicTester::notifySendDmaDone( Nic::XXX* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    handleSelfEvent(NULL);
    return true;
}

bool NicTester::notifyRecvDmaDone( Nic::XXX* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    waitRecv(key);
    return true;
}

bool NicTester::notifySendPioDone( Nic::XXX* key)
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    handleSelfEvent(NULL);
    return true;
}

bool NicTester::notifyNeedRecv( Nic::XXX* key)
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    postRecv();
    return true;
}

void NicTester::handleSelfEvent( SST::Event* e )
{
    m_dbg.verbose(CALL_INFO,1,0,"%s\n",m_stateName[m_state]);

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
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    Nic::XXX* xxx = new Nic::XXX; 
    DmaEntry* entry = new DmaEntry;
    xxx->usrData = entry; 
    std::vector<IoVec> iovec(2);

    iovec[0].ptr = &entry->hdr; 
    iovec[0].len = sizeof(entry->hdr);
    iovec[1].ptr = &entry->body; 
    iovec[1].len = sizeof(entry->body);
    m_nic->dmaRecv( -1, SHORT_MSG, iovec, xxx );

    m_state = PostSend; 

    m_selfLink->send(0,NULL);
}

void NicTester::postSend()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    Nic::XXX* xxx = new Nic::XXX; 
    PioEntry* entry = new PioEntry;
    xxx->usrData = entry;
    std::vector<IoVec> iovec(2);

    iovec[0].ptr = &entry->hdr; 
    iovec[0].len = sizeof(entry->hdr);
    iovec[1].ptr = &entry->body; 
    iovec[1].len = 10;//sizeof(entry->body);

    entry->hdr.pid = 0xf00d;
    for ( unsigned int i = 0; i < BODY_SIZE; i++ ) {
        entry->body[i] = i;
    }

    m_nic->pioSend( (m_nic->getNodeId() + 1 ) % 2, SHORT_MSG, iovec, xxx );

    m_state = WaitSend; 

    m_selfLink->send(0,NULL);
}

void NicTester::waitSend()
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    m_state = WaitRecv;
}

void NicTester::waitRecv( Nic::XXX* xxx )
{
    DmaEntry* entry = static_cast<DmaEntry*>(xxx->usrData);
    m_dbg.verbose(CALL_INFO,1,0,"src=%d len=%lu pid=%d\n",xxx->src, 
            xxx->len, entry->hdr.pid);

    for ( unsigned int i = 0; i < xxx->len - sizeof(entry->hdr); i++ ) {
        if ( entry->body[i] != (i&0xff) ) {
            printf("ERROR %d != %d\n",i,entry->body[i]);
        }
    }
    primaryComponentOKToEndSim();
}
