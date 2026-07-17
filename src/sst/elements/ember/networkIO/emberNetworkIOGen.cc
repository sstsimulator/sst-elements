// Copyright 2013-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SPDX-FileCopyrightText: Copyright Hewlett Packard Enterprise Development LP
// SPDX-License-Identifier: BSD-3-Clause

#include <sst_config.h>
#include "emberNetworkIOGen.h"

using namespace SST::Ember;

EmberNetworkIOGenerator::EmberNetworkIOGenerator(ComponentId_t id, Params& params, std::string name)
    : EmberGenerator(id, params, name), m_networkIOLib(nullptr)
{
}

void EmberNetworkIOGenerator::setup()
{
    m_networkIOLib = static_cast<EmberNetworkIOLib*>(getLib("networkIO"));
    assert(m_networkIOLib);
}
