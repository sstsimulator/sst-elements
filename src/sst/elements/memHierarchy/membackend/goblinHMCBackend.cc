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
#include <membackend/goblinHMCBackend.h>

using namespace SST::MemHierarchy;

// all the standard and cmc hmc packets
const HMCPacket __PACKETS[] = {
  /* name : type : data_len : rqst_len : rsp_len : isCMC */
  { "WR16",       WR16,       8,    16,   2,  false},
  { "WR32",       WR32,       9,    32,   3,  false},
  { "WR48",       WR48,       10,   48,   4,  false},
  { "WR64",       WR64,       11,   64,   5,  false},
  { "WR80",       WR80,       12,   80,   6,  false},
  { "WR96",       WR96,       13,   96,   7,  false},
  { "WR112",      WR112,      14,   112,  8,  false},
  { "WR128",      WR128,      15,   128,  9,  false},
  { "WR256",      WR256,      79,   256,  17, false},
  { "P_WR16",     P_WR16,     8,    16,   2,  false},
  { "P_WR32",     P_WR32,     9,    32,   3,  false},
  { "P_WR48",     P_WR48,     10,   48,   4,  false},
  { "P_WR64",     P_WR64,     11,   64,   5,  false},
  { "P_WR80",     P_WR80,     12,   80,   6,  false},
  { "P_WR96",     P_WR96,     13,   96,   7,  false},
  { "P_WR112",    P_WR112,    14,   112,  8,  false},
  { "P_WR128",    P_WR128,    15,   128,  9,  false},
  { "P_WR256",    P_WR256,    79,   256,  17, false},
  { "RD16",       RD16,       48,   16,   1,  false},
  { "RD32",       RD32,       49,   32,   1,  false},
  { "RD48",       RD48,       50,   48,   1,  false},
  { "RD64",       RD64,       51,   64,   1,  false},
  { "RD80",       RD80,       52,   80,   1,  false},
  { "RD96",       RD96,       53,   96,   1,  false},
  { "RD112",      RD112,      54,   112,  1,  false},
  { "RD128",      RD128,      55,   128,  1,  false},
  { "RD256",      RD256,      119,  256,  1,  false},
  { "TWOADD8",    TWOADD8,    18,   16,   2,  false},
  { "ADD16",      ADD16,      19,   16,   2,  false},
  { "P_2ADD8",    P_2ADD8,    34,   16,   2,  false},
  { "P_ADD16",    P_ADD16,    35,   16,   2,  false},
  { "TWOADDS8R",  TWOADDS8R,  82,   16,   2,  false},
  { "ADDS16R",    ADDS16R,    83,   16,   2,  false},
  { "INC8",       INC8,       80,   0,    1,  false},
  { "P_INC8",     P_INC8,     84,   0,    1,  false},
  { "XOR16",      XOR16,      64,   16,   2,  false},
  { "OR16",       OR16,       65,   16,   2,  false},
  { "NOR16",      NOR16,      66,   16,   2,  false},
  { "AND16",      AND16,      67,   16,   2,  false},
  { "NAND16",     NAND16,     68,   16,   2,  false},
  { "CASGT8",     CASGT8,     96,   16,   2,  false},
  { "CASGT16",    CASGT16,    98,   16,   2,  false},
  { "CASLT8",     CASLT8,     97,   16,   2,  false},
  { "CASLT16",    CASLT16,    99,   16,   2,  false},
  { "CASEQ8",     CASEQ8,     100,  16,   2,  false},
  { "CASZERO16",  CASZERO16,  101,  16,   2,  false},
  { "EQ8",        EQ8,        105,  16,   2,  false},
  { "EQ16",       EQ16,       104,  16,   2,  false},
  { "BWR",        BWR,        17,   16,   2,  false},
  { "P_BWR",      P_BWR,      33,   16,   2,  false},
  { "BWR8R",      BWR8R,      81,   16,   2,  false},
  { "SWAP16",     SWAP16,     106,  16,   2,  false},
  { "MD_WR",      MD_WR,      16,   16,   2,  false},
  { "MD_RD",      MD_RD,      40,   8,    1,  false},

  // cmc packets.  everything except for the name and the
  // boolean is ignored
  { "CMC04",      CMC04,      0,    0,    0,  true},
  { "CMC05",      CMC05,      0,    0,    0,  true},
  { "CMC06",      CMC06,      0,    0,    0,  true},
  { "CMC07",      CMC07,      0,    0,    0,  true},
  { "CMC20",      CMC20,      0,    0,    0,  true},
  { "CMC21",      CMC21,      0,    0,    0,  true},
  { "CMC22",      CMC22,      0,    0,    0,  true},
  { "CMC23",      CMC23,      0,    0,    0,  true},
  { "CMC32",      CMC32,      0,    0,    0,  true},
  { "CMC36",      CMC36,      0,    0,    0,  true},
  { "CMC37",      CMC37,      0,    0,    0,  true},
  { "CMC38",      CMC38,      0,    0,    0,  true},
  { "CMC39",      CMC39,      0,    0,    0,  true},
  { "CMC41",      CMC41,      0,    0,    0,  true},
  { "CMC42",      CMC42,      0,    0,    0,  true},
  { "CMC43",      CMC43,      0,    0,    0,  true},
  { "CMC44",      CMC44,      0,    0,    0,  true},
  { "CMC45",      CMC45,      0,    0,    0,  true},
  { "CMC46",      CMC46,      0,    0,    0,  true},
  { "CMC47",      CMC47,      0,    0,    0,  true},
  { "CMC56",      CMC56,      0,    0,    0,  true},
  { "CMC57",      CMC57,      0,    0,    0,  true},
  { "CMC58",      CMC58,      0,    0,    0,  true},
  { "CMC59",      CMC59,      0,    0,    0,  true},
  { "CMC60",      CMC60,      0,    0,    0,  true},
  { "CMC61",      CMC61,      0,    0,    0,  true},
  { "CMC62",      CMC62,      0,    0,    0,  true},
  { "CMC63",      CMC63,      0,    0,    0,  true},
  { "CMC69",      CMC69,      0,    0,    0,  true},
  { "CMC70",      CMC70,      0,    0,    0,  true},
  { "CMC71",      CMC71,      0,    0,    0,  true},
  { "CMC72",      CMC72,      0,    0,    0,  true},
  { "CMC73",      CMC73,      0,    0,    0,  true},
  { "CMC74",      CMC74,      0,    0,    0,  true},
  { "CMC75",      CMC75,      0,    0,    0,  true},
  { "CMC76",      CMC76,      0,    0,    0,  true},
  { "CMC77",      CMC77,      0,    0,    0,  true},
  { "CMC78",      CMC78,      0,    0,    0,  true},
  { "CMC85",      CMC85,      0,    0,    0,  true},
  { "CMC86",      CMC86,      0,    0,    0,  true},
  { "CMC87",      CMC87,      0,    0,    0,  true},
  { "CMC88",      CMC88,      0,    0,    0,  true},
  { "CMC89",      CMC89,      0,    0,    0,  true},
  { "CMC90",      CMC90,      0,    0,    0,  true},
  { "CMC91",      CMC91,      0,    0,    0,  true},
  { "CMC92",      CMC92,      0,    0,    0,  true},
  { "CMC93",      CMC93,      0,    0,    0,  true},
  { "CMC94",      CMC94,      0,    0,    0,  true},
  { "CMC102",     CMC102,     0,    0,    0,  true},
  { "CMC103",     CMC103,     0,    0,    0,  true},
  { "CMC107",     CMC107,     0,    0,    0,  true},
  { "CMC108",     CMC108,     0,    0,    0,  true},
  { "CMC109",     CMC109,     0,    0,    0,  true},
  { "CMC110",     CMC110,     0,    0,    0,  true},
  { "CMC111",     CMC111,     0,    0,    0,  true},
  { "CMC112",     CMC112,     0,    0,    0,  true},
  { "CMC113",     CMC113,     0,    0,    0,  true},
  { "CMC114",     CMC114,     0,    0,    0,  true},
  { "CMC115",     CMC115,     0,    0,    0,  true},
  { "CMC116",     CMC116,     0,    0,    0,  true},
  { "CMC117",     CMC117,     0,    0,    0,  true},
  { "CMC118",     CMC118,     0,    0,    0,  true},
  { "CMC120",     CMC120,     0,    0,    0,  true},
  { "CMC121",     CMC121,     0,    0,    0,  true},
  { "CMC122",     CMC122,     0,    0,    0,  true},
  { "CMC123",     CMC123,     0,    0,    0,  true},
  { "CMC124",     CMC124,     0,    0,    0,  true},
  { "CMC125",     CMC125,     0,    0,    0,  true},
  { "CMC126",     CMC126,     0,    0,    0,  true},
  { "CMC127",     CMC127,     0,    0,    0,  true},

  // last entry
  { "EOL",        MD_RD,      0,    0,    0,  false}
};


GOBLINHMCSimBackend::GOBLINHMCSimBackend(ComponentId_t id, Params& params) : ExtMemBackend(id, params) {

	int verbose = params.find<int>("verbose", 0);

        // init the internal counters
        s_link_phy_power=0.;
        s_link_local_route_power=0.;
        s_link_remote_route_power=0.;
        s_xbar_rqst_slot_power=0.;
        s_xbar_rsp_slot_power=0.;
        s_xbar_route_extern_power=0.;
        s_vault_rqst_slot_power=0.;
        s_vault_rsp_slot_power=0.;
        s_vault_ctrl_power=0.;
        s_row_access_power=0.;
        s_link_phy_therm=0.;
        s_link_local_route_therm=0.;
        s_link_remote_route_therm=0.;
        s_xbar_rqst_slot_therm=0.;
        s_xbar_rsp_slot_therm=0.;
        s_xbar_route_extern_therm=0.;
        s_vault_rqst_slot_therm=0.;
        s_vault_rsp_slot_therm=0.;
        s_vault_ctrl_therm=0.;
        s_row_access_therm=0.;

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
#if defined( HMC_DEF_DRAM_LATENCY )
        hmc_dram_latency = params.find<uint32_t>("dram_latency", HMC_DEF_DRAM_LATENCY );
#endif

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

        // These are the new CMC command config strings
        // These are in the form:
        //    /PATH/TO/LIB.SO:CMD:RQST_FLITS:RSP_FLITS
        // CMD = command designator; one of "hmc_rqst_t"
        params.find_array<std::string>("cmd-config", cmcconfigs);

        // These are the new CMC command mappings.  We can
        // map all the existing commands as well as any custom
        // commands.
        // These are in the form:
        //    {WR|RD|POSTED|CUSTOM}:OPC:SIZE:CMD
        // OPC = integer that designates custom command opcodes; if not
        //        using custom commands (CUSTOM), this is ignored
        // SIZE = integer size of the request (ignored for CUSTOM)
        // CMD = target command to execute; one of "hmc_rqst_t"
        params.find_array<std::string>("cmd-map", cmdmaps);

        // register the SST statistics
        if( (hmc_trace_level & HMC_TRACE_CMD) > 0 ){
          registerStatistics();
        }

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

        rc = hmcsim_util_set_max_blocksize(&the_hmc, 0, 128);
        if(0 != rc ){
		output->fatal(CALL_INFO, -1, "Unable to initialize HMC back end max block size to 128\n" );
        }else{
		output->verbose(CALL_INFO, 1, 0, "Initialized block size successfully.\n");
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
#if defined( HMC_DEF_DRAM_LATENCY )
        rc = hmcsim_init_dram_latency( &the_hmc, hmc_dram_latency );
        if( rc != 0 ){
	    output->fatal(CALL_INFO, -1,
                          "Unable to initialize the HMC-Sim dram latency configuration; return code is %d\n", rc);
        }
#endif


        // handle cmc library configs
        if( cmcconfigs.size() > 0 ){
          handleCMCConfig();
        }

        // handle command mappings
        if( cmdmaps.size() > 0 ){
          handleCmdMap();
        }

        // load the cmc libs
        for( unsigned i=0; i< cmclibs.size(); i++ ){
          if( cmclibs[i].length() > 0 ){
            // attempt to add the cmc lib
            output->verbose(CALL_INFO, 1, 0,
                            "Initializing HMC-Sim CMC Library: %s\n", cmclibs[i].c_str() );
            char *libpath = new char[cmclibs[i].size()+1];
            std::copy(cmclibs[i].begin(), cmclibs[i].end(), libpath);
            libpath[cmclibs[i].size()] = '\0';

            rc = hmcsim_load_cmc(&the_hmc, libpath );
            delete[] libpath;
            if( rc != 0 ){
	      output->fatal(CALL_INFO, -1,
                            "Unable to load HMC-Sim CMC Library and the return code is %d\n", rc);
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
			output->fatal(CALL_INFO, 1, 0,
                          "Error configuring link %" PRIu32 ", code=%" PRId32 "\n", i, rc);
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

bool GOBLINHMCSimBackend::HMCRqstToStr( hmc_rqst_t R,
                                        std::string *S ){
  int cur = 0;

  while( __PACKETS[cur].name != "EOL" ){
    if( __PACKETS[cur].type == R ){
      *S = __PACKETS[cur].name;
      return true;
    }
    cur++;
  }

  return false;
}

bool GOBLINHMCSimBackend::strToHMCRqst( std::string S,
                                        hmc_rqst_t *R,
                                        bool isCMC ){

  int cur = 0;

  while( __PACKETS[cur].name != "EOL" ){
    if( __PACKETS[cur].name == S ){
      if( isCMC && __PACKETS[cur].isCMC ){
        *R = __PACKETS[cur].type;
        return true;
      }else if( !isCMC ){
        *R = __PACKETS[cur].type;
        return true;
      }
    }
    cur++;
  }

  return false;
}

void GOBLINHMCSimBackend::handleCMCConfig(){
  std::string delim = ":";
  std::string cmdstr;
  std::string path;
  size_t pos = 0;
  hmc_rqst_t rqst;
  int rqst_flits = -1;
  int rsp_flits = -1;

  for( unsigned i=0; i<cmcconfigs.size(); i++ ){
    // Format:
    //    /PATH/TO/LIB.SO:CMD:RQST_FLITS:RSP_FLITS
    std::string s = cmcconfigs[i];

    std::vector<std::string> vstr;
    splitStr(s,':',vstr);

    if( vstr.size() != 4 ){
      // formatting error
      output->fatal(CALL_INFO, -1,
                    "Unable to process cmc config: %s\n", s.c_str() );
    }

    // -- /path/to/lib.so
    path = vstr[0];

    // -- cmd
    cmdstr = vstr[1];
    if( !strToHMCRqst( cmdstr, &rqst, true ) ){
      output->fatal(CALL_INFO, -1,
                    "Unable find to a suitable CMC HMC command for: %s\n",
                    cmdstr.c_str() );
    }

    // -- rqst_flits
    std::string::size_type sz1;
    rqst_flits = std::stoi(vstr[2],&sz1);

    // -- rsp_flits
    std::string::size_type sz2;
    rsp_flits = std::stoi(vstr[3],&sz2);

    // load the cmc command in HMCSim
    char *libpath = new char[path.size()+1];
    std::copy(path.begin(), path.end(), libpath);
    libpath[path.size()] = '\0';
    int rc = hmcsim_load_cmc(&the_hmc, &libpath[0] );
    delete[] libpath;
    if( rc != 0 ){
      output->fatal(CALL_INFO, -1,
        "Unable to load the mapped HMC-Sim CMC Library %s and the return code is %d\n", path.c_str(), rc);
    }

    // add the new config to our list
    CmcConfig.push_back( new HMCCMCConfig( path, rqst, rqst_flits, rsp_flits ) );

  }// end for
}

void GOBLINHMCSimBackend::splitStr(const string& s,
                                   char c,
                                   vector<string>& v) {
  string::size_type i = 0;
  string::size_type j = s.find(c);

  while (j != string::npos) {
    v.push_back(s.substr(i, j-i));
    i = ++j;
    j = s.find(c, j);
    if (j == string::npos)
      v.push_back(s.substr(i, s.length()));
  }
}

void GOBLINHMCSimBackend::handleCmdMap(){
  size_t pos = 0;
  CMCSrcReq ctype_int;
  int csize_int = -1;
  uint32_t copc_uint = 0xFFFF;
  std::string ctype;
  std::string cstr;
  std::string csize;
  std::string copc;
  hmc_rqst_t rqst;

  for( unsigned i=0; i<cmdmaps.size(); i++ ){
    // Format:
    //    {WR|RD|POSTED|CUSTOM}:SIZE:CMD
    std::string s = cmdmaps[i];

    std::vector<std::string> vstr;
    splitStr(s,':',vstr);

    if( vstr.size() != 4 ){
      // formatting error
      output->fatal(CALL_INFO, -1,
                    "Unable to process command map: %s\n", s.c_str() );
    }

    // parse the string
    // -- WR | RD | POSTED | CUSTOM
    ctype = vstr[0];
    if( ctype == "WR" ){
      ctype_int = SRC_WR;
    }else if( ctype == "RD" ){
      ctype_int = SRC_RD;
    }else if( ctype == "POSTED" ){
      ctype_int = SRC_POSTED;
    }else if( ctype == "CUSTOM" ){
      ctype_int = SRC_CUSTOM;
    }else{
      // error
      output->fatal(CALL_INFO, -1,
                    "Erroneous command type in CMC config: %s\n",
                    ctype.c_str() );
    }

    // -- opcode
    copc = vstr[1];
    std::string::size_type sz;
    copc_uint = std::stoi(copc,&sz);

    // -- size
    csize = vstr[2];
    csize_int = std::stoi(csize,&sz);


    // -- cmd
    cstr = vstr[3];
    if( !strToHMCRqst( cstr, &rqst, false ) ){
      output->fatal(CALL_INFO, -1,
                    "Unable find to a suitable HMC command for: %s\n",
                    cstr.c_str() );
    }

    // add the new mapping to our list
    CmdMapping.push_back( new HMCSimCmdMap(ctype_int, copc_uint,
                                           csize_int, rqst) );
  }
}

bool GOBLINHMCSimBackend::isReadRqst( hmc_rqst_t R ){
  switch(R){
  case RD16:
  case RD32:
  case RD48:
  case RD64:
  case RD80:
  case RD96:
  case RD112:
  case RD128:
  case RD256:
    return true;
  default:
    return false;
  }
  return false;
}

bool GOBLINHMCSimBackend::isPostedRqst( hmc_rqst_t R ){
  switch(R){
  case P_WR16:
  case P_WR32:
  case P_WR48:
  case P_WR64:
  case P_WR80:
  case P_WR96:
  case P_WR112:
  case P_WR128:
  case P_WR256:
  case P_2ADD8:
  case P_ADD16:
  case P_INC8:
    return true;
    break;
  default:
    return false;
    break;
  }
  return false;
}

bool GOBLINHMCSimBackend::issueMappedRequest(ReqId reqId, Addr addr, bool isWrite,
                                       std::vector<uint64_t> ins, uint32_t flags,
                                       unsigned numBytes, uint8_t req_dev,
                                       uint16_t req_tag, bool *Success) {
  // Step 1: decode the incoming command type
  CMCSrcReq Src = SRC_RD;
  bool isPosted = false;
  bool isRead = false;
  if( isWrite ){
    Src = SRC_WR;
  }
  if( (flags & MemEvent::F_NORESPONSE) > 0 ){
    Src = SRC_POSTED;
  }

  // Step 2: walk the CmdMapping table and look for a mapping
  HMCSimCmdMap *MapCmd = NULL;
  for( auto itr = CmdMapping.begin(); itr != CmdMapping.end(); ++itr ){
    MapCmd = *itr;
    if( (MapCmd->getSrcType() == Src) &&
        ((unsigned)(MapCmd->getSrcSize()) == numBytes) ){
      // found a positive map
      std::string name;
      if( !HMCRqstToStr(MapCmd->getTargetType(), &name) ){
        output->fatal(CALL_INFO, -1, "Unable to map hmc_rqst_t to name\n");
      }
      output->verbose(CALL_INFO, 8, 0,
                      "Found mapped request target of %s\n", name.c_str() );

      break;
    }
    MapCmd = NULL;
  }

  // if our MapCmd is NULL, then nothing was found
  if( MapCmd == NULL ){
    return false;
  }

  // Step 3: build our new request
  hmc_rqst_t req_type = MapCmd->getTargetType();

  // -- check to see if the target mapped command is posted, NOT the source
  if( isPostedRqst( req_type ) ){
    isPosted = true;
  }else{
    isPosted = false;
  }

  // -- check to see whether this is a read request
  isRead = isReadRqst( req_type );
  if( isPosted && isRead ){
    // can't have posted reads
    output->fatal(CALL_INFO, -1, "Mapped posted reads are illegal\n" );
  }

  const uint8_t           req_link   = (uint8_t) (nextLink);
  nextLink = nextLink + 1;
  nextLink = (nextLink == hmc_link_count) ? 0 : nextLink;

  uint64_t                req_header = (uint64_t) 0;
  uint64_t                req_tail   = (uint64_t) 0;

  output->verbose(CALL_INFO, 8, 0, "Issuing mapped request: %" PRIu64 " %s tag: %" PRIu16 " dev: %" PRIu8 " link: %" PRIu8 "\n",
  addr, (isWrite ? "WRITE" : "READ"), req_tag, req_dev, req_dev);

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
    output->fatal(CALL_INFO, -1, "Unable to build a mapped request for address: %" PRIu64 "\n", addr);
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
    *Success = false;
  } else if(0 == rc) {
    output->verbose(CALL_INFO, 4, 0, "Issue of mapped request for address 0X%" PRIx64 " successfully accepted by HMC.\n", addr);

    // Create the request entry which we keep in a table
    HMCSimBackEndReq* reqEntry = new HMCSimBackEndReq(reqId, addr,
                                                      getCurrentSimTimeNano(),
                                                      !isPosted);

    // Add the tag and request into our table of pending
    tag_req_map.insert( std::pair<uint16_t, HMCSimBackEndReq*>(req_tag, reqEntry) );

    // Record the I/O statistics
    if( (hmc_trace_level & HMC_TRACE_CMD) > 0 ){
      recordIOStats( hmc_packet[0] );
    }
    *Success = true;
  } else {
    *Success = false;
    output->fatal(CALL_INFO, -1, "Error issue request for address %" PRIu64 " into HMC on link %" PRIu8 "\n", addr, req_link);
  }

  // if there was a positive mapping, we return true
  // the *Success flags determines whether the packet injection
  // was successful or not
  return true;
}

bool GOBLINHMCSimBackend::issueCustomRequest(ReqId reqId, Addr addr, uint32_t cmd,
                                            std::vector<uint64_t> ins, uint32_t flags,
                                            unsigned numBytes) {
  // Step 1: try to grab a tag
  // We have run out of tags
  if(tag_queue.empty()) {
    output->verbose(CALL_INFO, 4, 0,
                    "Will not issue request this call, tag queue has no free entries.\n");
    return false;
  }

  output->verbose(CALL_INFO, 8, 0,
    "Received custom request of opcode %" PRIu32 " of size: %" PRIu32 "\n",
    cmd, numBytes);


  // Step 2: walk the CmdMapping table and look for a mapping
  HMCSimCmdMap *MapCmd = NULL;
  std::string name;
  for( auto itr = CmdMapping.begin(); itr != CmdMapping.end(); ++itr ){
    MapCmd = *itr;
    if( (MapCmd->getOpcode() == cmd) &&
        ((unsigned)(MapCmd->getSrcSize()) == numBytes) ){
      // found a positive map
      if( !HMCRqstToStr(MapCmd->getTargetType(), &name) ){
        std::string tStr;
        output->fatal(CALL_INFO, -1, "Unable to map hmc_rqst_t=%d to name\n", (int)(MapCmd->getTargetType()));
      }
      output->verbose(CALL_INFO, 8, 0,
                      "Found mapped request target of %s\n", name.c_str() );

      break;
    }
    MapCmd = NULL;
  }

  // -- if our MapCmd is NULL, then nothing was found
  if( MapCmd == NULL ){
    return false;
  }

  // Step 3: attempt to issue the request

  // Zero out the packet ready for us to populate it with values below
  zeroPacket(hmc_packet);

  output->verbose(CALL_INFO, 4, 0, "Queue front is: %" PRIu16 "\n", tag_queue.front());

  const uint8_t     req_dev  = 0;
  const uint16_t    req_tag  = (uint16_t) tag_queue.front();
  tag_queue.pop();

  hmc_rqst_t req_type = MapCmd->getTargetType();

  const uint8_t     req_link   = (uint8_t) (nextLink);
  nextLink = nextLink + 1;
  nextLink = (nextLink == hmc_link_count) ? 0 : nextLink;

  uint64_t          req_header = (uint64_t) 0;
  uint64_t          req_tail   = (uint64_t) 0;

  // -- determine if the request is posted
  bool isPosted = isPostedRqst(req_type);

  // -- check to see whether this is a read request
  bool isRead = isReadRqst( req_type );
  if( isPosted && isRead ){
    // can't have posted reads
    output->fatal(CALL_INFO, -1, "Mapped posted reads are illegal\n" );
  }

  output->verbose(CALL_INFO, 8, 0,
    "Issuing custom request: %" PRIu64 " cmd=%s tag: %" PRIu16 " dev: %" PRIu8 " link: %" PRIu8 "\n",
    addr, name.c_str(), req_tag, req_dev, req_dev);

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
    output->fatal(CALL_INFO, -1, "Unable to build a custom request for address: %" PRIu64 "\n", addr);
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
    output->verbose(CALL_INFO, 4, 0,
      "Issue of custom request for %s at address 0x%" PRIx64 " successfully accepted by HMC.\n",
      name.c_str(), addr);

    // Create the request entry which we keep in a table
    HMCSimBackEndReq* reqEntry = new HMCSimBackEndReq(reqId, addr,
                                                      getCurrentSimTimeNano(),
                                                      !isPosted);

    // Add the tag and request into our table of pending
    tag_req_map.insert( std::pair<uint16_t, HMCSimBackEndReq*>(req_tag, reqEntry) );

    // Record the I/O statistics
    if( (hmc_trace_level & HMC_TRACE_CMD) > 0 ){
      recordIOStats( hmc_packet[0] );
    }
  } else {
    output->fatal(CALL_INFO, -1,
      "Error issue request for %s at address %" PRIu64 " into HMC on link %" PRIu8 "\n",
      name.c_str(), addr, req_link);
  }

  return true;
}
bool GOBLINHMCSimBackend::issueRequest(ReqId reqId, Addr addr, bool isWrite,
                                       std::vector<uint64_t> ins, uint32_t flags,
                                       unsigned numBytes) {
	// We have run out of tags
	if(tag_queue.empty()) {
		output->verbose(CALL_INFO, 4, 0, "Will not issue request this call, tag queue has no free entries.\n");
		return false;
	}

	output->verbose(CALL_INFO, 8, 0, "Received request of size: %" PRIu32 " of type %s\n",
		numBytes, (isWrite ? "WRITE" : "READ"));

	// Zero out the packet ready for us to populate it with values below
	zeroPacket(hmc_packet);

	output->verbose(CALL_INFO, 4, 0, "Queue front is: %" PRIu16 "\n", tag_queue.front());

	const uint8_t		req_dev  = 0;
	const uint16_t          req_tag  = (uint16_t) tag_queue.front();
	tag_queue.pop();

	hmc_rqst_t       	req_type;

        // handle mapped commands
        if( CmdMapping.size() > 0 ){
          // we have mapped commands, check these first
          bool Success;
	  output->verbose(CALL_INFO, 4, 0, "Handling mapped requests\n");
          if( issueMappedRequest( reqId, addr, isWrite,
                                ins, flags,
                                numBytes, req_dev, req_tag,
                                &Success ) ){
            // the incoming command was mapped
            // return the 'Success' of the request
            return Success;
          }
        }

        // handle posted requests
        bool isPosted = false;
        if( (flags & MemEvent::F_NORESPONSE) > 0 ){
	  output->verbose(CALL_INFO, 4, 0, "Request is marked as posted\n");
          isPosted = true;
        }
        bool isRead = isReadRqst( req_type );
        if( isPosted && isRead ){
          // can't have posted reads
          output->fatal(CALL_INFO, -1, "Mapped posted reads are illegal\n" );
        }


	// Check if the request is for a read or write then transform this into something
	// for HMC simulator to use
	if( (isWrite) && (isPosted) ) {
                switch( numBytes ){
                case 8:
                case 16:
                  req_type = P_WR16;
                  break;
                case 32:
                  req_type = P_WR32;
                  break;
                case 48:
                  req_type = P_WR48;
                  break;
                case 64:
                  req_type = P_WR64;
                  break;
                case 80:
                  req_type = P_WR80;
                  break;
                case 96:
                  req_type = P_WR96;
                  break;
                case 112:
                  req_type = P_WR112;
                  break;
                case 128:
                  req_type = P_WR128;
                  break;
                case 256:
                  req_type = P_WR256;
                  break;
                default:
                  // flag an error
		  output->fatal(CALL_INFO, -1, "Unknown posted write request size: %d\n", numBytes );
                  break;
                }
        }else if(isWrite) {
                switch( numBytes ){
                case 8:
                case 16:
                  req_type = WR16;
                  break;
                case 32:
                  req_type = WR32;
                  break;
                case 48:
                  req_type = WR48;
                  break;
                case 64:
                  req_type = WR64;
                  break;
                case 80:
                  req_type = WR80;
                  break;
                case 96:
                  req_type = WR96;
                  break;
                case 112:
                  req_type = WR112;
                  break;
                case 128:
                  req_type = WR128;
                  break;
                case 256:
                  req_type = WR256;
                  break;
                default:
                  // flag an error
		  output->fatal(CALL_INFO, -1, "Unknown write request size: %d\n", numBytes );
                  break;
                }
	} else {
                switch( numBytes ){
                case 8:
                case 16:
                  req_type = RD16;
                  break;
                case 32:
                  req_type = RD32;
                  break;
                case 48:
                  req_type = RD48;
                  break;
                case 64:
                  req_type = RD64;
                  break;
                case 80:
                  req_type = RD80;
                  break;
                case 96:
                  req_type = RD96;
                  break;
                case 112:
                  req_type = RD112;
                  break;
                case 128:
                  req_type = RD128;
                  break;
                case 256:
                  req_type = RD256;
                  break;
                default:
                  // flag an error
		  output->fatal(CALL_INFO, -1, "Unknown read request size: %d\n", numBytes );
                  break;
                }
	}

	const uint8_t           req_link   = (uint8_t) (nextLink);
	nextLink = nextLink + 1;
	nextLink = (nextLink == hmc_link_count) ? 0 : nextLink;

	uint64_t                req_header = (uint64_t) 0;
	uint64_t                req_tail   = (uint64_t) 0;

	output->verbose(CALL_INFO, 8, 0, "Issuing request: %" PRIu64 " %s tag: %" PRIu16 " dev: %" PRIu8 " link: %" PRIu8 "\n",
		addr, (isWrite ? "WRITE" : "READ"), req_tag, req_dev, req_dev);

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

        std::string RqstStr;
        HMCRqstToStr( req_type, &RqstStr );

	if(HMC_STALL == rc) {
		output->verbose(CALL_INFO, 2, 0, "Issue revoked by HMC, reason: cube is stalled.\n");

		// Restore tag for use later, remember this request did not succeed
		tag_queue.push(req_tag);

		// Return false, the request was not issued so we must tell memory controller to give it
		// back to us when we are free
		return false;
	} else if(0 == rc) {
                if( isPosted ){
		  output->verbose(CALL_INFO, 4, 0, "Issue of posted request %s for address 0X%" PRIx64 " successfully accepted by HMC.\n", RqstStr.c_str(), addr);
                }else{
		  output->verbose(CALL_INFO, 4, 0, "Issue of request %s for address 0X%" PRIx64 " successfully accepted by HMC.\n", RqstStr.c_str(), addr);
                }

		// Create the request entry which we keep in a table
		HMCSimBackEndReq* reqEntry = new HMCSimBackEndReq(reqId, addr,
                                                                  getCurrentSimTimeNano(),
                                                                  !isPosted);

		// Add the tag and request into our table of pending
		tag_req_map.insert( std::pair<uint16_t, HMCSimBackEndReq*>(req_tag, reqEntry) );

                // Record the I/O statistics
                if( (hmc_trace_level & HMC_TRACE_CMD) > 0 ){
                  recordIOStats( hmc_packet[0] );
                }
	} else {
		output->fatal(CALL_INFO, -1, "Error issue request for address %" PRIu64 " into HMC on link %" PRIu8 "\n", addr, req_link);
	}

	return true;
}

void GOBLINHMCSimBackend::setup() {

}

void GOBLINHMCSimBackend::finish() {

}

void GOBLINHMCSimBackend::collectStats(){
#if defined( HMC_STAT )
  // stall stats
  xbar_rqst_stall_stat->addData(hmcsim_int_stat(&the_hmc, XBAR_RQST_STALL_STAT));
  xbar_rsp_stall_stat->addData(hmcsim_int_stat(&the_hmc, XBAR_RSP_STALL_STAT));
  vault_rqst_stall_stat->addData(hmcsim_int_stat(&the_hmc, VAULT_RQST_STALL_STAT));
  vault_rsp_stall_stat->addData(hmcsim_int_stat(&the_hmc, VAULT_RSP_STALL_STAT));
  route_rqst_stall_stat->addData(hmcsim_int_stat(&the_hmc, ROUTE_RQST_STALL_STAT));
  route_rsp_stall_stat->addData(hmcsim_int_stat(&the_hmc, ROUTE_RSP_STALL_STAT));
  undef_stall_stat->addData(hmcsim_int_stat(&the_hmc, UNDEF_STALL_STAT));
  bank_conflict_stat->addData(hmcsim_int_stat(&the_hmc, BANK_CONFLICT_STAT));
  xbar_latency_stat->addData(hmcsim_int_stat(&the_hmc, XBAR_LATENCY_STAT));

  // power & thermal stats
  link_phy_power_stat->addData(hmcsim_float_stat(&the_hmc, LINK_PHY_POWER_STAT)-s_link_phy_power);
  link_local_route_power_stat->addData(hmcsim_float_stat(&the_hmc, LINK_LOCAL_ROUTE_POWER_STAT)-s_link_local_route_power);
  link_remote_route_power_stat->addData(hmcsim_float_stat(&the_hmc, LINK_REMOTE_ROUTE_POWER_STAT)-s_link_remote_route_power);
  xbar_rqst_slot_power_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_RQST_SLOT_POWER_STAT)-s_xbar_rqst_slot_power);
  xbar_rsp_slot_power_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_RSP_SLOT_POWER_STAT)-s_xbar_rsp_slot_power);
  xbar_route_extern_power_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_ROUTE_EXTERN_POWER_STAT)-s_xbar_route_extern_power);
  vault_rqst_slot_power_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_RQST_SLOT_POWER_STAT)-s_vault_rqst_slot_power);
  vault_rsp_slot_power_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_RSP_SLOT_POWER_STAT)-s_vault_rsp_slot_power);
  vault_ctrl_power_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_CTRL_POWER_STAT)-s_vault_ctrl_power);
  row_access_power_stat->addData(hmcsim_float_stat(&the_hmc, ROW_ACCESS_POWER_STAT)-s_row_access_power);

  link_phy_therm_stat->addData(hmcsim_float_stat(&the_hmc, LINK_PHY_THERM_STAT)-s_link_phy_therm);
  link_local_route_therm_stat->addData(hmcsim_float_stat(&the_hmc, LINK_LOCAL_ROUTE_THERM_STAT)-s_link_local_route_therm);
  link_remote_route_therm_stat->addData(hmcsim_float_stat(&the_hmc, LINK_REMOTE_ROUTE_THERM_STAT)-s_link_remote_route_therm);
  xbar_rqst_slot_therm_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_RQST_SLOT_THERM_STAT)-s_xbar_rqst_slot_therm);
  xbar_rsp_slot_therm_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_RSP_SLOT_THERM_STAT)-s_xbar_rsp_slot_therm);
  xbar_route_extern_therm_stat->addData(hmcsim_float_stat(&the_hmc, XBAR_ROUTE_EXTERN_THERM_STAT)-s_xbar_route_extern_therm);
  vault_rqst_slot_therm_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_RQST_SLOT_THERM_STAT)-s_vault_rqst_slot_therm);
  vault_rsp_slot_therm_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_RSP_SLOT_THERM_STAT)-s_vault_rsp_slot_therm);
  vault_ctrl_therm_stat->addData(hmcsim_float_stat(&the_hmc, VAULT_CTRL_THERM_STAT)-s_vault_ctrl_therm);
  row_access_therm_stat->addData(hmcsim_float_stat(&the_hmc, ROW_ACCESS_THERM_STAT)-s_row_access_therm);

  s_link_phy_power = hmcsim_float_stat(&the_hmc, LINK_PHY_POWER_STAT);
  s_link_local_route_power = hmcsim_float_stat(&the_hmc, LINK_LOCAL_ROUTE_POWER_STAT);
  s_link_remote_route_power = hmcsim_float_stat(&the_hmc, LINK_REMOTE_ROUTE_POWER_STAT);
  s_xbar_rqst_slot_power = hmcsim_float_stat(&the_hmc, XBAR_RQST_SLOT_POWER_STAT);
  s_xbar_rsp_slot_power = hmcsim_float_stat(&the_hmc, XBAR_RSP_SLOT_POWER_STAT);
  s_xbar_route_extern_power = hmcsim_float_stat(&the_hmc, XBAR_ROUTE_EXTERN_POWER_STAT);
  s_vault_rqst_slot_power = hmcsim_float_stat(&the_hmc, VAULT_RQST_SLOT_POWER_STAT);
  s_vault_rsp_slot_power = hmcsim_float_stat(&the_hmc, VAULT_RSP_SLOT_POWER_STAT);
  s_vault_ctrl_power = hmcsim_float_stat(&the_hmc, VAULT_CTRL_POWER_STAT);
  s_row_access_power = hmcsim_float_stat(&the_hmc, ROW_ACCESS_POWER_STAT);

  s_link_phy_therm = hmcsim_float_stat(&the_hmc, LINK_PHY_THERM_STAT);
  s_link_local_route_therm = hmcsim_float_stat(&the_hmc, LINK_LOCAL_ROUTE_THERM_STAT);
  s_link_remote_route_therm = hmcsim_float_stat(&the_hmc, LINK_REMOTE_ROUTE_THERM_STAT);
  s_xbar_rqst_slot_therm = hmcsim_float_stat(&the_hmc, XBAR_RQST_SLOT_THERM_STAT);
  s_xbar_rsp_slot_therm = hmcsim_float_stat(&the_hmc, XBAR_RSP_SLOT_THERM_STAT);
  s_xbar_route_extern_therm = hmcsim_float_stat(&the_hmc, XBAR_ROUTE_EXTERN_THERM_STAT);
  s_vault_rqst_slot_therm = hmcsim_float_stat(&the_hmc, VAULT_RQST_SLOT_THERM_STAT);
  s_vault_rsp_slot_therm = hmcsim_float_stat(&the_hmc, VAULT_RSP_SLOT_THERM_STAT);
  s_vault_ctrl_therm = hmcsim_float_stat(&the_hmc, VAULT_CTRL_THERM_STAT);
  s_row_access_therm = hmcsim_float_stat(&the_hmc, ROW_ACCESS_THERM_STAT);

#endif
}

bool GOBLINHMCSimBackend::clock(Cycle_t cycle) {
	output->verbose(CALL_INFO, 8, 0, "Clocking HMC...\n");
	int rc = hmcsim_clock(&the_hmc);

	if(rc > 0) {
		output->fatal(CALL_INFO, -1, "Error: clock call to the HMC failed.\n");
	}

#if defined( HMC_STAT )
        collectStats();
#endif

	// Call to process any responses from the HMC
	processResponses();
        return false;
}

void GOBLINHMCSimBackend::processResponses() {
	int rc = HMC_OK;
        uint32_t flags = 0;

	printPendingRequests();

	for(uint32_t i = 0; i < (uint32_t) the_hmc.num_links; ++i) {
		output->verbose(CALL_INFO, 6, 0, "Polling responses on link %d...\n", i);

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

						output->verbose(CALL_INFO, 4, 0, "Matched tag %" PRIu16 " to request for address: 0x%" PRIx64 ", processing time: %" PRIu64 "ns\n",
							resp_tag, matchedReq->getAddr(),
							getCurrentSimTimeNano() - matchedReq->getStartTime());

						// Pass back to the controller to be handled, HMC sim is finished with it
						handleMemResponse(matchedReq->getRequest(),flags);

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

        // handle all the posted requests
        std::map<uint16_t, HMCSimBackEndReq*>::iterator it;
        for(it=tag_req_map.begin(); it!=tag_req_map.end();){
          uint16_t resp_tag = it->first;
          HMCSimBackEndReq* matchedReq = it->second;
          if( !matchedReq->hasResponse() ){
            // i am a posted request
	    output->verbose(CALL_INFO, 4, 0, "Handling posted memory response for tag: %" PRIu16 "\n", resp_tag);
            handleMemResponse(matchedReq->getRequest(),flags);
            it = tag_req_map.erase(it);
            tag_queue.push(resp_tag);
            delete matchedReq;
          } else {
            it++;
          }
        }
}

GOBLINHMCSimBackend::~GOBLINHMCSimBackend() {
	output->verbose(CALL_INFO, 1, 0, "Freeing the HMC resources...\n");
	hmcsim_free(&the_hmc);

	output->verbose(CALL_INFO, 1, 0, "Closing HMC trace file...\n");
	fclose(hmc_trace_file_handle);

	output->verbose(CALL_INFO, 1, 0, "Completed.\n");

        // free the map list
        for( auto itr = CmdMapping.begin(); itr != CmdMapping.end(); ++itr ){
          HMCSimCmdMap *c = *itr;
          delete c;
        }

        // free the cmc command list
        for( auto itr = CmcConfig.begin(); itr != CmcConfig.end(); ++itr ){
          HMCCMCConfig *c = *itr;
          delete c;
        }
}

void GOBLINHMCSimBackend::registerStatistics() {
        // I/O Ops
        Write16Ops = registerStatistic<uint64_t>("WR16");
        Write32Ops = registerStatistic<uint64_t>("WR32");
        Write48Ops = registerStatistic<uint64_t>("WR48");
        Write64Ops = registerStatistic<uint64_t>("WR64");
        Write80Ops = registerStatistic<uint64_t>("WR80");
        Write96Ops = registerStatistic<uint64_t>("WR96");
        Write112Ops = registerStatistic<uint64_t>("WR112");
        Write128Ops = registerStatistic<uint64_t>("WR128");
        Write256Ops = registerStatistic<uint64_t>("WR256");
        Read16Ops = registerStatistic<uint64_t>("RD16");
        Read32Ops = registerStatistic<uint64_t>("RD32");
        Read48Ops = registerStatistic<uint64_t>("RD48");
        Read64Ops = registerStatistic<uint64_t>("RD64");
        Read80Ops = registerStatistic<uint64_t>("RD80");
        Read96Ops = registerStatistic<uint64_t>("RD96");
        Read112Ops = registerStatistic<uint64_t>("RD112");
        Read128Ops = registerStatistic<uint64_t>("RD128");
        Read256Ops = registerStatistic<uint64_t>("RD256");
        ModeWriteOps = registerStatistic<uint64_t>("MD_WR");
        ModeReadOps = registerStatistic<uint64_t>("MD_RD");
        BWROps = registerStatistic<uint64_t>("BWR");
        TwoAdd8Ops = registerStatistic<uint64_t>("2ADD8");
        Add16Ops = registerStatistic<uint64_t>("ADD16");
        PWrite16Ops = registerStatistic<uint64_t>("P_WR16");
        PWrite32Ops = registerStatistic<uint64_t>("P_WR32");
        PWrite48Ops = registerStatistic<uint64_t>("P_WR48");
        PWrite64Ops = registerStatistic<uint64_t>("P_WR64");
        PWrite80Ops = registerStatistic<uint64_t>("P_WR80");
        PWrite96Ops = registerStatistic<uint64_t>("P_WR96");
        PWrite112Ops = registerStatistic<uint64_t>("P_WR112");
        PWrite128Ops = registerStatistic<uint64_t>("P_WR128");
        PWrite256Ops = registerStatistic<uint64_t>("P_WR256");
        TwoAddS8ROps = registerStatistic<uint64_t>("2ADDS8R");
        AddS16ROps = registerStatistic<uint64_t>("ADDS16");
        Inc8Ops = registerStatistic<uint64_t>("INC8");
        PInc8Ops = registerStatistic<uint64_t>("P_INC8");
        Xor16Ops = registerStatistic<uint64_t>("XOR16");
        Or16Ops = registerStatistic<uint64_t>("OR16");
        Nor16Ops = registerStatistic<uint64_t>("NOR16");
        And16Ops = registerStatistic<uint64_t>("AND16");
        Nand16Ops = registerStatistic<uint64_t>("NAND16");
        CasGT8Ops = registerStatistic<uint64_t>("CASGT8");
        CasGT16Ops = registerStatistic<uint64_t>("CASGT16");
        CasLT8Ops = registerStatistic<uint64_t>("CASLT8");
        CasLT16Ops = registerStatistic<uint64_t>("CASLT16");
        CasEQ8Ops = registerStatistic<uint64_t>("CASEQ8");
        CasZero16Ops = registerStatistic<uint64_t>("CASZERO16");
        Eq8Ops = registerStatistic<uint64_t>("EQ8");
        Eq16Ops = registerStatistic<uint64_t>("EQ16");
        BWR8ROps = registerStatistic<uint64_t>("BWR8R");
        Swap16Ops = registerStatistic<uint64_t>("SWAP16");

        // Stall Stats
        xbar_rqst_stall_stat = registerStatistic<uint64_t>("XbarRqstStall");
        xbar_rsp_stall_stat = registerStatistic<uint64_t>("XbarRspStall");
        vault_rqst_stall_stat = registerStatistic<uint64_t>("VaultRqstStall");
        vault_rsp_stall_stat = registerStatistic<uint64_t>("VaultRspStall");
        route_rqst_stall_stat = registerStatistic<uint64_t>("RouteRqstStall");
        route_rsp_stall_stat = registerStatistic<uint64_t>("RouteRspStall");
        undef_stall_stat = registerStatistic<uint64_t>("UndefStall");
        bank_conflict_stat = registerStatistic<uint64_t>("BankConflict");
        xbar_latency_stat = registerStatistic<uint64_t>("XbarLatency");

        // Power & Thermal Stats
        link_phy_power_stat = registerStatistic<float>("LinkPhyPower");
        link_local_route_power_stat = registerStatistic<float>("LinkLocalRoutePower");
        link_remote_route_power_stat = registerStatistic<float>("LinkRemoteRoutePower");
        xbar_rqst_slot_power_stat = registerStatistic<float>("XbarRqstSlotPower");
        xbar_rsp_slot_power_stat = registerStatistic<float>("XbarRspSlotPower");
        xbar_route_extern_power_stat = registerStatistic<float>("XbarRouteExternPower");
        vault_rqst_slot_power_stat = registerStatistic<float>("VaultRqstSlotPower");
        vault_rsp_slot_power_stat = registerStatistic<float>("VaultRspSlotPower");
        vault_ctrl_power_stat = registerStatistic<float>("VaultCtrlPower");
        row_access_power_stat = registerStatistic<float>("RowAccessPower");
        link_phy_therm_stat = registerStatistic<float>("LinkPhyTherm");
        link_local_route_therm_stat = registerStatistic<float>("LinkLocalRouteTherm");
        link_remote_route_therm_stat = registerStatistic<float>("LinkRemoteRouteTherm");
        xbar_rqst_slot_therm_stat = registerStatistic<float>("XbarRqstSlotTherm");
        xbar_rsp_slot_therm_stat = registerStatistic<float>("XbarRspSlotTherm");
        xbar_route_extern_therm_stat = registerStatistic<float>("XbarRouteExternTherm");
        vault_rqst_slot_therm_stat = registerStatistic<float>("VaultRqstSlotTherm");
        vault_rsp_slot_therm_stat = registerStatistic<float>("VaultRspSlotTherm");
        vault_ctrl_therm_stat = registerStatistic<float>("VaultCtrlTherm");
        row_access_therm_stat = registerStatistic<float>("RowAccessTherm");
}

void GOBLINHMCSimBackend::recordIOStats( uint64_t header ){
        uint32_t cmd = (uint32_t)(header & 0x7F);

        switch( cmd ){
        case 8:
          // WR16
          Write16Ops->addData(1);
          break;
        case 9:
          // WR32
          Write32Ops->addData(1);
          break;
        case 10:
          // WR48
          Write48Ops->addData(1);
          break;
        case 11:
          // WR64
          Write64Ops->addData(1);
          break;
        case 12:
          // WR80
          Write80Ops->addData(1);
          break;
        case 13:
          // WR96
          Write96Ops->addData(1);
          break;
        case 14:
          // WR112
          Write112Ops->addData(1);
          break;
        case 15:
          // WR128
          Write128Ops->addData(1);
          break;
        case 79:
          // WR256
          Write256Ops->addData(1);
          break;
        case 16:
          // MD_WR
          ModeWriteOps->addData(1);
          break;
        case 17:
          // BWR
          BWROps->addData(1);
          break;
        case 18:
          // TWOADD8
          TwoAdd8Ops->addData(1);
          break;
        case 19:
          // ADD16
          Add16Ops->addData(1);
          break;
        case 24:
          // P_WR16
          PWrite16Ops->addData(1);
          break;
        case 25:
          // P_WR32
          PWrite32Ops->addData(1);
          break;
        case 26:
          // P_WR48
          PWrite48Ops->addData(1);
          break;
        case 27:
          // P_WR64
          PWrite64Ops->addData(1);
          break;
        case 28:
          // P_WR80
          PWrite80Ops->addData(1);
          break;
        case 29:
          // P_WR96
          PWrite96Ops->addData(1);
          break;
        case 30:
          // P_WR112
          PWrite112Ops->addData(1);
          break;
        case 31:
          // P_WR128
          PWrite128Ops->addData(1);
          break;
        case 95:
          // P_WR256
          PWrite256Ops->addData(1);
          break;
        case 33:
          // P_BWR
          PBWROps->addData(1);
          break;
        case 34:
          // P2ADD8
          P2ADD8Ops->addData(1);
          break;
        case 35:
          // P2ADD16
          P2ADD16Ops->addData(1);
          break;
        case 48:
          // RD16
          Read16Ops->addData(1);
          break;
        case 49:
          // RD32
          Read32Ops->addData(1);
          break;
        case 50:
          // RD48
          Read48Ops->addData(1);
          break;
        case 51:
          // RD64
          Read64Ops->addData(1);
          break;
        case 52:
          // RD80
          Read80Ops->addData(1);
          break;
        case 53:
          // RD96
          Read96Ops->addData(1);
          break;
        case 54:
          // RD112
          Read112Ops->addData(1);
          break;
        case 55:
          // RD128
          Read128Ops->addData(1);
          break;
        case 119:
          // RD256
          Read256Ops->addData(1);
          break;
        case 40:
          // MD_RD
          ModeReadOps->addData(1);
          break;
        case 82:
          // 2ADDS8R
          TwoAddS8ROps->addData(1);
          break;
        case 83:
          // ADDS16R
          AddS16ROps->addData(1);
          break;
        case 80:
          // INC8
          Inc8Ops->addData(1);
          break;
        case 84:
          // P_INC8
          PInc8Ops->addData(1);
          break;
        case 64:
          // XOR16
          Xor16Ops->addData(1);
          break;
        case 65:
          // OR16
          Or16Ops->addData(1);
          break;
        case 66:
          // NOR16
          Nor16Ops->addData(1);
          break;
        case 67:
          // AND16
          And16Ops->addData(1);
          break;
        case 68:
          // NAND16
          Nand16Ops->addData(1);
          break;
        case 96:
          // CASGT8
          CasGT8Ops->addData(1);
          break;
        case 98:
          // CASGT16
          CasGT16Ops->addData(1);
          break;
        case 97:
          // CASLT8
          CasLT8Ops->addData(1);
          break;
        case 99:
          // CASLT16
          CasLT16Ops->addData(1);
          break;
        case 100:
          // CASEQ8
          CasEQ8Ops->addData(1);
          break;
        case 101:
          // CASZERO16
          CasZero16Ops->addData(1);
          break;
        case 105:
          // EQ8
          Eq8Ops->addData(1);
          break;
        case 104:
          // EQ16
          Eq16Ops->addData(1);
          break;
        case 81:
          // BWR8R
          BWR8ROps->addData(1);
          break;
        case 106:
          // SWAP16
          Swap16Ops->addData(1);
          break;
        default:
          break;
        }
}

void GOBLINHMCSimBackend::zeroPacket(uint64_t* packet) const {
	for(int i = 0; i < HMC_MAX_UQ_PACKET; ++i) {
		packet[i] = (uint64_t) 0;
	}
}

void GOBLINHMCSimBackend::printPendingRequests() {
	output->verbose(CALL_INFO, 8, 0, "Pending requests:\n");

	std::map<uint16_t, HMCSimBackEndReq*>::iterator pending_itr;

	for(pending_itr = tag_req_map.begin(); pending_itr != tag_req_map.end(); pending_itr++) {
		output->verbose(CALL_INFO, 8, 0, "Tag: %8" PRIu16 " for address 0X%" PRIx64 "\n",
			pending_itr->first, pending_itr->second->getAddr());
	}
}
