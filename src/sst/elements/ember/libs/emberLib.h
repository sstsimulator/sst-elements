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


#ifndef _H_EMBER_LIBS_EMBERLIB
#define _H_EMBER_LIBS_EMBERLIB

#include <sst/core/subcomponent.h>
#include "sst/elements/hermes/hermes.h"

namespace SST {
namespace Ember {

class EmberLib : public SST::Module {
  public:
	EmberLib() : m_output(NULL), m_api(NULL) {}

	void initApi( Hermes::Interface* api ) { m_api = api; }
	void initOutput( SST::Output* output ) { m_output = output; }

  protected:
	Output* m_output;
	Hermes::Interface* m_api;
};

}
}

#endif
