// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
** $Id: sst_gen.c,v 1.13 2010/05/13 19:27:23 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>		/* For PRId64 */
#include "sst_gen_v2.h"
#include "machine.h"
#include "pattern.h"
#include "farlink.h"
#include "gen.h"

#define MAX_ID_LEN		(256)



void
sst_header(FILE *sstfile, int partition)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<?xml version=\"1.0\"?>\n");
    fprintf(sstfile, "<sdl version=\"2.0\"/>\n");
    fprintf(sstfile, "\n");

    /*
    ** For now we do the config section here. This may have to be
    ** broken out later, if we actually start config values.
    */
    fprintf(sstfile, "<config>\n");
    fprintf(sstfile, "\trun-mode=both\n");
    if (partition)   {
	fprintf(sstfile, "\tpartitioner=self\n");
    }
    fprintf(sstfile, "</config>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_header() */



void
sst_variables(FILE *sstfile, uint64_t IO_latency)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<variables>\n");
    fprintf(sstfile, "\t<lat_local_net> %" PRId64 "ns </lat_local_net>\n", NoCLinkLatency() / 2);
    fprintf(sstfile, "\t<lat_global_net> %" PRId64 "ns </lat_global_net>\n", NetLinkLatency() / 2);
    fprintf(sstfile, "\t<lat_local_nvram> %" PRId64 "ns </lat_local_nvram>\n", IO_latency);
    fprintf(sstfile, "\t<lat_storage_net> %" PRId64 "ns </lat_storage_net>\n", IO_latency);
    fprintf(sstfile, "\t<lat_storage_nvram> %" PRId64 "ns </lat_storage_nvram>\n", IO_latency);
    fprintf(sstfile, "\t<lat_ssd_io> %" PRId64 "ns </lat_ssd_io>\n", IO_latency);
    fprintf(sstfile, "</variables>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_variables() */



void
sst_param_start(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "<param_include>\n");
    }

}  /* end of sst_param_start() */



void
sst_param_end(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "</param_include>\n\n");
    }

}  /* end of sst_param_end() */



void
sst_gen_param_start(FILE *sstfile, int gen_debug)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t<Gp>\n");
    fprintf(sstfile, "\t\t<debug> %d </debug>\n", gen_debug);

}  /* end of sst_gen_param_start() */


void
sst_gen_param_entries(FILE *sstfile)
{

int i;


    if (sstfile == NULL)   {
	return;
    }

    /* Common parameters */
    fprintf(sstfile, "\t\t<Net_x_dim> %d </Net_x_dim>\n", Net_x_dim());
    fprintf(sstfile, "\t\t<Net_y_dim> %d </Net_y_dim>\n", Net_y_dim());
    fprintf(sstfile, "\t\t<Net_z_dim> %d </Net_z_dim>\n", Net_z_dim());
    fprintf(sstfile, "\t\t<NoC_x_dim> %d </NoC_x_dim>\n", NoC_x_dim());
    fprintf(sstfile, "\t\t<NoC_y_dim> %d </NoC_y_dim>\n", NoC_y_dim());
    fprintf(sstfile, "\t\t<NoC_z_dim> %d </NoC_z_dim>\n", NoC_z_dim());
    fprintf(sstfile, "\t\t<NetXwrap> %d </NetXwrap>\n", Net_x_wrap());
    fprintf(sstfile, "\t\t<NetYwrap> %d </NetYwrap>\n", Net_y_wrap());
    fprintf(sstfile, "\t\t<NetZwrap> %d </NetZwrap>\n", Net_z_wrap());
    fprintf(sstfile, "\t\t<NoCXwrap> %d </NoCXwrap>\n", NoC_x_wrap());
    fprintf(sstfile, "\t\t<NoCYwrap> %d </NoCYwrap>\n", NoC_y_wrap());
    fprintf(sstfile, "\t\t<NoCZwrap> %d </NoCZwrap>\n", NoC_z_wrap());
    fprintf(sstfile, "\t\t<cores> %d </cores>\n", num_router_cores());
    fprintf(sstfile, "\t\t<nodes> %d </nodes>\n", num_router_nodes());

    fprintf(sstfile, "\t\t<NetNICgap> %d </NetNICgap>\n", NetNICgap());
    fprintf(sstfile, "\t\t<NoCNICgap> %d </NoCNICgap>\n", NoCNICgap());

    fprintf(sstfile, "\t\t<NetLinkBandwidth> %" PRId64 " </NetLinkBandwidth>\n", NetLinkBandwidth());
    fprintf(sstfile, "\t\t<NetLinkLatency> %" PRId64 " </NetLinkLatency>\n", NetLinkLatency());
    fprintf(sstfile, "\t\t<NoCLinkBandwidth> %" PRId64 " </NoCLinkBandwidth>\n", NoCLinkBandwidth());
    fprintf(sstfile, "\t\t<NoCLinkLatency> %" PRId64 " </NoCLinkLatency>\n", NoCLinkLatency());
    fprintf(sstfile, "\t\t<IOLinkBandwidth> %" PRId64 " </IOLinkBandwidth>\n", IOLinkBandwidth());
    fprintf(sstfile, "\t\t<IOLinkLatency> %" PRId64 " </IOLinkLatency>\n", IOLinkLatency());


    for (i= 0; i < NetNICinflections(); i++)   {
	fprintf(sstfile, "\t\t<NetNICinflection%d> %d </NetNICinflection%d>\n",
	    i, NetNICinflectionpoint(i), i);
	fprintf(sstfile, "\t\t<NetNIClatency%d> %" PRId64 " </NetNIClatency%d>\n",
	    i, NetNIClatency(i), i);
    }

    for (i= 0; i < NoCNICinflections(); i++)   {
	fprintf(sstfile, "\t\t<NoCNICinflection%d> %d </NoCNICinflection%d>\n",
	    i, NoCNICinflectionpoint(i), i);
	fprintf(sstfile, "\t\t<NoCNIClatency%d> %" PRId64 " </NoCNIClatency%d>\n",
	    i, NoCNIClatency(i), i);
    }

    machine_params(sstfile);
    farlink_params(sstfile, num_router_cores() + 1);
    pattern_params(sstfile);

}  /* end of sst_gen_param_entries() */


void
sst_gen_param_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t</Gp>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_gen_param_end() */


void
sst_pwr_param_entries(FILE *sstfile, pwr_method_t power_method)
{
    if (sstfile == NULL)   {
	return;
    }

    if (power_method == pwrNone)   {
	/* Nothing to do */
	return;
    } else if (power_method == pwrMcPAT)   {
	fprintf(sstfile, "\t<intro1_params>\n");
	fprintf(sstfile, "\t\t<period>15000000ns</period>\n");
	fprintf(sstfile, "\t\t<model>routermodel_power</model>\n");
	fprintf(sstfile, "\t</intro1_params>\n");
	fprintf(sstfile, "\n");
    } else if (power_method == pwrORION)   {
	fprintf(sstfile, "\t<intro1_params>\n");
	fprintf(sstfile, "\t\t<period>15000000ns</period>\n");
	fprintf(sstfile, "\t\t<model>routermodel_power</model>\n");
	fprintf(sstfile, "\t</intro1_params>\n");
	fprintf(sstfile, "\n");
    } else   {
	/* error */
    }

}  /* end of sst_pwr_param_entries() */


void
sst_nvram_param_entries(FILE *sstfile, int nvram_read_bw, int nvram_write_bw,
	int ssd_read_bw, int ssd_write_bw)
{
    if (sstfile == NULL)   {
	return;
    }

    fprintf(sstfile, "\t<NVRAMparams>\n");
    fprintf(sstfile, "\t\t<debug> 0 </debug>\n");
    fprintf(sstfile, "\t\t<read_bw> %d </read_bw>\n", nvram_read_bw);
    fprintf(sstfile, "\t\t<write_bw> %d </write_bw>\n", nvram_write_bw);
    fprintf(sstfile, "\t</NVRAMparams>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "\t<SSDparams>\n");
    fprintf(sstfile, "\t\t<debug> 0 </debug>\n");
    fprintf(sstfile, "\t\t<read_bw> %d </read_bw>\n", ssd_read_bw);
    fprintf(sstfile, "\t\t<write_bw> %d </write_bw>\n", ssd_write_bw);
    fprintf(sstfile, "\t</SSDparams>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nvram_param_entries() */


void
sst_router_param_start(FILE *sstfile, char *Rname, int num_ports, int num_router_cores,
    int wormhole, pwr_method_t power_method)
{

int aggregator;
int i;


    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t<%s>\n", Rname);
    if (num_ports >= 0)   {
	fprintf(sstfile, "\t\t<num_ports> %d </num_ports>\n", num_ports);
    }
    fprintf(sstfile, "\t\t<debug> 0 </debug>\n");
    /* FIXME: This could be done more efficiently w/o a string compare */
    if (strcmp(Rname, RNAME_NETWORK) == 0)   {
	aggregator= 0;
	for (i= 0; i < NetRtrinflections(); i++)   {
	    fprintf(sstfile, "\t\t<Rtrinflection%d> %d </Rtrinflection%d>\n",
		i, NetRtrinflectionpoint(i), i);
	    fprintf(sstfile, "\t\t<Rtrlatency%d> %" PRId64 " </Rtrlatency%d>\n",
		i, NetRtrlatency(i), i);
	}

	/*
	** The routers need the NICinflection values as well. It's
	** basically the bandwidth of the links between routers
	*/
	for (i= 0; i < NetNICinflections(); i++)   {
	    fprintf(sstfile, "\t\t<NICinflection%d> %d </NICinflection%d>\n",
		i, NetNICinflectionpoint(i), i);
	    fprintf(sstfile, "\t\t<NIClatency%d> %" PRId64 " </NIClatency%d>\n",
		i, NetNIClatency(i), i);
	}
    }

    if (strcmp(Rname, RNAME_NoC) == 0)   {
	aggregator= 0;
	for (i= 0; i < NoCRtrinflections(); i++)   {
	    fprintf(sstfile, "\t\t<Rtrinflection%d> %d </Rtrinflection%d>\n",
		i, NoCRtrinflectionpoint(i), i);
	    fprintf(sstfile, "\t\t<Rtrlatency%d> %" PRId64 " </Rtrlatency%d>\n",
		i, NoCRtrlatency(i), i);
	}

	for (i= 0; i < NoCNICinflections(); i++)   {
	    fprintf(sstfile, "\t\t<NICinflection%d> %d </NICinflection%d>\n",
		i, NoCNICinflectionpoint(i), i);
	    fprintf(sstfile, "\t\t<NIClatency%d> %" PRId64 " </NIClatency%d>\n",
		i, NoCNIClatency(i), i);
	}
    }

    if (strcmp(Rname, RNAME_NET_ACCESS) == 0)   {
	aggregator= 1;
    }
    if (strcmp(Rname, RNAME_NVRAM) == 0)   {
	aggregator= 1;
    }
    if (strcmp(Rname, RNAME_STORAGE) == 0)   {
	aggregator= 1;
    }
    if (strcmp(Rname, RNAME_IO) == 0)   {
	aggregator= 1;
    }



    if (!aggregator)   {
	fprintf(sstfile, "\t\t<wormhole> %d </wormhole>\n", wormhole);
    } else   {
	fprintf(sstfile, "\t\t<aggregator> 1 </aggregator>\n");
    }

    if (power_method == pwrNone)   {
	/* Nothing to do */
    } else if (power_method == pwrMcPAT)   {
	fprintf(sstfile, "\t\t<McPAT_XMLfile>../../core/techModels/libMcPATbeta/Niagara1.xml</McPAT_XMLfile>\n");
	fprintf(sstfile, "\t\t<core_temperature>350</core_temperature>\n");
	fprintf(sstfile, "\t\t<core_tech_node>65</core_tech_node>\n");
	fprintf(sstfile, "\t\t<frequency>1ns</frequency>\n");
	fprintf(sstfile, "\t\t<power_monitor>YES</power_monitor>\n");
	fprintf(sstfile, "\t\t<temperature_monitor>NO</temperature_monitor>\n");
	fprintf(sstfile, "\t\t<router_voltage>1.1</router_voltage>\n");
	fprintf(sstfile, "\t\t<router_clock_rate>1000000000</router_clock_rate>\n");
	fprintf(sstfile, "\t\t<router_flit_bits>64</router_flit_bits>\n");
	fprintf(sstfile, "\t\t<router_virtual_channel_per_port>2</router_virtual_channel_per_port>\n");
	fprintf(sstfile, "\t\t<router_input_ports>%d</router_input_ports>\n", num_ports);
	fprintf(sstfile, "\t\t<router_output_ports>%d</router_output_ports>\n", num_ports);
	fprintf(sstfile, "\t\t<router_link_length>16691</router_link_length>\n");
	fprintf(sstfile, "\t\t<router_power_model>McPAT</router_power_model>\n");
	fprintf(sstfile, "\t\t<router_has_global_link>1</router_has_global_link>\n");
	fprintf(sstfile, "\t\t<router_input_buffer_entries_per_vc>16</router_input_buffer_entries_per_vc>\n");
	fprintf(sstfile, "\t\t<router_link_throughput>1</router_link_throughput>\n");
	fprintf(sstfile, "\t\t<router_link_latency>1</router_link_latency>\n");
	fprintf(sstfile, "\t\t<router_horizontal_nodes>1</router_horizontal_nodes>\n");
	fprintf(sstfile, "\t\t<router_vertical_nodes>1</router_vertical_nodes>\n");
	fprintf(sstfile, "\t\t<router_topology>RING</router_topology>\n");
	fprintf(sstfile, "\t\t<core_number_of_NoCs>%d</core_number_of_NoCs>\n", num_router_cores);
	fprintf(sstfile, "\t\t<push_introspector>routerIntrospector</push_introspector>\n");

    } else if (power_method == pwrORION)   {
	fprintf(sstfile, "\t\t<ORION_XMLfile>../../core/techModels/libORION/something.xml</ORION_XMLfile>\n");
	fprintf(sstfile, "\t\t<core_temperature>350</core_temperature>\n");
	fprintf(sstfile, "\t\t<core_tech_node>65</core_tech_node>\n");
	fprintf(sstfile, "\t\t<frequency>1ns</frequency>\n");
	fprintf(sstfile, "\t\t<power_monitor>YES</power_monitor>\n");
	fprintf(sstfile, "\t\t<temperature_monitor>NO</temperature_monitor>\n");
	fprintf(sstfile, "\t\t<router_voltage>1.1</router_voltage>\n");
	fprintf(sstfile, "\t\t<router_clock_rate>1000000000</router_clock_rate>\n");
	fprintf(sstfile, "\t\t<router_flit_bits>64</router_flit_bits>\n");
	fprintf(sstfile, "\t\t<router_virtual_channel_per_port>2</router_virtual_channel_per_port>\n");
	fprintf(sstfile, "\t\t<router_input_ports>%d</router_input_ports>\n", num_ports);
	fprintf(sstfile, "\t\t<router_output_ports>%d</router_output_ports>\n", num_ports);
	fprintf(sstfile, "\t\t<router_link_length>16691</router_link_length>\n");
	fprintf(sstfile, "\t\t<router_power_model>ORION</router_power_model>\n");
	fprintf(sstfile, "\t\t<router_has_global_link>1</router_has_global_link>\n");
	fprintf(sstfile, "\t\t<router_input_buffer_entries_per_vc>16</router_input_buffer_entries_per_vc>\n");
	fprintf(sstfile, "\t\t<router_link_throughput>1</router_link_throughput>\n");
	fprintf(sstfile, "\t\t<router_link_latency>1</router_link_latency>\n");
	fprintf(sstfile, "\t\t<router_horizontal_nodes>1</router_horizontal_nodes>\n");
	fprintf(sstfile, "\t\t<router_vertical_nodes>1</router_vertical_nodes>\n");
	fprintf(sstfile, "\t\t<router_topology>RING</router_topology>\n");
	fprintf(sstfile, "\t\t<core_number_of_NoCs>%d</core_number_of_NoCs>\n", num_router_cores);
	fprintf(sstfile, "\t\t<push_introspector>routerIntrospector</push_introspector>\n");

    } else   {
	/* error */
    }

}  /* end of sst_router_param_start() */



void
sst_router_param_end(FILE *sstfile, char *Rname)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t</%s>\n", Rname);
    fprintf(sstfile, "\n");

}  /* end of sst_router_param_end() */



void
sst_body_start(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "<sst>\n");
    }

}  /* end of sst_body_start() */



void
sst_body_end(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "</sst>\n");
    }

}  /* end of sst_body_end() */



void
sst_pwr_component(FILE *sstfile, pwr_method_t power_method)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    if (power_method == pwrNone)   {
	/* Nothing to do */
    } else if ((power_method == pwrMcPAT) || (power_method == pwrORION))   {
	fprintf(sstfile, "\t\t<introspector name=\"routerIntrospector\">\n");
	fprintf(sstfile, "\t\t\t<introspector_router>\n");
	fprintf(sstfile, "\t\t\t\t<params>\n");
	fprintf(sstfile, "\t\t\t\t\t<params include=intro1_params />\n");
	fprintf(sstfile, "\t\t\t\t</params>\n");
	fprintf(sstfile, "\t\t\t</introspector_router>\n");
	fprintf(sstfile, "\t\t</introspector>\n");
	fprintf(sstfile, "\n");
    }

}  /* end of sst_pwr_component() */



void
sst_gen_component(char *id, char *net_link_id, char *net_aggregator_id,
	char *nvram_aggregator_id, char *ss_aggregator_id,
	int core_num, int mpi_rank, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t<component name=%s type=\"patterns.%s\" rank=%d>\n", id, pattern_name(), mpi_rank);
    fprintf(sstfile, "\t\t<params include=Gp>\n");
    fprintf(sstfile, "\t\t\t<rank> %d </rank>\n", core_num);
    fprintf(sstfile, "\t\t</params>\n");

    if (net_link_id)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"NoC\" latency=$lat_local_net/>\n",
	    net_link_id);
    }
    if (net_aggregator_id)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"Net\" latency=$lat_global_net/>\n",
	    net_aggregator_id);
    }
    if (nvram_aggregator_id)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"NVRAM\" latency=$lat_local_nvram/>\n",
	    nvram_aggregator_id);
    }
    if (ss_aggregator_id)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"STORAGE\" latency=$lat_storage_net/>\n",
	    ss_aggregator_id);
    }
    fprintf(sstfile, "\t</component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_gen_component() */



void
sst_nvram_component(char *id, char *link_id, nvram_type_t type, int mpi_rank, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t<component name=\"%s\" type=\"patterns.bit_bucket\" rank=%d>\n", id, mpi_rank);
    if (type == LOCAL_NVRAM)   {
	fprintf(sstfile, "\t\t<params include=NVRAMparams></params>\n");
    }
    if (type == SSD)   {
	fprintf(sstfile, "\t\t<params include=SSDparams></params>\n");
    }

    if (type == LOCAL_NVRAM)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"STORAGE\" latency=$lat_storage_nvram/>\n",
	    link_id);
    }
    if (type == SSD)   {
	fprintf(sstfile, "\t\t<link name=\"%s\" port=\"STORAGE\" latency=$lat_ssd_io/>\n",
	    link_id);
    }
    fprintf(sstfile, "\t</component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nvram_component() */



void
sst_router_component_start(char *id, char *cname, router_function_t role,
	int num_ports, pwr_method_t power_method, int mpi_rank, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t<component name=\"%s\" ", id);
    if (power_method == pwrNone)   {
	fprintf(sstfile, "type=patterns.routermodel rank=%d>\n", mpi_rank);
    } else   {
	fprintf(sstfile, "type=patterns.routermodel_power rank=%d>\n", mpi_rank);
    }
    switch (role)   {
	case Rnet:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_NETWORK);
	    break;
	case RNoC:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_NoC);
	    break;
	case RnetPort:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_NET_ACCESS);
	    break;
	case Rnvram:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_NVRAM);
	    break;
	case Rstorage:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_STORAGE);
	    break;
	case RstoreIO:
	    fprintf(sstfile, "\t\t<params include=%s>\n", RNAME_IO);
	    break;
    }
    fprintf(sstfile, "\t\t\t<component_name> %s </component_name>\n", cname);
    if (num_ports >= 0)   {
	fprintf(sstfile, "\t\t\t<num_ports> %d </num_ports>\n", num_ports);
    }

}  /* end of sst_router_component_start() */



void
sst_router_component_link(char *id, uint64_t link_lat, char *link_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t\t<link name=\"%s\" port=\"%s\" latency=%" PRId64 "ns/>\n",
	id, link_name, link_lat);

}  /* end of sst_router_component_link() */



void
sst_router_component_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "\t</component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_router_component_end() */



void
sst_footer(FILE *sstfile)
{
    /* Nothing to be done here anymore */
}  /* end of sst_footer() */



/*
** Generate the pattern generator components
*/
void
sst_pattern_generators(FILE *sstfile)
{

int n, r, p;
int mpi_rank;
int aggregator, aggregator_port;
int nvram, nvram_port;
int ss, ss_port;
char id[MAX_ID_LEN];
char net_link_id[MAX_ID_LEN];
char net_aggregator_id[MAX_ID_LEN];
char nvram_link_id[MAX_ID_LEN];
char nvram_aggregator_id[MAX_ID_LEN];
char ss_link_id[MAX_ID_LEN];
char ss_aggregator_id[MAX_ID_LEN];
char *label;


    if (sstfile == NULL)   {
	return;
    }

    reset_nic_list();
    while (next_nic(&n, &r, &p, &mpi_rank,
	    &aggregator, &aggregator_port,
	    &nvram, &nvram_port,
	    &ss, &ss_port,
	    &label))   {
	snprintf(id, MAX_ID_LEN, "G%d", n);

	snprintf(net_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	snprintf(net_aggregator_id, MAX_ID_LEN, "R%dP%d", aggregator, aggregator_port);

	snprintf(nvram_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	snprintf(nvram_aggregator_id, MAX_ID_LEN, "R%dP%d", nvram, nvram_port);

	snprintf(ss_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	snprintf(ss_aggregator_id, MAX_ID_LEN, "R%dP%d", ss, ss_port);

	if (r >= 0)   {
	    if (aggregator >= 0)   {
		sst_gen_component(id, net_link_id, net_aggregator_id, nvram_aggregator_id, ss_aggregator_id, n, mpi_rank, sstfile);
	    } else   {
		/* No network aggregator in this configuration */
		sst_gen_component(id, net_link_id, NULL, nvram_aggregator_id, ss_aggregator_id, n, mpi_rank, sstfile);
	    }
	} else   {
	    /* Single node, no network */
	    if (aggregator >= 0)   {
		sst_gen_component(id, NULL, net_aggregator_id, nvram_aggregator_id, ss_aggregator_id, n, mpi_rank, sstfile);
	    } else   {
		/* No network aggregator nor NoC in this configuration */
		sst_gen_component(id, NULL, NULL, nvram_aggregator_id, ss_aggregator_id, n, mpi_rank, sstfile);
	    }
	}
    }

}  /* end of sst_pattern_generators() */



/*
** Generate the NVRAM components
*/
void
sst_nvram(FILE *sstfile)
{

int n, r, p;
int mpi_rank;
nvram_type_t t;
char id[MAX_ID_LEN];
char link_id[MAX_ID_LEN];


    if (sstfile == NULL)   {
	return;
    }

    reset_nvram_list();
    while (next_nvram(&n, &r, &p, &mpi_rank, &t))   {
	if (t == LOCAL_NVRAM)   {
	    snprintf(id, MAX_ID_LEN, "LocalNVRAM%d", n);
	}
	if (t == SSD)   {
	    snprintf(id, MAX_ID_LEN, "SSD%d", n);
	}
	snprintf(link_id, MAX_ID_LEN, "R%dP%d", r, p);

	if (r >= 0)   {
	    sst_nvram_component(id, link_id, t, mpi_rank, sstfile);
	}
    }

}  /* end of sst_nvram() */



/*
** Generate the router components
*/
void
sst_routers(FILE *sstfile, uint64_t nvram_latency, pwr_method_t power_method)
{

int l, r, p;
int lp, rp;
static int flip= 0;
char net_link_id[MAX_ID_LEN];
char nvram_link_id[MAX_ID_LEN];
char router_id[MAX_ID_LEN];
char cname[MAX_ID_LEN];
router_function_t role;
int wormhole;
link_type_t ltype;
int num_ports;
int mpi_rank;


    if (sstfile == NULL)   {
	return;
    }

    reset_router_list();
    while (next_router(&r, &role, &wormhole, &num_ports, &mpi_rank))   {
	snprintf(router_id, MAX_ID_LEN, "R%d", r);
	snprintf(cname, MAX_ID_LEN, "R%d", r);
	if (role == RnetPort)   {
	    /* We specify the number of ports because it can differ by role and location */
	    sst_router_component_start(router_id, cname, role, num_ports, power_method,
		mpi_rank, sstfile);
	} else   {
	    /*
	    ** We'll specify the number of ports in a variable, since it is the
	    ** same for all router of this type.
	    */
	    sst_router_component_start(router_id, cname, role, -1, power_method,
		mpi_rank, sstfile);
	}
	/*
	** We have to list the links in order in the params section, so the router
	** component can get the names and create the appropriate links.
	*/

	/* Links to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p, &ltype))   {
	    snprintf(net_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "\t\t\t<%s> %s </%s>\n", cname, net_link_id, cname);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &lp, &rp, &ltype))   {
	    if (l < 0)   {
		snprintf(net_link_id, MAX_ID_LEN, "unused");
		/*
		** This is a bit ugly. We're dealing with a loop back link on a router.
		** It doesn't belong in the routermodel link section, but we have to list
		** it in the parameter section so that the port numbering is sequential.
		** So, once we use the left port, then the right port number.
		*/
		if (flip)   {
		    p= lp;
		    flip= 0;
		} else   {
		    p= rp;
		    flip= 1;
		}
	    } else   {
		p= lp;
		snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    }
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "\t\t\t<%s> %s </%s>\n", cname, net_link_id, cname);
	}

	/* Links to NVRAM(s) */
	reset_router_nvram(r);
	while (next_router_nvram(r, &p))   {
	    snprintf(nvram_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "\t\t\t<%s> %s </%s>\n", cname, nvram_link_id, cname);
	}

	
	if ((power_method == pwrMcPAT) || (power_method == pwrORION))   {
	    fprintf(sstfile, "\t\t\t<router_floorplan_id> %d </router_floorplan_id>\n", r);
	}

	fprintf(sstfile, "\t\t</params>\n");



	/* Now do it again for the links section */

	/* Links to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p, &ltype))   {
	    snprintf(net_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    switch (ltype)   {
		case LnetAccess:
		    sst_router_component_link(net_link_id, NetLinkLatency() / 2, net_link_id, sstfile);
		    break;
		case LNoC:
		    sst_router_component_link(net_link_id, NoCLinkLatency() / 2, net_link_id, sstfile);
		    break;
		case LIO:
		    sst_router_component_link(net_link_id, IOLinkLatency(), net_link_id, sstfile);
		    break;
		case LNVRAM:
		    sst_router_component_link(net_link_id, IOLinkLatency(), net_link_id, sstfile);
		    break;
		case Lnet:
		case LnetNIC:
		case LNoCNIC:
		    fprintf(stderr, "These type of links are not used going to NICs\n");
	    }
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &lp, &rp, &ltype))   {
	    if (l >= 0)   {
		snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
		switch (ltype)   {
		    case Lnet:
			/* FIXME: The 0's below are temporary. */
			sst_router_component_link(net_link_id, 0, net_link_id, sstfile);
			break;
		    case LNoC:
			sst_router_component_link(net_link_id, 0, net_link_id, sstfile);
			break;
		    case LIO:
			sst_router_component_link(net_link_id, IOLinkLatency(), net_link_id, sstfile);
			break;
		    case LNVRAM:
			sst_router_component_link(net_link_id, IOLinkLatency(), net_link_id, sstfile);
			break;
		    case LnetAccess:
			sst_router_component_link(net_link_id, 0, net_link_id, sstfile);
			break;
		    case LnetNIC:
			sst_router_component_link(net_link_id, 0, net_link_id, sstfile);
			break;
		    case LNoCNIC:
			sst_router_component_link(net_link_id, 0, net_link_id, sstfile);
			break;
		}
	    } else   {
		/* Don't create loops back to the same router */
	    }
	}

	/* Link to local NVRAMS(s) */
	reset_router_nvram(r);
	while (next_router_nvram(r, &p))   {
	    snprintf(nvram_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    sst_router_component_link(nvram_link_id, nvram_latency, nvram_link_id, sstfile);
	}

	sst_router_component_end(sstfile);
    }

}  /* end of sst_routers() */
