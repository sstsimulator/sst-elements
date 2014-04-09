// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 */

#include <sst_config.h>
#include "macro_address.h"

using namespace sstmac;

// ----- these functions are used if this is in an sst_message ---- //
void macro_address::serialize(sst_serializer* ser) const
{
	ser->pack(id);

}

void macro_address::deserialize(sst_serializer* ser)
{

  SST::ComponentId_t i;
  ser->unpack(i);

  id = i;
}
