// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_MISCLIB_NODE
#define _H_EMBER_MISCLIB_NODE

#include "embergen.h"
#include "libs/emberLib.h"
#include "libs/miscEvents/emberGetNodeNumEvent.h"
#include "libs/miscEvents/emberGetNumNodesEvent.h"
#include "libs/miscEvents/emberMallocEvent.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberMiscLib : public EmberLib {

  public:

    SST_ELI_REGISTER_MODULE(
        EmberMiscLib,
        "ember",
        "miscLib",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        "SST::Ember::EmberMiscLib"
    )
    SST_ELI_DOCUMENT_PARAMS(
	)

    EmberMiscLib( Params& params ) {}

    void getNodeNum( EmberGenerator::Queue& q, int* ptr ) {
        q.push( new EmberGetNodeNumEvent( api(), m_output, ptr ) );
    }

    void getNumNodes( EmberGenerator::Queue& q, int* ptr ) {
        q.push( new EmberGetNumNodesEvent( api(), m_output, ptr ) );
    }

    void malloc( EmberGenerator::Queue& q, Hermes::MemAddr* addr, size_t length, bool backed = false ) {
        q.push( new EmberMallocEvent( api(), m_output, addr, length, backed ) );
    }

  private:
	Misc::Interface& api() { return *static_cast<Misc::Interface*>(m_api); }
};

}
}

#endif
