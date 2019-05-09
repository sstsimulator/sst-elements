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


#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include "membackend/pagePlacementMethods.h"
#include "sst/elements/Opal/Opal.h"

#include <cmath>
using namespace SST;
using namespace SST::MemHierarchy;

#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) m_dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define Debug(level, fmt, ... )
#endif

PagePlacementMethods::PagePlacementMethods(Component *comp, Params& params )
{

    owner = comp;

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    opal_latency = (uint32_t)params.find<uint32_t>("opal_latency", 2000);	// 2us

    page_placement_method = params.find<uint64_t>("method", 0);

    num_pages_to_migrate = params.find<uint64_t>("num_pages_to_migrate", 50);
    std::cerr << owner->getName().c_str() << " num_pages_to_migrate: " << num_pages_to_migrate << std::endl;

    node_num = params.find<uint32_t>("node", "0");
    if(node_num < 9999)
    	total_nodes = 1;
    else
    	total_nodes = params.find<uint32_t>("total_nodes", 1);

    std::cerr << owner->getName().c_str() << " total nodes: " << total_nodes << std::endl;

    epoch_count = new int[total_nodes];
    migration_threshold = params.find<uint32_t>("migration_threshold", 10);

    dynamic_threshold = params.find<uint32_t>("dynamic_threshold", 0);
    std::cerr << owner->getName().c_str() << " dynamic_threshold: " << dynamic_threshold << std::endl;

    page_placement = params.find<uint32_t>("page_placement", 0);
    std::cerr << owner->getName().c_str() << " page_placement: " << page_placement << std::endl;

    /* collecting average page access statistics */
    std::string output_file_name = params.find<std::string>("output_file", "NULL");
    if(output_file_name == "NULL")
    	out.fatal(CALL_INFO,-1,"error output file is NULL");

    char* buffer = (char*) malloc(sizeof(char) * 1024);
    if(! (node_num < 9999) ) {
    	output_file = new ofstream[total_nodes+1];
    	stats_average_page_access = new std::map<uint64_t, std::pair<uint64_t, uint64_t>>[total_nodes+1];
    	for(int i=0; i<total_nodes; i++)
    	{
    		epoch_count[i] = 0;
    		memset(buffer, 0 , 1024);
    		sprintf(buffer, "%s_node%d.%s.txt",output_file_name.c_str(),i,owner->getName().c_str());
    		std::cerr << owner->getName().c_str() << " file name: " <<buffer << std::endl;
    		output_file[i].open(buffer, std::ofstream::out | std::ofstream::trunc);
    	}
    	if(! (node_num < 9999) ) {
    		memset(buffer, 0 , 1024);
    		sprintf(buffer, "%s_%s.txt",output_file_name.c_str(),owner->getName().c_str());
    		std::cerr << owner->getName().c_str() << " file name: " <<buffer << std::endl;
    		output_file[total_nodes].open(buffer, std::ofstream::out | std::ofstream::trunc);
    	}
    }
	else {
		output_file = new ofstream[1];
		stats_average_page_access = new std::map<uint64_t, std::pair<uint64_t, uint64_t>>[1];
		epoch_count[0] = 0;
		memset(buffer, 0 , 1024);
		sprintf(buffer, "%s_%s.txt",output_file_name.c_str(),owner->getName().c_str());
		std::cerr << owner->getName().c_str() << " file name: " <<buffer << std::endl;
		output_file[0].open(buffer, std::ofstream::out | std::ofstream::trunc);
	}


	/* end of collecting average page access statistics */
    memset(buffer, 0 , 1024);
	sprintf(buffer, "%s_%s_page_access.txt",output_file_name.c_str(),owner->getName().c_str());
	std::cerr << owner->getName().c_str() << " file name: " <<buffer << std::endl;
	page_access_file.open(buffer, std::ofstream::out | std::ofstream::trunc);
	/* collecting all page accesses */

	/* end of collecting all page accesses */

	free(buffer);

    update_lock = new std::mutex[total_nodes];

    switch(page_placement_method)
    {
    case 0:
    	page_access_count = new std::map<uint64_t, uint64_t>[total_nodes];
        pages_with_threshold_accesses = new std::map<uint64_t,int>[total_nodes];
        previous_page_migrations = new std::map<uint64_t,int>[total_nodes];
        page_access_migration_threshold = new uint64_t[total_nodes];
        for(uint32_t n=0; n<total_nodes; n++)
        	page_access_migration_threshold[n] = migration_threshold;

		for(uint32_t n=0; n<total_nodes; n++)
			std::cerr << owner->getName().c_str() << " page_access_migration_threshold["<<n<<"]: " << page_access_migration_threshold[n] << std::endl;

		break;

    default:
		out.fatal(CALL_INFO,-1,"invalid page migration method");
    	break;
    }

}

void PagePlacementMethods::registerEvent(SST::Event *event, uint64_t global_address)
{

    MemEvent * ev = static_cast<MemEvent*>(event);

	uint32_t node;
	if(node_num < 9999)
		node = 0;
	else {
		std::string requester = ev->getRqstr();

		if(requester=="None")
			out.fatal(CALL_INFO,-1,"invalid requester None");
		node = (uint32_t)std::stoi(requester.substr(4,requester.find(".")));

		if(node >= total_nodes && node < 0)
			out.fatal(CALL_INFO,-1,"invalid requester Node");
	}

	update_lock[node].lock();

	switch(page_placement_method)
	{
	case 0:
		{
			uint64_t page = global_address - (global_address % 4096);
			uint64_t page_number = ev->getBaseAddr()/4096;

			if(stats_page_map.find(page_number) == stats_page_map.end())
				stats_page_map[page_number] = 0;
			stats_page_map[page_number]++;

			if(page_access_count[node].find(page) == page_access_count[node].end())
				page_access_count[node][page] = 0;
			page_access_count[node][page]++;
			//std::cerr << owner->getName().c_str() << " Node: " << node << " page: " << page << " pagecount: " << page_access_count[node][page] << std::endl;

			if(page_placement) {
				if(page_access_count[node][page] > page_access_migration_threshold[node])
				{
					//if(std::find(pages_with_threshold_accesses[node].begin(), pages_with_threshold_accesses[node].end(), page) == pages_with_threshold_accesses[node].end())
					//{
					if(previous_page_migrations[node].find(page) == previous_page_migrations[node].end())
					{
						//std::cerr << owner->getName().c_str() << " Node: " << node << " page: " << page << " global address: " << global_address << " storing in threshold access count: " << page_access_count[node][page] << std::endl;
						pages_with_threshold_accesses[node][page] = page_access_count[node][page];
					}
					//}
				}
			}
		}
		break;

	default:
		out.fatal(CALL_INFO,-1,"error page migration method");
	}

	update_lock[node].unlock();
}

void PagePlacementMethods::handleEvents(SST::Event *event)
{

	OpalEvent * ev =  dynamic_cast<OpalComponent::OpalEvent*> (event);
	uint32_t node = ev->getNodeId();
	//std::cout << owner->getName().c_str() << " received page migration request from Opal" << std::endl;
	update_lock[node].lock();

	int pages_exceeded_threshold = pages_with_threshold_accesses[node].size();

	switch(ev->getType())
	{
	case SST::OpalComponent::EventType::PAGE_REFERENCE:
	{
		switch(page_placement_method)
		{
		case 0:
			{
				/* sort the list */
				std::vector<std::pair<uint64_t,int> > pages_to_migrate;

				std::copy(pages_with_threshold_accesses[node].begin(),pages_with_threshold_accesses[node].end(),
						std::back_inserter<std::vector<std::pair<uint64_t,int> > >(pages_to_migrate));

				std::sort(pages_to_migrate.begin(),pages_to_migrate.end(),
						[](const std::pair<uint64_t,int>& l, const std::pair<uint64_t,int>& r) {if(l.second != r.second) return (l.second > r.second); });//return (l.first<r.first); });

				/* send top pages to Opal */

				previous_page_migrations[node].clear();
				std::vector<std::pair<uint64_t, int>>::iterator st,en;
				st = pages_to_migrate.begin();
				en = pages_to_migrate.end();

				int num_pages_migrated = 0;

				while( st != en )
				{
					if(num_pages_migrated == num_pages_to_migrate)
						break;

					uint64_t page = st->first;

					uint64_t count = page_access_count[node][page];
					//std::cerr << owner->getName().c_str() << " Node: " << node << " sending to opal page: " << page << " count: " << count << " pages_with_threshold_accesses: " <<
					//		pages_with_threshold_accesses[node].size() << std::endl;
					OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE);
					tse->setNodeId(node);
					tse->setResp(0,page,count);
					opal_link->send(tse);

					previous_page_migrations[node][page] = 1;

					num_pages_migrated++;

					st++;
				}

				OpalEvent * tse = new OpalEvent(OpalComponent::EventType::PAGE_REFERENCE_END);
				tse->setNodeId(node);
				opal_link->send(tse);

				pages_with_threshold_accesses[node].clear();

				page_access_count[node].clear();

			}
			break;

		default:
			out.fatal(CALL_INFO,-1,"error page migration method");
		}
	}
	break;

	case SST::OpalComponent::EventType::IPC_INFO:
	{
		if(dynamic_threshold) {
			int thr_change = ev->getSize(); // embedded change in threshold in size parameter
			uint64_t threshold = page_access_migration_threshold[node];
			threshold = (threshold + thr_change < 10) ? 10 : (threshold + thr_change) > 100 ? 100 : threshold + thr_change;

			//std::cerr << owner->getName().c_str() << " node: " << node << " previous_page_mig_thr: " << page_access_migration_threshold[node] << " current_page_mig_thr: " << threshold << std::endl;
			page_access_migration_threshold[node] = threshold;
		}
	}
	break;

	case SST::OpalComponent::EventType::PRINT_AVG_PAGE_ACCESS:
	{
		//std::cerr << owner->getName().c_str() << " node: " << node << " print average page_access_count[n].size(): " << page_access_count[node].size() <<std::endl;
		uint64_t n = node;
		if(node_num < 9999)
			n = 0;
		epoch_count[n]++;
		std::map<uint64_t, uint64_t>::iterator st1,en1;
		st1 = page_access_count[n].begin();
		en1 = page_access_count[n].end();
		int average_page_access = 0;
		while(st1 != en1) {
			average_page_access = average_page_access + st1->second;
			st1++;
		}
		if(!page_access_count[n].empty())
			average_page_access = average_page_access/page_access_count[n].size();

		stats_average_page_access[n][epoch_count[n]] = std::make_pair(page_access_count[n].size(),average_page_access);

		//if(!(node_num < 9999))
		//	stats_average_page_access[total_nodes][epoch_count[n]] = std::make_pair(n,average_page_access);

		if(!page_placement)
			page_access_count[n].clear();

	}
	break;

	default:
		out.fatal(CALL_INFO,-1,"error page migration invalid request");
	}

	//if(dynamic_threshold) {
	//	int new_page_access_mig_thresh = ((float)pages_exceeded_threshold/num_pages_to_migrate)*page_access_migration_threshold[node];
	//	page_access_migration_threshold[node] = new_page_access_mig_thresh < 10 ? 10 : new_page_access_mig_thresh;
	    //std::cerr << owner->getName().c_str() << " page_access_migration_threshold["<<node<<"]: " << page_access_migration_threshold[node] << " new_page_access_mig_thresh: "
	    //		<< new_page_access_mig_thresh << " pages_exceeded_threshold: " << pages_exceeded_threshold << std::endl;
	//}

	delete ev;

	update_lock[node].unlock();
}

void PagePlacementMethods::finish()
{
	if(node_num < 9999) {
		std::map<uint64_t, std::pair<uint64_t, uint64_t>>::iterator st, en;
		st = stats_average_page_access[0].begin();
		en = stats_average_page_access[0].end();
		output_file[0] << owner->getName().c_str() << ",node_"<<node_num<<"pm_"<<page_placement<<"_nPM_"<<num_pages_to_migrate<<"_pMT_"<<migration_threshold<<"_dyT_"<<dynamic_threshold << std::endl;
		output_file[0] << "avg page access, num_pages_accessed" << std::endl;
		while(st != en) {
			output_file[0] << st->second.second << ", " << st->second.first << "\n";
			st++;
		}
		output_file[0].close();
	}
	else {
		for(int i=0; i<=total_nodes; i++)
		{
			std::map<uint64_t, std::pair<uint64_t, uint64_t>>::iterator st, en;
			st = stats_average_page_access[i].begin();
			en = stats_average_page_access[i].end();
			output_file[i] << owner->getName().c_str() << ",node_"<<i<<"pm_"<<page_placement<<"_nPM_"<<num_pages_to_migrate<<"_pMT_"<<migration_threshold<<"_dyT_"<<dynamic_threshold << std::endl;
			output_file[i] << "avg page access, num_pages_accessed" << std::endl;
			while(st != en) {
				output_file[i] << st->second.second << ", " << st->second.first << "\n";
				st++;
			}
			output_file[i].close();
		}
	}

	std::map<uint64_t, uint64_t>::iterator st, en;
	st = stats_page_map.begin();
	en = stats_page_map.end();
	page_access_file << owner->getName().c_str() << "_pm_"<<page_placement<<"_nPM_"<<num_pages_to_migrate<<"_pMT_"<<migration_threshold<<"_dyT_"<<dynamic_threshold << std::endl;
	page_access_file << "count" << std::endl;
	while(st != en) {
		page_access_file << st->second << std::endl;
		st++;
	}
}


