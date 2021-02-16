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

#ifndef _H_SHOGUN_STAT_BUNDLE
#define _H_SHOGUN_STAT_BUNDLE

#include <sst/core/component.h>
#include <sst/core/statapi/statbase.h>

#include "shogun.h"

using namespace SST;

namespace SST {
namespace Shogun {

    class ShogunStatisticsBundle {

    public:
        ShogunStatisticsBundle(const int ports)
            : port_count(ports)
        {

            output_packet_count = (Statistic<uint64_t>**)malloc(sizeof(Statistic<uint64_t>*) * port_count);
            input_packet_count = (Statistic<uint64_t>**)malloc(sizeof(Statistic<uint64_t>*) * port_count);

            for (int i = 0; i < port_count; ++i) {
                output_packet_count[i] = nullptr;
                input_packet_count[i] = nullptr;
            }
        }

        ~ShogunStatisticsBundle()
        {
            free(output_packet_count);
            free(input_packet_count);
        }

        void registerStatistics(ShogunComponent* comp)
        {
            char* subIDName = new char[256];

            for (int i = 0; i < port_count; ++i) {
                sprintf(subIDName, "port%" PRIi32, i);
                output_packet_count[i] = comp->bundleRegisterStatistic("output_packet_count", subIDName);
                input_packet_count[i] = comp->bundleRegisterStatistic("input_packet_count", subIDName);
            }

            packetsMoved = comp->bundleRegisterStatistic("packets_moved");

            delete[] subIDName;
        }

        Statistic<uint64_t>* getOutputPacketCount(const int port)
        {
            return output_packet_count[port];
        }

        Statistic<uint64_t>* getInputPacketCount(const int port)
        {
            return input_packet_count[port];
        }

        Statistic<uint64_t>* getPacketsMoved()
        {
            return packetsMoved;
        }

    private:
        const int port_count;
        Statistic<uint64_t>** output_packet_count;
        Statistic<uint64_t>** input_packet_count;
        Statistic<uint64_t>* packetsMoved;
    };

}
}

#endif
