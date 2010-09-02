/*
** $Id: sst_gen.c,v 1.13 2010/05/13 19:27:23 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <stdlib.h>
#include "sst_gen.h"
#include "gen.h"

#define MAX_ID_LEN		(256)



void
sst_header(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<?xml version=\"1.0\"?>\n");
    fprintf(sstfile, "\n");
    fprintf(sstfile, "<config>\n");
    fprintf(sstfile, "    stopAtCycle=100000000000\n");
    fprintf(sstfile, "</config>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_header() */



void
sst_gen_param_start(FILE *sstfile, int gen_debug)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<Gp>\n");
    fprintf(sstfile, "    <debug> %d </debug>\n", gen_debug);

}  /* end of sst_gen_param_start() */


void
sst_gen_param_entries(FILE *sstfile, int x_dim, int y_dim, int cores, uint64_t net_lat,
        uint64_t net_bw, uint64_t node_lat, uint64_t node_bw, uint64_t compute_time,
	uint64_t app_time, int msg_len, char *method, uint64_t chckpt_delay,
	uint64_t chckpt_interval, uint64_t envelope_write_time)
{

    if (sstfile == NULL)   {
	return;
    }

    fprintf(sstfile, "    <x_dim> %d </x_dim>\n", x_dim);
    fprintf(sstfile, "    <y_dim> %d </y_dim>\n", y_dim);
    fprintf(sstfile, "    <cores> %d </cores>\n", cores);
    fprintf(sstfile, "    <net_latency> %lu </net_latency>\n", net_lat);
    fprintf(sstfile, "    <net_bandwidth> %lu </net_bandwidth>\n", net_bw);
    fprintf(sstfile, "    <node_latency> %lu </node_latency>\n", node_lat);
    fprintf(sstfile, "    <node_bandwidth> %lu </node_bandwidth>\n", node_bw);
    fprintf(sstfile, "    <compute_time> %lu </compute_time>\n", compute_time);
    fprintf(sstfile, "    <application_end_time> %lu </application_end_time>\n", app_time);
    fprintf(sstfile, "    <exchange_msg_len> %d </exchange_msg_len>\n", msg_len);
    fprintf(sstfile, "    <chckpt_method> %s </chckpt_method>\n", method);
    fprintf(sstfile, "    <chckpt_delay> %lu </chckpt_delay>\n", chckpt_delay);
    fprintf(sstfile, "    <chckpt_interval> %lu </chckpt_interval>\n", chckpt_interval);
    fprintf(sstfile, "    <envelope_write_time> %lu </envelope_write_time>\n", envelope_write_time);

}  /* end of sst_gen_param_entries() */


void
sst_gen_param_end(FILE *sstfile, uint64_t node_latency, uint64_t net_latency)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</Gp>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<LocalNet>\n");
    fprintf(sstfile, "    <name> NoC </name>\n");
    fprintf(sstfile, "    <lat> %luns </lat>\n", node_latency);
    fprintf(sstfile, "</LocalNet>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<GlobalNet>\n");
    fprintf(sstfile, "    <name> NETWORK </name>\n");
    fprintf(sstfile, "    <lat> %luns </lat>\n", net_latency);
    fprintf(sstfile, "</GlobalNet>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<LocalNVRAMaccess>\n");
    fprintf(sstfile, "    <name> NVRAM </name>\n");
    fprintf(sstfile, "    <lat> %luns </lat>\n", net_latency);
    fprintf(sstfile, "</LocalNVRAMaccess>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<StableStorageAccess>\n");
    fprintf(sstfile, "    <name> STORAGE </name>\n");
    fprintf(sstfile, "    <lat> %luns </lat>\n", net_latency);
    fprintf(sstfile, "</StableStorageAccess>\n");
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
	fprintf(sstfile, "<intro1_params>\n");
	fprintf(sstfile, "    <period>15000000ns</period>\n");
	fprintf(sstfile, "    <model>routermodel</model>\n");
	fprintf(sstfile, "</intro1_params>\n");
	fprintf(sstfile, "\n");
    } else if (power_method == pwrORION)   {
	fprintf(sstfile, "<intro1_params>\n");
	fprintf(sstfile, "    <period>15000000ns</period>\n");
	fprintf(sstfile, "    <model>routermodel</model>\n");
	fprintf(sstfile, "</intro1_params>\n");
	fprintf(sstfile, "\n");
    } else   {
	/* error */
    }

}  /* end of sst_pwr_param_entries() */


void
sst_nvram_param_entries(FILE *sstfile)
{
    if (sstfile == NULL)   {
	return;
    }

    fprintf(sstfile, "<NVRAMparams>\n");
    fprintf(sstfile, "    <debug> 0 </debug>\n");
    fprintf(sstfile, "</NVRAMparams>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<SSDparams>\n");
    fprintf(sstfile, "    <debug> 0 </debug>\n");
    fprintf(sstfile, "</SSDparams>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nvram_param_entries() */


void
sst_router_param_start(FILE *sstfile, int num_ports, uint64_t router_bw, int num_cores,
    pwr_method_t power_method)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<Rp>\n");
    fprintf(sstfile, "    <hop_delay> 25 </hop_delay>\n");
    fprintf(sstfile, "    <debug> 0 </debug>\n");
    fprintf(sstfile, "    <num_ports> %d </num_ports>\n", num_ports);
    fprintf(sstfile, "    <bw> %lu </bw>\n", router_bw);

    if (power_method == pwrNone)   {
	/* Nothing to do */
    } else if (power_method == pwrMcPAT)   {
	fprintf(sstfile, "    <McPAT_XMLfile>../../core/techModels/libMcPATbeta/Niagara1.xml</McPAT_XMLfile>\n");
	fprintf(sstfile, "    <core_temperature>350</core_temperature>\n");
	fprintf(sstfile, "    <core_tech_node>65</core_tech_node>\n");
	fprintf(sstfile, "    <frequency>1ns</frequency>\n");
	fprintf(sstfile, "    <power_monitor>YES</power_monitor>\n");
	fprintf(sstfile, "    <temperature_monitor>NO</temperature_monitor>\n");
	fprintf(sstfile, "    <router_voltage>1.1</router_voltage>\n");
	fprintf(sstfile, "    <router_clock_rate>1000000000</router_clock_rate>\n");
	fprintf(sstfile, "    <router_flit_bits>64</router_flit_bits>\n");
	fprintf(sstfile, "    <router_virtual_channel_per_port>2</router_virtual_channel_per_port>\n");
	fprintf(sstfile, "    <router_input_ports>%d</router_input_ports>\n", num_ports);
	fprintf(sstfile, "    <router_output_ports>%d</router_output_ports>\n", num_ports);
	fprintf(sstfile, "    <router_link_length>16691</router_link_length>\n");
	fprintf(sstfile, "    <router_power_model>McPAT</router_power_model>\n");
	fprintf(sstfile, "    <router_has_global_link>1</router_has_global_link>\n");
	fprintf(sstfile, "    <router_input_buffer_entries_per_vc>16</router_input_buffer_entries_per_vc>\n");
	fprintf(sstfile, "    <router_link_throughput>1</router_link_throughput>\n");
	fprintf(sstfile, "    <router_link_latency>1</router_link_latency>\n");
	fprintf(sstfile, "    <router_horizontal_nodes>1</router_horizontal_nodes>\n");
	fprintf(sstfile, "    <router_vertical_nodes>1</router_vertical_nodes>\n");
	fprintf(sstfile, "    <router_topology>RING</router_topology>\n");
	fprintf(sstfile, "    <core_number_of_NoCs>%d</core_number_of_NoCs>\n", num_cores);
	fprintf(sstfile, "    <push_introspector>routerIntrospector</push_introspector>\n");

    } else if (power_method == pwrORION)   {
	fprintf(sstfile, "    <ORION_XMLfile>../../core/techModels/libORION/something.xml</ORION_XMLfile>\n");
	fprintf(sstfile, "    <core_temperature>350</core_temperature>\n");
	fprintf(sstfile, "    <core_tech_node>65</core_tech_node>\n");
	fprintf(sstfile, "    <frequency>1ns</frequency>\n");
	fprintf(sstfile, "    <power_monitor>YES</power_monitor>\n");
	fprintf(sstfile, "    <temperature_monitor>NO</temperature_monitor>\n");
	fprintf(sstfile, "    <router_voltage>1.1</router_voltage>\n");
	fprintf(sstfile, "    <router_clock_rate>1000000000</router_clock_rate>\n");
	fprintf(sstfile, "    <router_flit_bits>64</router_flit_bits>\n");
	fprintf(sstfile, "    <router_virtual_channel_per_port>2</router_virtual_channel_per_port>\n");
	fprintf(sstfile, "    <router_input_ports>%d</router_input_ports>\n", num_ports);
	fprintf(sstfile, "    <router_output_ports>%d</router_output_ports>\n", num_ports);
	fprintf(sstfile, "    <router_link_length>16691</router_link_length>\n");
	fprintf(sstfile, "    <router_power_model>ORION</router_power_model>\n");
	fprintf(sstfile, "    <router_has_global_link>1</router_has_global_link>\n");
	fprintf(sstfile, "    <router_input_buffer_entries_per_vc>16</router_input_buffer_entries_per_vc>\n");
	fprintf(sstfile, "    <router_link_throughput>1</router_link_throughput>\n");
	fprintf(sstfile, "    <router_link_latency>1</router_link_latency>\n");
	fprintf(sstfile, "    <router_horizontal_nodes>1</router_horizontal_nodes>\n");
	fprintf(sstfile, "    <router_vertical_nodes>1</router_vertical_nodes>\n");
	fprintf(sstfile, "    <router_topology>RING</router_topology>\n");
	fprintf(sstfile, "    <core_number_of_NoCs>%d</core_number_of_NoCs>\n", num_cores);
	fprintf(sstfile, "    <push_introspector>routerIntrospector</push_introspector>\n");

    } else   {
	/* error */
    }

}  /* end of sst_router_param_start() */



void
sst_router_param_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</Rp>\n");
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
	fprintf(sstfile, "    <introspector id=\"routerIntrospector\">\n");
	fprintf(sstfile, "        <introspector_router>\n");
	fprintf(sstfile, "            <params>\n");
	fprintf(sstfile, "                <params reference=intro1_params />\n");
	fprintf(sstfile, "            </params>\n");
	fprintf(sstfile, "        </introspector_router>\n");
	fprintf(sstfile, "    </introspector>\n");
	fprintf(sstfile, "\n");
    }

}  /* end of sst_pwr_component() */



void
sst_gen_component(char *id, char *net_link_id, char *net_aggregator_id,
	char *nvram_aggregator_id, char *ss_aggregator_id, float weight,
	int rank, char *pattern_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", id, weight);
    fprintf(sstfile, "        <%s>\n", pattern_name);
    fprintf(sstfile, "            <params include=Gp>\n");
    fprintf(sstfile, "               <rank> %d </rank>\n", rank);
    fprintf(sstfile, "            </params>\n");
    fprintf(sstfile, "            <links>\n");
    if (net_link_id)   {
	fprintf(sstfile, "                <link id=\"%s\">\n", net_link_id);
	fprintf(sstfile, "                    <params reference=LocalNet> </params>\n");
	fprintf(sstfile, "                </link>\n");
    }
    if (net_aggregator_id)   {
	fprintf(sstfile, "                <link id=\"%s\">\n", net_aggregator_id);
	fprintf(sstfile, "                    <params reference=GlobalNet> </params>\n");
	fprintf(sstfile, "                </link>\n");
    }
    if (nvram_aggregator_id)   {
	fprintf(sstfile, "                <link id=\"%s\">\n", nvram_aggregator_id);
	fprintf(sstfile, "                    <params reference=LocalNVRAMaccess> </params>\n");
	fprintf(sstfile, "                </link>\n");
    }
    if (ss_aggregator_id)   {
	fprintf(sstfile, "                <link id=\"%s\">\n", ss_aggregator_id);
	fprintf(sstfile, "                    <params reference=StableStorageAccess> </params>\n");
	fprintf(sstfile, "                </link>\n");
    }
    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </%s>\n", pattern_name);
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_gen_component() */



void
sst_nvram_component(char *id, char *link_id, float weight, nvram_type_t type, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", id, weight);
    fprintf(sstfile, "        <bit_bucket>\n");
    if (type == LOCAL_NVRAM)   {
	fprintf(sstfile, "            <params include=NVRAMparams></params>\n");
    }
    if (type == SSD)   {
	fprintf(sstfile, "            <params include=SSDparams></params>\n");
    }
    fprintf(sstfile, "            <links>\n");
    fprintf(sstfile, "                <link id=\"%s\">\n", link_id);
    fprintf(sstfile, "                    <params reference=LocalNVRAMaccess> </params>\n");
    fprintf(sstfile, "                </link>\n");
    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </bit_bucket>\n");
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_nvram_component() */



void
sst_router_component_start(char *id, float weight, char *cname, int wormhole, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%.2f>\n", id, weight);
    fprintf(sstfile, "        <routermodel>\n");
    fprintf(sstfile, "            <params include=Rp>\n");
    fprintf(sstfile, "                <component_name> %s </component_name>\n", cname);
    fprintf(sstfile, "                <wormhole> %d </wormhole>\n", wormhole);

}  /* end of sst_router_component_start() */



void
sst_router_component_link(char *id, uint64_t link_lat, char *link_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "            <link id=\"%s\">\n", id);
    fprintf(sstfile, "                <params>\n");
    fprintf(sstfile, "                    <lat>%luns</lat>\n", link_lat);
    fprintf(sstfile, "                    <name>%s</name>\n", link_name);
    fprintf(sstfile, "                </params>\n");
    fprintf(sstfile, "            </link>\n");

}  /* end of sst_router_component_link() */



void
sst_router_component_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </routermodel>\n");
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_router_component_end() */



void
sst_footer(FILE *sstfile)
{

    if (sstfile != NULL)   {
	fprintf(sstfile, "\n");
    }

}  /* end of sst_footer() */



/*
** Generate the pattern generator components
*/
void
sst_pattern_generators(char *pattern_name, FILE *sstfile)
{

int n, r, p;
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
    while (next_nic(&n, &r, &p,
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
		sst_gen_component(id, net_link_id, net_aggregator_id, nvram_aggregator_id, ss_aggregator_id, 1.0, n, pattern_name, sstfile);
	    } else   {
		/* No network aggregator in this configuration */
		sst_gen_component(id, net_link_id, NULL, nvram_aggregator_id, ss_aggregator_id, 1.0, n, pattern_name, sstfile);
	    }
	} else   {
	    /* Single node, no network */
	    if (aggregator >= 0)   {
		sst_gen_component(id, NULL, net_aggregator_id, nvram_aggregator_id, ss_aggregator_id, 1.0, n, pattern_name, sstfile);
	    } else   {
		/* No network aggregator nor NoC in this configuration */
		sst_gen_component(id, NULL, NULL, nvram_aggregator_id, ss_aggregator_id, 1.0, n, pattern_name, sstfile);
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
nvram_type_t t;
char id[MAX_ID_LEN];
char link_id[MAX_ID_LEN];


    if (sstfile == NULL)   {
	return;
    }

    reset_nvram_list();
    while (next_nvram(&n, &r, &p, &t))   {
	if (t == LOCAL_NVRAM)   {
	    snprintf(id, MAX_ID_LEN, "LocalNVRAM%d", n);
	}
	if (t == SSD)   {
	    snprintf(id, MAX_ID_LEN, "SSD%d", n);
	}
	snprintf(link_id, MAX_ID_LEN, "R%dP%d", r, p);

	if (r >= 0)   {
	    sst_nvram_component(id, link_id, 1.0, t, sstfile);
	}
    }

}  /* end of sst_nvram() */



/*
** Generate the router components
*/
void
sst_routers(FILE *sstfile, uint64_t node_latency, uint64_t net_latency,
	uint64_t nvram_latency, pwr_method_t power_method)
{

int l, r, p;
char net_link_id[MAX_ID_LEN];
char nvram_link_id[MAX_ID_LEN];
char router_id[MAX_ID_LEN];
char cname[MAX_ID_LEN];
int wormhole;


    if (sstfile == NULL)   {
	return;
    }

    reset_router_list();
    while (next_router(&r, &wormhole))   {
	snprintf(router_id, MAX_ID_LEN, "R%d", r);
	snprintf(cname, MAX_ID_LEN, "R%d", r);
	sst_router_component_start(router_id, 1.0, cname, wormhole, sstfile);
	/*
	** We have to list the links in order in the params section, so the router
	** componentn can get the names and create the appropriate links.
	*/

	/* Links to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "                <%s> %s </%s>\n", cname, net_link_id, cname);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "                <%s> %s </%s>\n", cname, net_link_id, cname);
	}

	/* Links to NVRAM(s) */
	reset_router_nvram(r);
	while (next_router_nvram(r, &p))   {
	    snprintf(nvram_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    snprintf(cname, MAX_ID_LEN, "Link%dname", p);
	    fprintf(sstfile, "                <%s> %s </%s>\n", cname, nvram_link_id, cname);
	}

	
	if ((power_method == pwrMcPAT) || (power_method == pwrORION))   {
	    fprintf(sstfile, "                <router_floorplan_id> %d </router_floorplan_id>\n", r);
	}

	fprintf(sstfile, "            </params>\n");
	fprintf(sstfile, "            <links>\n");



	/* Now do it again for the links section */

	/* Links to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "R%dP%d", r, p);
	    sst_router_component_link(net_link_id, node_latency, net_link_id, sstfile);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    sst_router_component_link(net_link_id, net_latency, net_link_id, sstfile);
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
