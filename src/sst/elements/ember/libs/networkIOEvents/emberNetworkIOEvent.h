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

#include <sst/elements/ember/emberevent.h>
#include <sst/elements/hermes/networkIOapi.h>

using namespace Hermes;

namespace SST {
namespace Ember {

typedef Statistic<uint32_t> EmberEventTimeStatistic;

class EmberNetworkIOEvent : public EmberEvent {
public:
    EmberNetworkIOEvent(NetworkIO::Interface& api, Output* output, 
                   EmberEventTimeStatistic* stat = nullptr) :
        EmberEvent(output, stat), m_api(api)
    {
        m_state = IssueCallback;  
    }

protected:
    NetworkIO::Interface& m_api;
};

}
}

