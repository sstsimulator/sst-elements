// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/link.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/MessierBackend.h"
#include "sst/elements/Messier/NVM_Request.h"

#include "sst/elements/Messier/memReqEvent.h"

#include "membackend/memBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;
//using namespace SST::MessierComponent;


Messier::Messier(Component *comp, Params &params) : SimpleMemBackend(comp,params){
	std::string access_time = "1ns"; //params.find<std::string>("access_time", "1 ns");
	nvm_link = comp->configureLink( "cube_link", access_time,
			new Event::Handler<Messier>(this, &Messier::handleMessierResp));

	bool found = false;
        std::string encStr = params.find<std::string>("encryption_mode", "none", found);
	bool found_i = false;
        std::string intStr =  params.find<std::string>("integrity_mode", "none", found_i);


//	using std::placeholders::_1;
//	using std::placeholders::_2;
//	static_cast<MessierBackend*>(m_backend)->setResponseHandler( std::bind( &Messier::handleMemResponse, this, _1,_2 ) );
	//output->init("VaultSimMemory[@p:@l]: ", 10, 0, Output::STDOUT);
}



bool Messier::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes ){
	// TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
	   if (outToNVM.size() > 64) {
	     return false;
	   }
	//  }

	//    if (outToCubes.find(reqId) != outToCubes.end())
	//      output->fatal(CALL_INFO, -1, "Assertion failed");

	outToNVM.insert( reqId );
	nvm_link->send( new SST::MessierComponent::MemReqEvent(reqId,addr,isWrite,numBytes, 0) ); 
	return true;
}


void Messier::handleMessierResp(SST::Event *event){


	MessierComponent::MemRespEvent *ev = dynamic_cast<MessierComponent::MemRespEvent*>(event);

	if (ev) {
		if ( outToNVM.find( ev->getReqId() ) != outToNVM.end() ) {
			outToNVM.erase( ev->getReqId() );
			handleMemResponse( ev->getReqId() );
			delete event;
		} else {  
			;// output->fatal(CALL_INFO, -1, "Could not match incoming request from cubes\n");
		}
	} else {
		;// output->fatal(CALL_INFO, -1, "Recived wrong event type from cubes\n");
	}
}


