// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_TRIVIAL_MEMORY_MODEL_H
#define COMPONENTS_FIREFLY_TRIVIAL_MEMORY_MODEL_H

#include "memoryModel/memoryModel.h"

namespace SST {
namespace Firefly {

class TrivialMemoryModel : public MemoryModel {

    class SelfEvent : public SST::Event {
      public:
        SelfEvent( MemoryModel::Callback callback ) : callback(callback) {}
		MemoryModel::Callback callback;
        NotSerializable(SelfEvent)
    };

public:

   SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        TrivialMemoryModel,
        "firefly",
        "TrivialMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
       	SST::Firefly::MemoryModel
    )


    TrivialMemoryModel( ComponentId_t id, Params& params ) : MemoryModel(id)
	{
		m_selfLink = configureSelfLink("Nic::TrivialMemoryModel", "1 ns",
        new Event::Handler<TrivialMemoryModel>(this,&TrivialMemoryModel::handleSelfEvent));
	}
    virtual void printStatus( Output& out, int id ) { }
	virtual void schedHostCallback( int core, std::vector< MemOp >* ops, Callback callback ) {
		for ( size_t i = 0; i < ops->size(); i++ ) {
			if ( (*ops)[i].callback) {
					schedCallback( 0, (*ops)[i].callback );
			}
		}
		schedCallback( 0, callback );
	}
	virtual void schedNicCallback( int unit, int pid, std::vector< MemOp >* ops, Callback callback ) {
		for ( size_t i = 0; i < ops->size(); i++ ) {
			if ( (*ops)[i].callback) {
					schedCallback( 0, (*ops)[i].callback );
			}
		}
		schedCallback( 0, callback );
	}

private:

	void schedCallback( SimTime_t delay, Callback callback ){
		m_selfLink->send( delay , new SelfEvent( callback ) );
	}

	void handleSelfEvent( Event* ev ) {
		SelfEvent* event = static_cast<SelfEvent*>(ev);
		event->callback();
		delete ev;
	}
	Link* m_selfLink;
};

} // namespace Firefly
} // namespace SST
#endif
