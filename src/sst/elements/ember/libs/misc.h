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

  private:
    Output* m_output;
    Misc::Interface* m_api;
};

}
}

#endif
