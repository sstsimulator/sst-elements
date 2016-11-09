// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/link.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memNIC.h"
#include "sst/elements/memHierarchy/membackend/backing.h"

namespace SST {
namespace MemHierarchy {

class MemBackendConvertor;

class MemController : public SST::Component {
public:
	typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    virtual void handleMemResponse( MemEvent* );

private:

    MemController();  // for serialization only
    ~MemController() {}

    void notifyListeners( MemEvent* ev ) {
        if (  ! listeners_.empty()) {
            CacheListenerNotification notify(ev->getAddr(), ev->getVirtualAddress(), 
                        ev->getInstructionPointer(), ev->getSize(), READ, HIT);

            for (unsigned long int i = 0; i < listeners_.size(); ++i) {
                listeners_[i]->notifyAccess(notify);
            }
        }
    }

    void fixupParam( Params& params, const std::string oldKey, const std::string newKey ) {
        bool found;

        std::string value = params.find<std::string>(oldKey,found);
        if ( found ) {
            params.insert( newKey , value );
        //    params.erase( oldKey );
        }
    }

    void fixupParams( Params& params, const std::string oldKey, const std::string newKey ) {
        Params tmp = params.find_prefix_params( oldKey );

        std::set<std::string> keys = tmp.getKeys();   
        std::set<std::string>::iterator iter = keys.begin();
        for ( ; iter != keys.end(); ++iter ) {
            std::string value = tmp.find<std::string>( (*iter) );
            params.insert( newKey + (*iter), value );
        //    params.erase( oldKey + (*iter) );
        }
    }

    void handleEvent( SST::Event* );
    bool clock( SST::Cycle_t );
    void performRequest( MemEvent* );
    void performResponse( MemEvent* );
    void processInitEvent( MemEvent* );

    Output dbg;

    MemBackendConvertor*    memBackendConvertor_;
    Backend::Backing*       backing_; 

    SST::Link*  cacheLink_;         // Link to the rest of memHierarchy 
    MemNIC*     networkLink_;       // Link to the rest of memHierarchy if we're communicating over a network

    std::vector<CacheListener*> listeners_;


    bool isRequestAddressValid(Addr addr){
        return (addr < memSize_);
    }

    int         protocol_;
    size_t      memSize_;
};

}}

#endif /* _MEMORYCONTROLLER_H */
