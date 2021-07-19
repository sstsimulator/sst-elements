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


#ifndef _H_ZODIAC_FINALIZE_EVENT
#define _H_ZODIAC_FINALIZE_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacFinalizeEvent : public ZodiacEvent {

	public:
		ZodiacFinalizeEvent();
		ZodiacEventType getEventType();

	private:

};

}
}

#endif
