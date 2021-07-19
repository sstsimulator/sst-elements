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

#include <sst_config.h>

#include "emberShmemGen.h"

using namespace SST;
using namespace SST::Ember;

EmberShmemGenerator::EmberShmemGenerator(
            ComponentId_t id, Params& params, std::string name) :
    EmberGenerator(id, params, name )
{ }

void EmberShmemGenerator::setup()
{
    m_shmem = static_cast<EmberShmemLib*>(getLib("shmem"));
    assert(m_shmem);
    m_shmem->initOutput( &getOutput() );
    m_miscLib = static_cast<EmberMiscLib*>(getLib("misc"));
    assert(m_miscLib);
}
