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

#ifndef _H_THORNHILL_DETAILED_COMPUTE
#define _H_THORNHILL_DETAILED_COMPUTE

#include <sst/core/subcomponent.h>

namespace SST {
namespace Thornhill {

class DetailedCompute : public SubComponent {

  public:

    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Thornhill::DetailedCompute)

	struct Generator {
		std::string name;
		SST::Params params;
	};

    DetailedCompute( ComponentId_t id ) : SubComponent( id ) {}
    DetailedCompute( ComponentId_t id, Params& ) : SubComponent( id ) {}

    virtual ~DetailedCompute(){};
    virtual void start( std::deque< std::pair< std::string, SST::Params > >&,
                 std::function<int()> retFunc, std::function<int()> finiFunc) = 0;
    virtual bool isConnected() = 0;
	virtual std::string getModelName() = 0;
};

}
}

#endif
