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

#pragma once
#include "sst/elements/ember/embergen.h"
#include "sst/elements/ember/libs/emberLib.h"
#include "sst/elements/ember/libs/emberNetworkIOLib.h"

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberNetworkIOGenerator : public EmberGenerator {
public:
    EmberNetworkIOGenerator(ComponentId_t id, Params& params, std::string name = "");
    ~EmberNetworkIOGenerator() {}
    virtual void completed(const SST::Output*, uint64_t time) {}
    virtual void setup();

protected:
    EmberNetworkIOLib* m_networkIOLib;
    EmberNetworkIOLib& networkIO() { return *m_networkIOLib; }
};

}
}
