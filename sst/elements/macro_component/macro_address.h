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

/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 */

#ifndef MACROADDRESS_H_
#define MACROADDRESS_H_

#include <sst/core/sst_types.h>

#include <sstmac/common/nodeaddress.h>
#include <sstmac/common/messages/sst_serializer.h>

#include <sstmac/common/logger.h>

#include <iostream>
#include <sstream>
#include <stdint.h>

class macro_address : public sstmac::nodeaddress
{

protected:
  SST::ComponentId_t id;

public:
  typedef boost::intrusive_ptr<macro_address> ptr;

  macro_address()
  {
    id = 9999999999;

    if (!converter_)
      converter_ = macro_address_converter::construct();

  }

  macro_address(SST::ComponentId_t i) //: dbg_("address")
  {

    id = i;

    if (!converter_)
      converter_ = macro_address_converter::construct();
  }

  virtual
  ~macro_address()
  {

  }

  virtual long
  unique_id() const
  {
    return id;
  }

  friend class macro_address_converter;

  class macro_address_converter : public nodeaddress::address_converter
  {

    macro_address_converter()
    {
    }

  public:
    virtual nodeaddress::ptr
    convert(long uid)
    {
      return macro_address::construct(uid);
    }

    virtual
    ~macro_address_converter()
    {
    }

    typedef boost::intrusive_ptr<macro_address_converter> ptr;

    static macro_address_converter::ptr
    construct()
    {
      return ptr(new macro_address_converter());
    }
  };

  static ptr
  construct(SST::ComponentId_t i)
  {
    ptr retval(new macro_address(i));

    return retval;
  }

  static ptr
  construct()
  {
    ptr retval(new macro_address());

    return retval;
  }

  static ptr
  construct(ptr &addr)
  {
    ptr retval(new macro_address(addr->id));

    return retval;
  }

  virtual void
  serialize(sstmac::sst_serializer* ser) const;

  virtual void
  deserialize(sstmac::sst_serializer* ser);

  virtual std::string
  to_string() const
  {
    std::stringstream ss;
    ss << "Address(" << id << ")";
    return ss.str();
  }

  bool
  equals(const nodeaddress::ptr &addr) const
  {
	  if(!addr){
		  // sst_throw(sstmac::ssterror, "macroaddress::equals - null incoming address");
		  throw sstmac::ssterror("macroaddress::equals - null incoming address");
	  }
    macro_address::ptr maddr = boost::dynamic_pointer_cast<macro_address>(addr);
	  
	  if(maddr){
		  return (maddr->id == id);
	  }else{
		  return addr->unique_id() == unique_id();  
	  }
  }

};

inline bool
operator==(const macro_address::ptr &a, const macro_address::ptr &b)
{
  return (a->equals(b));
}
inline bool
operator!=(const macro_address::ptr &a, const macro_address::ptr &b)
{
  return (!a->equals(b));
}

#endif /* OMNETPPADDRESS_H_ */
