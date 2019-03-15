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


#ifndef _H_EMBER_MISCLIB_NODE
#define _H_EMBER_MISCLIB_NODE

#include "embergen.h"
#include "libs/emberLib.h"
#include "libs/miscEvents/emberGetNodeNumEvent.h"
#include "libs/miscEvents/emberGetNumNodesEvent.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberMiscLib : public EmberLib {

  public:
    EmberMiscLib( Output* output, Misc::Interface* api ) : m_output(output), m_api(api) {
    }

    void getNodeNum( EmberGenerator::Queue& q, int* ptr ) {
        q.push( new EmberGetNodeNumEvent( *m_api, m_output, ptr ) ); 
    }

    void getNumNodes( EmberGenerator::Queue& q, int* ptr ) {
        q.push( new EmberGetNumNodesEvent( *m_api, m_output, ptr ) ); 
    }

  private:
    Output* m_output;
    Misc::Interface* m_api;
};

}
}

#endif
