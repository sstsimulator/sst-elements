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


#ifndef __SST_MEMH_PAGEPLACEMENTMETHODS__
#define __SST_MEMH_PAGEPLACEMENTMETHODS__

#include <queue>
#include <mutex>
#include <sst/core/subcomponent.h>
#include <sst/core/event.h>
#include <sst/core/warnmacros.h>
#include <fstream>

#include "sst/elements/memHierarchy/memEvent.h"

namespace SST {
namespace MemHierarchy {

class PagePlacementMethods {

    Output dbg;

    Output out;

    SST::Component *owner;

    std::mutex* update_lock;

    uint32_t opal_latency;

    uint32_t total_nodes;

    SST::Link* opal_link;

    uint64_t num_pages_to_migrate;

    int page_placement_method;

    int dynamic_threshold;

    int *epoch_count;

    int page_placement;

    ofstream *output_file;

    ofstream page_access_file;

    int migration_threshold;

    std::map<uint64_t, std::pair<uint64_t, uint64_t>>* stats_average_page_access;

    std::map<uint64_t, uint64_t> stats_page_map;

    std::map<uint64_t, uint64_t>* page_access_count; // map to store page accesses

    std::map<uint64_t, int> *pages_with_threshold_accesses; // list which stores most frequently accessed pages per node

    std::map<uint64_t, int> *previous_page_migrations;

    uint64_t *page_access_migration_threshold; // array that stores threshold for migrating pages per node

  public:
    uint32_t node_num;

	PagePlacementMethods();

	PagePlacementMethods(Component* comp, Params& params);

	~PagePlacementMethods()
	{
		delete page_access_count;
		delete pages_with_threshold_accesses;
		delete previous_page_migrations;
		delete page_access_migration_threshold;
		delete output_file;
		delete stats_average_page_access;
	}

	void finish(void);

	void setOpalLink(SST::Link* link) { opal_link = link; }

    void registerEvent(SST::Event* event, uint64_t global_address);

    void handleEvents(SST::Event* event);

};

}}
#endif /* __SST_MEMH_PAGEPLACEMENTMETHODS__ */


