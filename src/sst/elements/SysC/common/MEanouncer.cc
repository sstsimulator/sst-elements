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

#include <sst_config.h>
#include "sst/elements/SysC/common/MEanouncer.h"

#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <sst/elements/memHierarchy/memEvent.h>
using namespace SST;
using namespace SST::SysC;

void MEAnnouncer::setup(){}
void MEAnnouncer::finish(){printData();}
bool MEAnnouncer::Status(){return 0;}
MEAnnouncer::MEAnnouncer(SST::ComponentId_t _id,Params& _params)
: Component(_id) {
  cout<<"Constructing MEAnnouncer"<<endl;
  registerTimeBase("1ns");
  cout<<"MEAnnouncer registered timebase"<<endl;
  registerAsPrimaryComponent();
  cout<<"MEAnnouncer registerd as primary"<<endl;
  link=configureLink(MEA_PORT_NAME,
                     new Event::Handler<ThisType_t>(this,&ThisType_t::handleEvent)
                    );
  assert(link);
  cout<<"MEAnnouncer Link Configured"<<endl;
}
void MEAnnouncer::handleEvent(SST::Event *_event) {
  //cout<<"MEAnnouncer handleEvent()"<<endl;
  Event_t* _ev=dynamic_cast<Event_t*>(_event);
  assert(_ev);
  Command_t cmd=_ev->getCmd();
  Address_t address=_ev->getAddr();
  //Event_t *response=_ev->makeResponse(this);
  Event_t *response=_ev->makeResponse(); //Changed to accomodate removal of method from memEvent
  cout<< "Announcer :" <<std::dec<< getCurrentSimTimeNano() 
      << "ns :Recieved transaction: "
      << " Address=" <<std::hex << address 
      << " Command=" << SST::MemHierarchy::CommandString[cmd]
      << endl;
  switch(cmd){
  case SST::MemHierarchy::GetS:
    writeData(response,address);
    break;
  case SST::MemHierarchy::PutM:
    readData(_ev,address);
  default:
    break;
  }
  link->send(response);
  delete _ev;
  //cout<<"MEAnnouncer END handleEvent()"<<endl;
}
//read data from event
void MEAnnouncer::readData(Event_t* _ev,Address_t _address) {
  //Vector_t& payload=_ev->getPayload();
  memcpy(&data[_address%16],_ev->getPayload().data(),4);
}
//write data to event
void MEAnnouncer::writeData(Event_t* _ev,Address_t _address) {
  _ev->setPayload(4,reinterpret_cast<uint8_t*>(&data[_address%16]));
}
void MEAnnouncer::printData() {
  cout<<"Announcer memory :"<<std::hex<<std::setfill('0');
  for(int N:data)
    cout<<" 0x"<<std::setw(8)<<N;
  cout<<endl;
}

