// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/element.h"

#include "sst/core/element.h"

#include "memEvent.h"

// Initialize our ids
uint64_t SST::MemHierarchy::MemEvent::main_id = 0;


using namespace SST;
using namespace SST::MemHierarchy;

void MemEvent::setPayload(uint32_t size, uint8_t *data)
{
	setSize(size);
	for ( uint32_t i = 0 ; i < size ; i++ ) {
		payload[i] = data[i];
	}
}

void MemEvent::setPayload(std::vector<uint8_t> &data)
{
	payload = data;
}

BOOST_CLASS_EXPORT(MemEvent)

