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


#ifndef _H_SST_MESSIER_SIM_BACKEND
#define _H_SST_MESSIER_SIM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
	namespace MemHierarchy {

		class Messier : public SimpleMemBackend {
			public:
				Messier(Component *comp, Params &params);
				virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
				void handleMessierResp(SST::Event *event);


			private:

				SST::Link *cube_link;
				std::set<ReqId> outToNVM;
				SST::Link *nvm_link;

		};

	}
}

#endif
