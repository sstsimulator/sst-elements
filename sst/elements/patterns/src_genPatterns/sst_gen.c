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

    fprintf(sstfile, "<gen_params>\n");
    fprintf(sstfile, "    <debug> %d </debug>\n", gen_debug);

}  /* end of sst_gen_param_start() */


void
sst_gen_param_entries(FILE *sstfile, int x_dim, int y_dim, double lat,
	double bw, double compute_time, int msg_len)
{

    if (sstfile == NULL)   {
	return;
    }

    fprintf(sstfile, "    <x_dim> %d </x_dim>\n", x_dim);
    fprintf(sstfile, "    <y_dim> %d </y_dim>\n", y_dim);
    fprintf(sstfile, "    <latency> %0.9f </latency>\n", lat);
    fprintf(sstfile, "    <bandwidth> %0.0f </bandwidth>\n", bw);
    fprintf(sstfile, "    <compute_time> %0.9f </compute_time>\n", compute_time);
    fprintf(sstfile, "    <exchange_msg_len> %d </exchange_msg_len>\n", msg_len);

}  /* end of sst_gen_param_entries() */


void
sst_gen_param_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</gen_params>\n");
    fprintf(sstfile, "\n");

    fprintf(sstfile, "<gen_net_link>\n");
    fprintf(sstfile, "    <name> NETWORK </name>\n");
    fprintf(sstfile, "    <lat> 1ns </lat>\n");
    fprintf(sstfile, "</gen_net_link>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_gen_param_end() */



void
sst_router_param_start(FILE *sstfile, int num_ports)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "<router_params>\n");
    fprintf(sstfile, "    <hop_delay> 2us </hop_delay>\n");
    fprintf(sstfile, "    <debug> 0 </debug>\n");
    fprintf(sstfile, "    <num_ports> %d </num_ports>\n", num_ports);

}  /* end of sst_router_param_start() */



void
sst_router_param_end(FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "</router_params>\n");
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
sst_gen_component(char *id, char *net_link_id, float weight, int rank, char *pattern_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", id, weight);
    fprintf(sstfile, "        <%s>\n", pattern_name);
    fprintf(sstfile, "            <params include=gen_params>\n");
    fprintf(sstfile, "               <rank> %d </rank>\n", rank);
    fprintf(sstfile, "            </params>\n");
    fprintf(sstfile, "            <links>\n");
    fprintf(sstfile, "                <link id=\"%s\">\n", net_link_id);
    fprintf(sstfile, "                    <params reference=gen_net_link> </params>\n");
    fprintf(sstfile, "                </link>\n");
    fprintf(sstfile, "            </links>\n");
    fprintf(sstfile, "        </%s>\n", pattern_name);
    fprintf(sstfile, "    </component>\n");
    fprintf(sstfile, "\n");

}  /* end of sst_gen_component() */



void
sst_router_component_start(char *id, float weight, char *cname, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "    <component id=\"%s\" weight=%f>\n", id, weight);
    fprintf(sstfile, "        <routermodel>\n");
    fprintf(sstfile, "            <params include=router_params>\n");
    fprintf(sstfile, "                <component_name> %s </component_name>\n", cname);

}  /* end of sst_router_component_start() */



void
sst_router_component_link(char *id, char *link_lat, char *link_name, FILE *sstfile)
{

    if (sstfile == NULL)   {
	/* Nothing to output */
	return;
    }

    fprintf(sstfile, "            <link id=\"%s\">\n", id);
    fprintf(sstfile, "                <params>\n");
    fprintf(sstfile, "                    <lat>%s</lat>\n", link_lat);
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
char id[MAX_ID_LEN];
char net_link_id[MAX_ID_LEN];
char *label;


    if (sstfile == NULL)   {
	return;
    }

    reset_nic_list();
    while (next_nic(&n, &r, &p, &label))   {
	snprintf(id, MAX_ID_LEN, "gen%d", n);
	snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
	sst_gen_component(id, net_link_id, 1.0, n, pattern_name, sstfile);
    }

}  /* end of sst_pattern_generators() */



/*
** Generate the router components
*/
void
sst_routers(FILE *sstfile)
{

int l, r, p;
char net_link_id[MAX_ID_LEN];
char router_id[MAX_ID_LEN];
char cname[MAX_ID_LEN];


    if (sstfile == NULL)   {
	return;
    }

    reset_router_list();
    while (next_router(&r))   {
	snprintf(router_id, MAX_ID_LEN, "router%d", r);
	snprintf(cname, MAX_ID_LEN, "R%d", r);
	sst_router_component_start(router_id, 0.3, cname, sstfile);
	/*
	** We have to list the links in order in the params section, so the router
	** componentn can get the names and create the appropriate links.
	*/

	/* Link to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
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
	fprintf(sstfile, "            </params>\n");
	fprintf(sstfile, "            <links>\n");



	/* Now do it again for the links section */

	/* Link to local NIC(s) */
	reset_router_nics(r);
	while (next_router_nic(r, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "Router%dPort%d", r, p);
	    sst_router_component_link(net_link_id, "1ns", net_link_id, sstfile);
	}

	/* Links to other routers */
	reset_router_links(r);
	while (next_router_link(r, &l, &p))   {
	    snprintf(net_link_id, MAX_ID_LEN, "L%d", l);
	    sst_router_component_link(net_link_id, "1ns", net_link_id, sstfile);
	}

	sst_router_component_end(sstfile);
    }

}  /* end of sst_routers() */
