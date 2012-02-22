/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

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
