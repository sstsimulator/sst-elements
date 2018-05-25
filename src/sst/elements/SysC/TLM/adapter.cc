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
#include "sst/elements/SysC/TLM/adapter.h"

#include <sst/core/element.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SysC;
//////////////////////////////////////////////////////////////////////////////
//Adapter_base
//////////////////////////////////////////////////////////////////////////////

uint64_t Adapter_base::main_id=0;
Adapter_base::Adapter_base(SST::Component *_comp,
                           SST::Params& _params)
//: BaseType(_comp,_params)
: out("In @p() at @f:@l: ",0,0,Output::STDOUT)
    , payload_queue(this,&ThisType::callBackProxy)
{
  out.setVerboseLevel(_params.find_integer(AD_VERBOSE,DEF_AD_VERBOSE));
  out.verbose(CALL_INFO,1,0,
              "Contructing Adapter_base, verbose = %d\n",out.getVerboseLevel());
  id=main_id++;
  parent_component=_comp;
  manager=new Manager_t(&out);
  //Get Controller
  controller=getPrimaryController();
  if(!controller)
    out.fatal(CALL_INFO,-1,
              "No controller found!\n");
  controller->registerComponent(parent_component);
  FIND_REPORT_BOOL(use_link,AD_USE_LINK,DEF_AD_USE_LINK);
  if(use_link){
    //fetch link_name from params
    std::string link_name;
    FIND_REPORT_STRING(link_name,AD_LINK_NAME,STR(DEF_AD_LINK_NAME));
    //configure the SST link
    sst_link = parent_component->configureLink(link_name,
                                               new Handler_t(this,
                                                             &ThisType::proxy)
                                              );
    if(!sst_link)
      out.fatal(CALL_INFO,-1,
                "No link '%s' found.\n",link_name.c_str());
    out.verbose(CALL_INFO,2,0,
                "Link '%s' configured successfully\n",link_name.c_str());
  }
  FIND_REPORT_BOOL(forward_systemc_end,AD_FW_SC_END,DEF_AD_FW_SC_END);
  FIND_REPORT_BOOL(create_systemc_end,AD_CR_SC_END,DEF_AD_CR_SC_END);
  FIND_REPORT_BOOL(use_streaming_width_as_id,AD_USE_SW_ID,DEF_AD_USE_SW_ID);
  FIND_REPORT_BOOL(no_payload,AD_NO_PAYLOAD,DEF_AD_NO_PAYLOAD);
  FIND_REPORT_BOOL(force_size,AD_FORCE_SIZE,DEF_AD_FORCE_SIZE);
  FIND_REPORT_BOOL(force_align,AD_FORCE_ALIGN,DEF_AD_FORCE_ALIGN);
  FIND_REPORT_BOOL(check_size,AD_CHECK_SIZE,DEF_AD_CHECK_SIZE);
  FIND_REPORT_BOOL(check_align,AD_CHECK_ALIGN,DEF_AD_CHECK_ALIGN);
  FIND_REPORT_INT(enforced_size,AD_SIZE,DEF_AD_SIZE);
  FIND_REPORT_INT(streaming_width_id,AD_SW_ID,DEF_AD_SW_ID);
  out.verbose(CALL_INFO,2,0,
              "Done Constructing Adapter_base\n");
}

void Adapter_base::proxy(Event_t* _ev){
  handleSSTEvent(_ev);
}
void Adapter_base::callBackProxy(Payload_t& _trans,const Phase_t& _phase){
  callBack(_trans,_phase);
}
