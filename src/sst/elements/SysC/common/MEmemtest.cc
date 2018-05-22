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
#include "sst/elements/SysC/common/MEmemtest.h"

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

void MEMemTest::setup(){}
void MEMemTest::finish(){printData();}
bool MEMemTest::Status(){return 0;}
MEMemTest::MEMemTest(SST::ComponentId_t _id,Params& _params)
: Component(_id) 
{
  cout<<"Constructing MEMemTest"<<endl;
  cycles=0;
  num_cycles=_params.find_integer("num_cycles",100);
  cout<<"MEMemTest num_cycles = "<<dec<<num_cycles<<endl;
  registerAsPrimaryComponent();
  cout<<"MEMemTest registerd as primary"<<endl;
  primaryComponentDoNotEndSim();
  cout<<"MEMemTest do not end sim"<<endl;
  std::string clock_string=_params.find_string("clock","1MHz");
  registerClock(clock_string,
                new SST::Clock::Handler<ThisType_t>(this,&ThisType_t::tic)
               );
  cout<<"MEMemTest register clock "<<clock_string<<endl;
  //buffer=new unsigned char[4];
  link=configureLink("GeneratorPort",
                     new Event::Handler<ThisType_t>(this,&ThisType_t::handleEvent)
                    );
  assert(link);
  cout << "MEMemTest configured link" << endl;
  payload_size = _params.find_integer("payload_size",32);
  cout << "MEMemTest payload_size = " << payload_size << endl;
  data_size = _params.find_integer("data_size",64);
  cout << "MEMemTest data_size = " << data_size << endl;
  max_index = data_size-payload_size;
  cout << "MEMemTest max_index = " << max_index <<endl;
  data = new uint8_t[data_size];
  buffer= new uint8_t[payload_size];
  last_cmd = SST::MemHierarchy::GetS;
  old_addresses=new Address_t[num_cycles];
  old_buffers=new uint8_t*[num_cycles];
  for(unsigned int i=0;i<num_cycles;++i)
    old_buffers[i]=new uint8_t[payload_size];
  first_loop_done=false;
  index=0;
}
void MEMemTest::handleEvent(SST::Event *_event) {
  Event_t* _ev=dynamic_cast<Event_t*>(_event);
  assert(_ev);
  Command_t cmd=_ev->getCmd();
  Address_t address=_ev->getAddr();
  cout<< "MemTest:" << dec << getCurrentSimTime(getTimeConverter("1ps")) 
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
void MEMemTest::readData(Event_t* _ev,Address_t _address) {
  memcpy(&data[_address%max_index],_ev->getPayload().data(),payload_size);
  cout << "Read Data :";
  cout << hex << setfill('0');
  uint8_t* temp=_ev->getPayload().data();
  assert(_ev->getAddr() == last_address);
  for(unsigned int i=0;i<payload_size;++i){
    //assert(temp[i] == buffer[i]);
    cout << " " << setw(2) << static_cast<int>(temp[i]);
  }
  cout << dec << endl;
}
//write data to event
void MEMemTest::writeData(Event_t* _ev,Address_t _address) {
  //store value for to compare agains following read
  //cout <<"before memcpy max_index="<<max_index<<endl;
  //cout<< "index="<<_address%max_index<<endl;
  memcpy(buffer,&data[_address%max_index],payload_size);
  //cout<<"after memcpy"<<endl;
  //cout<<"Address="<<dec<<_address
  //    <<"max_index="<<max_index
  //    <<"index="<<_address%max_index<<endl;
  _ev->setPayload(payload_size,
                  reinterpret_cast<uint8_t*>(&data[_address%max_index]));
  cout << "Write Data:";
  cout << hex << setfill('0');
  for(unsigned int i=0;i<payload_size;++i){
    cout << " " << setw(2) << static_cast<int>(buffer[i]);
  }
  cout << dec << endl;
}

bool MEMemTest::tic(SimTime_t _cycle) {
  if((num_cycles*2)<=(++cycles)){
    if(first_loop_done){
      primaryComponentOKToEndSim();
      return true;
    }
    else{
      cycles=0;
      num_cycles>>=1;
      index=0;
      first_loop_done=true;
    }
  }
  cout<<"----------------------------"<<endl<<"MEMemTest::tic("<<_cycle<<") "
      <<std::dec<<getCurrentSimTime(getTimeConverter("1ps"))<<"ps"<<endl;
  if(!first_loop_done){
    if(SST::MemHierarchy::GetS==last_cmd){
      last_address= rand();
      last_cmd = SST::MemHierarchy::PutM;
      for(unsigned int i=0;i<payload_size;++i)
        data[(last_address%max_index)+i] = uint8_t(rand()%256);
    }
    else {
      last_cmd = SST::MemHierarchy::GetS;
    }
    //Event_t* event=new Event_t(this,adr,cmd);
    Event_t* event=new Event_t(this,last_address,0,last_cmd);
    //if(SST::MemHierarchy::PutM == last_cmd)
    writeData(event,last_address);
    event->setSize(payload_size);
    link->send(event);
    cout << "MemTest: Sent " 
        << SST::MemHierarchy::CommandString[event->getCmd()]
        << " with address: 0x" 
        << hex << setw(8) << setfill('0') << event->getAddr() << dec
        << " and size " << event->getSize() << "B"
        << endl;
    if(last_cmd==SST::MemHierarchy::PutM){
      old_addresses[index]=last_address;
      memcpy(&old_buffers[index][0],buffer,payload_size);
      ++index;
    }
  }
  else{
    Event_t* event=new Event_t(this,
                               old_addresses[index],
                               0,
                               SST::MemHierarchy::GetS,
                               payload_size);
    link->send(event);
    cout << "Address = " <<old_addresses[index]<<endl;
    cout << "Old Data  :";
    cout << hex << setfill('0');
    for(unsigned int i=0;i<payload_size;++i){
      cout << " " << setw(2) << static_cast<int>(old_buffers[index][i]);
    }
    cout << dec << endl;
    last_address=old_addresses[index];
    memcpy(buffer,&old_buffers[index][0],payload_size);
    ++index;
  }
  return false;
}
void MEMemTest::printData() {
  cout<<"Generator memory:" << hex << setfill('0');
  for(unsigned int i=0;i<data_size;++i)
    cout<<" "<<setw(2)<< static_cast<int>(data[i]);
  cout<<endl;
}

