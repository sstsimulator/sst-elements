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

#ifndef COMMON_SIMPLEWEDGE_H
#define COMMON_SIMPLEWEDGE_H
#include <sst/core/sst_types.h>
#include "sst/elements/SysC/common/memorywedge.h"
namespace SST{
class Params;


class SimpleWedge_base
    : public MemoryWedge
{
 private:
  typedef SimpleWedge_base   ThisType;
  typedef MemoryWedge   BaseType;
 public:
  SimpleWedge_base(SST::ComponentId_t _id, SST::Params& _params);
  void setup();
  void finish();
 protected:
  virtual void readFromFile(std::string _filename);
  virtual void writeToFile(std::string _filename);
 protected:
  unsigned char*  data_store;
  uint64_t        size;
  uint64_t        base_address;
  uint32_t        size_multiplier; //size exponent
  uint32_t        size_exponent;
};

template<typename EVENT>
class SimpleWedge
: public SimpleWedge_base
{
 private:
  typedef SimpleWedge<EVENT>    ThisType;
  typedef SimpleWedge_base      BaseType;
 public:
  SimpleWedge(SST::ComponentId_t _id, SST::Params& _params);
  void setup(){BaseType::setup();}
  void finish(){BaseType::finish();}
 protected:
  virtual void load(Event_t *_event){
    EVENT* ev=dynamic_cast<EVENT*>(_event);
    if(!ev)
      out.fatal(CALL_INFO,-1,"Wrong event type\n");
    if(isRead(out,ev)){
      Address_t address=getAddress(*ev);
      address-=this->base_address;
      if(address>this->size)
        this->out.fatal(CALL_INFO,-1,
                        "Address '%llX' out of range\n",address+this->base_address);
      uint32_t length=ev->getSize();
      loadPayload(out,*ev,&(this->data_store[address]),length);
    }
  }
  virtual void store(Event_t *_event){
    EVENT* ev=dynamic_cast<EVENT*>(_event);
    if(!ev)
      this->out.fatal(CALL_INFO,-1,"Wrong event type\n");
    if(isWrite(this->out,ev)){
      Address_t address=getAddress(*ev);
      address -= this->base_address;
      if(address > this->size)
        this->out.fatal(CALL_INFO,-1,
                  "Address '%llX' out of range\n",address+this->base_address);
      uint32_t length=ev->getSize();
      loadPayload(this->out,*ev,&(this->data_store[address]),length);
    }
  }
};
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#define SW_SIZE_EXP "size_exponent"
#define DEF_SW_SIZE_EXP 20
#define SW_SIZE_MULTIPLIER "size_mulitplier"
#define DEF_SW_SIZE_MULTIPLIER 256
#define SW_BASE_ADDRESS "base_address"
#define DEF_SW_BASE_ADDRESS 0

#define SW_PARAMS                                                             \
{SW_BASE_ADDRESS,                                                             \
  "uInt: Starting address of store ",                                         \
  STR(DEF_SW_BASE_ADDRESS)},                                                  \
{SW_SIZE_MULTIPLIER,                                                          \
  "uInt: multiplier M in the size (M*2^E)",                                   \
  STR(DEF_SW_SIZE_MULTIPLIER)},                                               \
{SW_SIZE_EXP,                                                                 \
  "uInt: exponent E in the size (M*2^E)",                                     \
  STR(DEF_SW_SIZE_EXP)},                                                      \
WEDGE_PARAMS


}//namespace SST
#endif//COMMON_SIMPLEWEDGE_H
