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

#ifndef SSTCORE_PARAMETERS_H_INCLUDED
#define SSTCORE_PARAMETERS_H_INCLUDED

#include <sstmac/common/sim_parameters.h>
#include <sstmac/common/errors.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/params.h>

#include <sstmac/common/errors.h>

class macro_parameters : public sstmac::sim_parameters
{
protected:
  macro_parameters(SST::Params& params) :
      params_(params)
  {

  }

  macro_parameters() { }

public:
  typedef boost::intrusive_ptr<macro_parameters> ptr;

  static ptr
  construct(SST::Params& params)
  {
    return ptr(new macro_parameters(params));
  }

  virtual std::string
  get_param(const std::string &key) const
  {
    SST::Params::const_iterator it = params_.find(key.c_str());
    return it->second;

  }

  virtual bool
  has_param(const std::string &key) const
  {
    bool has = params_.find(key.c_str()) != params_.end();
    return has;
  }

  virtual void
  add_param(const std::string &key, const std::string &val)
  {
    params_[key.c_str()] = val;
  }

  virtual void
  add_param_override(const std::string &key, const std::string &val)
  {
    params_[key.c_str()] = val;

  }

  virtual void
  combine_into(sim_parameters::ptr sp)
  {
    SST::Params::iterator it, end = params_.end();
  }

  virtual std::string
  to_string() const
  {
    return "macro_parameters";
  }

  virtual
  ~macro_parameters()
  {
  }
	
	virtual sstmac::sim_parameters::ptr
    clone(){
		return construct(params_);
	}
	
	virtual void print_params(std::ostream& os = std::cerr) const {
		SST::Params::const_iterator it, end = params_.end();
		for(it = params_.begin(); it != end; it++){
				os << it->first << " = " << it->second << "\n";
		}
		
	}

	
   virtual sstmac::sim_parameters::ptr 
   subspace_clone(){
     return ptr(new macro_parameters());
   }

   virtual void 
   remove_param(const std::string&){
     sst_throw(sstmac::ssterror, "macro_component::macro_parameters::remove_param - don't call this");
   }


protected:
  SST::Params params_;


};

#endif
