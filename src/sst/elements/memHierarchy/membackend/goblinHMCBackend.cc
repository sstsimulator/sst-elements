// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
#include <membackend/goblinHMCBackend.h>

using namespace SST::MemHierarchy;

GOBLINHMCSimBackend::GOBLINHMCSimBackend(Component* comp, Params& params) : MemBackend(comp, params),
	owner(comp) {

	int verbose = params.find<int>("verbose", 0);

	output = new Output("HMCBackend[@p:@l]: ", verbose, 0, Output::STDOUT);

	hmc_dev_count    = params.find<uint32_t>("device_count", 1);
	hmc_link_count   = params.find<uint32_t>("link_count",  HMC_MIN_LINKS);
	hmc_vault_count  = params.find<uint32_t>("vault_count", HMC_MIN_VAULTS);
	hmc_queue_depth  = params.find<uint32_t>("queue_depth", 32);
	hmc_bank_count   = params.find<uint32_t>("bank_count", HMC_MIN_BANKS);
	hmc_dram_count   = params.find<uint32_t>("dram_count", HMC_MIN_DRAMS);
	hmc_capacity_per_device = params.find<uint32_t>("capacity_per_device", HMC_MIN_CAPACITY);
	hmc_xbar_depth   =  params.find<uint32_t>("xbar_depth", 4);
	hmc_max_req_size =  params.find<uint32_t>("max_req_size", 64);

        link_phy = params.find<float>("link_phy_power", 0.1);
        link_local_route = params.find<float>("link_local_route_power", 0.1);
        link_remote_route = params.find<float>("link_remote_route_power", 0.1);
        xbar_rqst_slot = params.find<float>("xbar_rqst_slot_power", 0.1);
        xbar_rsp_slot = params.find<float>("xbar_rsp_slot_power", 0.1);
        xbar_route_extern = params.find<float>("xbar_route_extern_power", 0.1);
        vault_rqst_slot = params.find<float>("vault_rqst_slot_power", 0.1);
        vault_rsp_slot = params.find<float>("vault_rsp_slot_power", 0.1);
        vault_ctrl = params.find<float>("vault_ctrl_power", 0.1);
        row_access = params.find<float>("row_access_power", 0.1);

	hmc_trace_level  = 0;

	if (params.find<bool>("trace-banks", false)) {
		hmc_trace_level = hmc_trace_level | HMC_TRACE_BANK;
	}

	if (params.find<bool>("trace-queue", false)) {
		hmc_trace_level = hmc_trace_level | HMC_TRACE_QUEUE;
	}

	if (params.find<bool>("trace-cmds", false)) {
		hmc_trace_level = hmc_trace_level | HMC_TRACE_CMD;
	}

	if(params.find<bool>("trace-latency", false)) {
		hmc_trace_level = hmc_trace_level | HMC_TRACE_LATENCY;
	}

	if(params.find<bool>("trace-stalls", false)) {
		hmc_trace_level = hmc_trace_level | HMC_TRACE_STALL;
	}

#if defined( HMC_TRACE_POWER )
        if(params.find<bool>("trace-power", false)) {
          hmc_trace_level = hmc_trace_level | HMC_TRACE_POWER;
        }
#endif

	hmc_tag_count    = params.find<uint32_t>("tag_count", 64);

	hmc_trace_file   = params.find<std::string>("trace_file", "hmc-trace.out");

        params.find_array<std::string>("cmc-lib", cmclibs);

	output->verbose(CALL_INFO, 1, 0, "Initializing HMC...\n");
	int rc = hmcsim_init(&the_hmc,
		hmc_dev_count,
		hmc_link_count,
		hmc_vault_count,
		hmc_queue_depth,
		hmc_bank_count,
		hmc_dram_count,
		hmc_capacity_per_device,
		hmc_xbar_depth);

	if(0 != rc) {
		output->fatal(CALL_INFO, -1, "Unable to initialize HMC back end model, return code is %d\n", rc);
	} else {
		output->verbose(CALL_INFO, 1, 0, "Initialized successfully.\n");
	}

#if defined( HMC_TRACE_POWER )
        // load the power config
        rc = hmcsim_power_config( &the_hmc,
                                 link_phy,
                                 link_local_route,
                                 link_remote_route,
                                 xbar_rqst_slot,
                                 xbar_rsp_slot,
                                 xbar_route_extern,
                                 vault_rqst_slot,
                                 vault_rsp_slot,
                                 vault_ctrl,
                                 row_access );
        if( rc != 0 ){
	    output->fatal(CALL_INFO, -1,
                          "Unable to initialize the HMC-Sim power configuration; return code is %d\n", rc);
        }
#endif

        // load the cmc libs
        for( unsigned i=0; i< cmclibs.size(); i++ ){
          if( cmclibs[i].length() > 0 ){
            // attempt to add the cmc lib
            output->verbose(CALL_INFO, 1, 0,
                            "Initializing HMC-Sim CMC Library...\n" );
            std::vector<char> libchars( cmclibs[i].begin(), cmclibs[i].end() );
            rc = hmcsim_load_cmc(&the_hmc, &libchars[0] );
            if( rc != 0 ){
	      output->fatal(CALL_INFO, -1,
                            "Unable to HMC-Sim CMC Library and the return code is %d\n", rc);
            }
          }
        }

	for(uint32_t i = 0; i < hmc_link_count; ++i) {
		output->verbose(CALL_INFO, 1, 0, "Configuring link: %" PRIu32 "...\n", i);
		rc = hmcsim_link_config(&the_hmc,
			hmc_dev_count+1,
			0,
			i,
			i,
			HMC_LINK_HOST_DEV);

		if(0 == rc) {
			output->verbose(CALL_INFO, 1, 0, "Successfully configured link.\n");
		} else {
			output->fatal(CALL_INFO, 1, 0, "Error configuring link %" PRIu32 ", code=%d\n", i, rc);
		}

		nextLink = 0;
	}

	output->verbose(CALL_INFO, 1, 0, "Populating tag entries allowed at the controller, max tag count is: %" PRIu32 "\n", hmc_tag_count);
	for(uint32_t i = 0; i < hmc_tag_count; i++) {
		tag_queue.push((uint16_t) i);
	}
	output->verbose(CALL_INFO, 1, 0, "Completed populating tag entry list.\n");

	output->verbose(CALL_INFO, 1, 0, "Setting the HMC trace file to %s\n", hmc_trace_file.c_str());
	hmc_trace_file_handle = fopen(hmc_trace_file.c_str(), "wt");

	if(NULL == hmc_trace_file_handle) {
		output->fatal(CALL_INFO, -1, "Unable to create and open the HMC trace file at %s\n", hmc_trace_file.c_str());
	}

	rc = hmcsim_trace_handle(&the_hmc, hmc_trace_file_handle);
	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Unable to set the HMC trace file to %s\n", hmc_trace_file.c_str());
	} else {
		output->verbose(CALL_INFO, 1, 0, "Succesfully set the HMC trace file path.\n");
	}

	output->verbose(CALL_INFO, 1, 0, "Setting the trace level of the HMC to %" PRIu32 "\n", hmc_trace_level);
	rc = hmcsim_trace_level(&the_hmc, hmc_trace_level);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Unable to set the HMC trace level to %" PRIu32 "\n", hmc_trace_level);
	} else {
		output->verbose(CALL_INFO, 1, 0, "Successfully set the HMC trace level.\n");
	}

	output->verbose(CALL_INFO, 1, 0, "Setting the maximum HMC request size to: %" PRIu32 "\n", hmc_max_req_size);
	rc = hmcsim_util_set_all_max_blocksize(&the_hmc, hmc_max_req_size);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Unable to set maximum HMC request size to %" PRIu32 "\n", hmc_max_req_size);
	} else {
		output->verbose(CALL_INFO, 1, 0, "Successfully set maximum request size.\n");
	}

	output->verbose(CALL_INFO, 1, 0, "Initializing the HMC trace log...\n");
	rc = hmcsim_trace_header(&the_hmc);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Unable to write trace header information into the HMC trace file.\n");
	} else {
		output->verbose(CALL_INFO, 1, 0, "Wrote the HMC trace file header successfully.\n");
	}

	zeroPacket(hmc_packet);

	nextLink = 0;

	// We are done with all the config/
	output->verbose(CALL_INFO, 1, 0, "Completed HMC Simulation Backend Initialization.\n");
}

bool GOBLINHMCSimBackend::issueRequest(DRAMReq* req) {
	// We have run out of tags
	if(tag_queue.empty()) {
		output->verbose(CALL_INFO, 4, 0, "Will not issue request this call, tag queue has no free entries.\n");
		return false;
	}

	// Zero out the packet ready for us to populate it with values below
	zeroPacket(hmc_packet);

	output->verbose(CALL_INFO, 4, 0, "Queue front is: %" PRIu16 "\n", tag_queue.front());

	const uint8_t		req_dev  = 0;
	const uint64_t		addr     = req->baseAddr_ + req->amtInProcess_;
	const uint16_t          req_tag  = (uint16_t) tag_queue.front();
	tag_queue.pop();

	hmc_rqst_t       	req_type;

	// Check if the request is for a read or write then transform this into something
	// for HMC simulator to use, right now lets just try 64-byte lengths
	if(req->isWrite_) {
		req_type = WR64;
	} else {
		req_type = RD64;
	}

	const uint8_t           req_link   = (uint8_t) (nextLink);
	nextLink = nextLink + 1;
	nextLink = (nextLink == hmc_link_count) ? 0 : nextLink;

	uint64_t                req_header = (uint64_t) 0;
	uint64_t                req_tail   = (uint64_t) 0;

	output->verbose(CALL_INFO, 8, 0, "Issuing request: %" PRIu64 " %s tag: %" PRIu16 " dev: %" PRIu8 " link: %" PRIu8 "\n",
		addr, (req->isWrite_ ? "WRITE" : "READ"), req_tag, req_dev, req_dev);

	int rc = hmcsim_build_memrequest(&the_hmc,
			req_dev,
			addr,
			req_tag,
			req_type,
			req_link,
			hmc_payload,
			&req_header,
			&req_tail);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Unable to build a request for address: %" PRIu64 "\n", addr);
	}

	hmc_packet[0] = req_header;
	hmc_packet[1] = req_tail;

	rc = hmcsim_send(&the_hmc, &hmc_packet[0]);

	if(HMC_STALL == rc) {
		output->verbose(CALL_INFO, 2, 0, "Issue revoked by HMC, reason: cube is stalled.\n");

		// Restore tag for use later, remember this request did not succeed
		tag_queue.push(req_tag);

		// Return false, the request was not issued so we must tell memory controller to give it
		// back to us when we are free
		return false;
	} else if(0 == rc) {
		output->verbose(CALL_INFO, 4, 0, "Issue of request for address %" PRIu64 " successfully accepted by HMC.\n", addr);

		// Create the request entry which we keep in a table
		HMCSimBackEndReq* reqEntry = new HMCSimBackEndReq(req, owner->getCurrentSimTimeNano());

		// Add the tag and request into our table of pending
		tag_req_map.insert( std::pair<uint16_t, HMCSimBackEndReq*>(req_tag, reqEntry) );
	} else {
		output->fatal(CALL_INFO, -1, "Error issue request for address %" PRIu64 " into HMC on link %" PRIu8 "\n", addr, req_link);
	}

	return true;
}

void GOBLINHMCSimBackend::setup() {

}

void GOBLINHMCSimBackend::finish() {

}

void GOBLINHMCSimBackend::clock() {
	output->verbose(CALL_INFO, 8, 0, "Clocking HMC...\n");
	int rc = hmcsim_clock(&the_hmc);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Error: clock call to the HMC failed.\n");
	}

	// Call to process any responses from the HMC
	processResponses();
}

void GOBLINHMCSimBackend::processResponses() {
	int rc = HMC_OK;

	printPendingRequests();

	for(uint32_t i = 0; i < (uint32_t) the_hmc.num_links; ++i) {
		output->verbose(CALL_INFO, 4, 0, "Pooling responses on link %d...\n", i);

		rc = HMC_OK;

		while(rc != HMC_STALL) {
			rc = hmcsim_recv(&the_hmc, 0, i, &hmc_packet[0]);

			if(HMC_OK == rc) {
				uint64_t	resp_head = 0;
				uint64_t	resp_tail = 0;
				hmc_response_t	resp_type;
				uint8_t		resp_length = 0;
				uint16_t	resp_tag = 0;
				uint8_t		resp_return_tag = 0;
				uint8_t		resp_src_link = 0;
				uint8_t		resp_rrp = 0;
				uint8_t		resp_frp = 0;
				uint8_t		resp_seq = 0;
				uint8_t		resp_dinv = 0;
				uint8_t		resp_errstat = 0;
				uint8_t		resp_rtc = 0;
				uint32_t	resp_crc = 0;

				int decode_rc = hmcsim_decode_memresponse(&the_hmc,
					&hmc_packet[0],
					&resp_head,
					&resp_tail,
					&resp_type,
					&resp_length,
					&resp_tag,
					&resp_return_tag,
					&resp_src_link,
					&resp_rrp,
					&resp_frp,
					&resp_seq,
					&resp_dinv,
					&resp_errstat,
					&resp_rtc,
					&resp_crc);

				if(HMC_OK == decode_rc) {
					output->verbose(CALL_INFO, 4, 0, "Successfully decoded an HMC memory response for tag: %" PRIu16 "\n", resp_tag);

					std::map<uint16_t, HMCSimBackEndReq*>::iterator locate_tag = tag_req_map.find(resp_tag);

					if(locate_tag == tag_req_map.end()) {
						output->fatal(CALL_INFO, -1, "Unable to find tag: %" PRIu16 " in the tag/request lookup table.\n", resp_tag);
					} else {
						HMCSimBackEndReq* matchedReq = locate_tag->second;

						DRAMReq* orig_req = matchedReq->getRequest();

						output->verbose(CALL_INFO, 4, 0, "Matched tag %" PRIu16 " to request for address: %" PRIu64 ", processing time: %" PRIu64 "ns\n",
							resp_tag, (orig_req->baseAddr_ + orig_req->amtInProcess_),
							owner->getCurrentSimTimeNano() - matchedReq->getStartTime());

						// Pass back to the controller to be handled, HMC sim is finished with it
						ctrl->handleMemResponse(orig_req);

						// Clear element from our map, it has been processed so no longer needed
						tag_req_map.erase(resp_tag);

						// Put the available tag back into the queue to be used
						tag_queue.push(resp_tag);

						// Delete our entry to free memory
						delete matchedReq;
					}
				} else if(HMC_STALL == decode_rc) {
					// Should this situation happen? We have pulled the request now we want it decoded, if it stalls is it lost?
					output->verbose(CALL_INFO, 8, 0, "Attempted to decode an HMC memory request but HMC returns stall.\n");
				} else {
					fflush(hmc_trace_file_handle);
					output->fatal(CALL_INFO, -1, "Error: call to decode an HMC sim memory response returned an error code (code=%d, possible tag=%" PRIu16 ")\n", decode_rc, resp_tag);
				}
			} else if(HMC_STALL == rc) {
				output->verbose(CALL_INFO, 8, 0, "Call to HMC simulator recv returns a stall, no requests in flight.\n");
			} else {
				output->fatal(CALL_INFO, -1, "Error attempting to call a recv pool on the HMC simulator.\n");
			}
		}
	}
}

GOBLINHMCSimBackend::~GOBLINHMCSimBackend() {
	output->verbose(CALL_INFO, 1, 0, "Freeing the HMC resources...\n");
	hmcsim_free(&the_hmc);

	output->verbose(CALL_INFO, 1, 0, "Closing HMC trace file...\n");
	fclose(hmc_trace_file_handle);

	output->verbose(CALL_INFO, 1, 0, "Completed.\n");
	delete output;
}

void GOBLINHMCSimBackend::zeroPacket(uint64_t* packet) const {
	for(int i = 0; i < HMC_MAX_UQ_PACKET; ++i) {
		packet[i] = (uint64_t) 0;
	}
}

void GOBLINHMCSimBackend::printPendingRequests() {
	output->verbose(CALL_INFO, 4, 0, "Pending requests:\n");

	std::map<uint16_t, HMCSimBackEndReq*>::iterator pending_itr;

	for(pending_itr = tag_req_map.begin(); pending_itr != tag_req_map.end(); pending_itr++) {
		output->verbose(CALL_INFO, 8, 0, "Tag: %8" PRIu16 " for address %" PRIu64 "\n",
			pending_itr->first, pending_itr->second->getRequest()->addr_);
	}
}
