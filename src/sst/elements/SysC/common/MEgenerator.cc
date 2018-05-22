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
#include "sst/elements/SysC/common/MEgenerator.h"

#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;

#include <sst/core/element.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/link.h>

#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
using namespace SysC;

void MEGenerator::setup(){}
void MEGenerator::finish(){printData();}
bool MEGenerator::Status(){return 0;}

MEGenerator::MEGenerator(SST::ComponentId_t _id,Params& _params)
: Component(_id) {
  cout<<"Constructing MEGenerator"<<endl;
  cycles=0;
  num_cycles=_params.find_integer(MEG_NUM_CYCLES,DEF_MEG_NUM_CYCLES);
  cout<<"MEGenerator num_cycles="<<std::dec<<num_cycles<<endl;
  registerAsPrimaryComponent();
  cout<<"MEGenerator registerd as primary"<<endl;
  primaryComponentDoNotEndSim();
  cout<<"MEGenerator do not end sim"<<endl;
  std::string clock_string = _params.find_string(MEG_CLOCK,STR(DEF_MEG_CLOCK));
  registerClock(clock_string,
                new SST::Clock::Handler<ThisType>(this,&ThisType::tic)
               );
  cout<<"MEGenerator register clock"<<endl;
  //buffer=new unsigned char[4];
  cout<<MEG_PORT_NAME<<endl;
  link=configureLink(MEG_PORT_NAME,
                     new Event::Handler<ThisType>(this,&ThisType::handleEvent)
                    );
  assert(link);
  cout<<"MEGenerator configured link"<<endl;
  payload_size = _params.find_integer(MEG_DATA_SIZE,DEF_MEG_DATA_SIZE);
  cout<<"MEGenerator data size = "<<payload_size<<endl;
  data= new int[payload_size];
}
void MEGenerator::handleEvent(SST::Event *_event) {
  Event_t* _ev=dynamic_cast<Event_t*>(_event);
  assert(_ev);
  Command_t cmd=_ev->getCmd();
  Address_t address=_ev->getAddr();
  cout<< "Generator:" << std::dec << getCurrentSimTime(getTimeConverter("1ps")) 
      << "ps :Recieved transaction: "
      << " Address= 0x" << hex << setw(8) << setfill('0') << address << dec 
      << " Command=" << SST::MemHierarchy::CommandString[cmd]
      << endl;
  switch(cmd){
  case SST::MemHierarchy::GetSResp:
  case SST::MemHierarchy::GetXResp:
    readData(_ev,address);
    break;
  default:
    break;
  }
  delete _ev;
}
//read data from event
void MEGenerator::readData(Event_t* _ev,Address_t _address) {
  memcpy(&data[_address%16],_ev->getPayload().data(),payload_size);
}
//write data to event
void MEGenerator::writeData(Event_t* _ev,Address_t _address) {
  _ev->setPayload(payload_size,reinterpret_cast<uint8_t*>(&data[_address%16]));
}

bool MEGenerator::tic(SimTime_t _cycle) {
  if(num_cycles<=(++cycles)){
    primaryComponentOKToEndSim();
    return true;
  }
  cout<<"----------------------------"<<endl<<"MEGenerator::tic("<<_cycle<<") "
      <<std::dec<<getCurrentSimTime(getTimeConverter("1ps"))<<"ps"<<endl;
  Address_t adr=rand();
  Command_t cmd= rand()%2 ? SST::MemHierarchy::PutM : SST::MemHierarchy::GetS;
  if(SST::MemHierarchy::PutM == cmd) data[adr%16] = rand();
  //Event_t* event=new Event_t(this,adr,cmd);
  Event_t* event=new Event_t(this,adr,0,cmd); //Changed to accomodate removal of method from memEvent
  writeData(event,adr);
  event->setSize(payload_size);
  link->send(event);
  cout << "MEG: Sent " << SST::MemHierarchy::CommandString[cmd]
      << " with address: 0x" << hex << setw(8) << setfill('0') << adr << dec
      << " and size " <<event->getSize() <<"B"
      << endl;
  return false;
}
void MEGenerator::printData() {
  cout<<"Generator memory:"<<std::hex<<std::setfill('0');
  for(uint32_t i=0;i<payload_size;++i)
    cout<<" 0x"<<std::setw(8)<<data[i];
  cout<<std::dec<<endl;
}

