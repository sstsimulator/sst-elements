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


#ifndef COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"
#include "sst/elements/portals4_sm/trig_cpu/portals.h"
#include <string.h>		       // for memcpy()

namespace SST {
namespace Portals4_sm {

class bcast_tree_triggered :  public application {
public:
    bcast_tree_triggered(trig_cpu *cpu) : application(cpu), init(false), algo_count(0)
    {
        radix = cpu->getRadix();
        ptl = cpu->getPortalsHandle();

        msg_size = cpu->getMessageSize();
        chunk_size = cpu->getChunkSize();

        // compute my root and children
        boost::tie(my_root, my_children) = buildBinomialTree(radix);
        num_children = my_children.size();

        if (my_id == my_root) {
            in_buf = (char*) malloc(msg_size);
            for (i = 0 ; i < msg_size ; ++i) {
                in_buf[i] = i % 255;
            }
        } else {
            in_buf = NULL;
        }
        out_buf = (char*) malloc(msg_size);
        memset(out_buf, 0, msg_size);
        bounce_buf = (char*) malloc(chunk_size);
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,0,&PT_BOUNCE);
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,2,&PT_OUT);

            /* Setup persistent ME/MD/CT to hold bounce data */
            ptl->PtlCTAlloc(PTL_CT_OPERATION, bounce_ct_h);
            me.start = bounce_buf;
            me.length = chunk_size;
            me.match_bits = 0x0;
	    me.options = 0;
            me.ignore_bits = 0x0;
            me.ct_handle = bounce_ct_h;
            ptl->PtlMEAppend(PT_BOUNCE, me, PTL_PRIORITY_LIST, NULL, bounce_me_h);

            md.start = bounce_buf;
            md.length = chunk_size;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &bounce_md_h);

            init = true;
            crReturn();
	    start_noise_section();
        }

        /* Initialization case */
        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        // Create description of user buffer.
        ptl->PtlCTAlloc(PTL_CT_OPERATION, out_me_ct_h);
        crReturn();
        me.start = out_buf;
        me.length = msg_size;
        me.match_bits = 0x0;
        me.ignore_bits = 0x0;
	me.options = 0;
        me.ct_handle = out_me_ct_h;
        ptl->PtlMEAppend(PT_OUT, me, PTL_PRIORITY_LIST, NULL, out_me_h);
        crReturn();

        ptl->PtlCTAlloc(PTL_CT_OPERATION, out_md_ct_h);
        crReturn();
        md.start = out_buf;
        md.length = msg_size;
        md.eq_handle = PTL_EQ_NONE;
        md.ct_handle = out_md_ct_h;
        ptl->PtlMDBind(md, &out_md_h);
        crReturn();

        ptl->PtlEnableCoalesce();
        crReturn();

        /* long protocol only for now */
        if (my_id == my_root) {
            /* copy to self */
            memcpy(out_buf, in_buf, msg_size);
            /* send to children */
            for (j = 0 ; j < msg_size ; j += chunk_size) {
/* 		ptl->PtlStartTriggeredPutV(num_children); */
                for (i = 0 ; i < num_children ; ++i) {
                    ptl->PtlPut(bounce_md_h, 0, 0, 0, my_children[i],
                                PT_BOUNCE, 0x0, 0, NULL, 0);
/*                     ptl->PtlTriggeredPut(bounce_md_h, 0, 0, 0, my_children[i], */
/* 					 PT_BOUNCE, 0x0, 0, NULL, 0, 0, 0); */
                    crReturn();
                }
/* 		ptl->PtlEndTriggeredPutV(); */
/* 		crReturn(); */
            }
            
        } else {
            for (j = 0 ; j < msg_size ; j += chunk_size) {
                /* when a chunk is ready, issue get. */
                comm_size = (msg_size - j > chunk_size) ? 
                    chunk_size : msg_size - j;
                ptl->PtlTriggeredGet(out_md_h, j, comm_size, my_root,
                                     PT_OUT, 0x0, NULL, j, bounce_ct_h,
                                     algo_count + j / chunk_size  + 1);
                crReturn();

                /* then when the get is completed, send ready acks to children */
		ptl->PtlStartTriggeredPutV(num_children);
                for (i = 0 ; i < num_children ; ++i) {
                    ptl->PtlTriggeredPut(bounce_md_h, 0, 0, 0, my_children[i],
                                         PT_BOUNCE, 0x0, 0, NULL, 0, out_md_ct_h,
                                         j / chunk_size  + 1);
                    crReturn();
                }
		ptl->PtlEndTriggeredPutV();
		crReturn();
            }

            /* reset 0-byte put received counter */
            count = (msg_size + chunk_size - 1) / chunk_size;
            algo_count += count;
        }

        ptl->PtlDisableCoalesce();
        crReturn();

        if (num_children > 0) {
            /* wait for completion */
            count = num_children * ((msg_size + chunk_size - 1) / chunk_size);
            while (!ptl->PtlCTWait(out_me_ct_h, count)) { crReturn(); }
        } else {
            /* wait for local gets to complete */
            count = (msg_size + chunk_size - 1) / chunk_size;
            while (!ptl->PtlCTWait(out_md_ct_h, count)) { crReturn(); }
        }
        crReturn();

        ptl->PtlCTFree(out_me_ct_h);
        crReturn();
        ptl->PtlMEUnlink(out_me_h);
        crReturn();
        ptl->PtlCTFree(out_md_ct_h);
        crReturn();
        ptl->PtlMDRelease(out_md_h);
        crReturn();

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        {
	    printf("Checking results\n");
            int bad = 0;
            for (i = 0 ; i < msg_size ; ++i) {
                if ((out_buf[i] & 0xff) != i % 255) bad++;
            }
            if (bad) printf("%5d: bad results: %d\n",my_id,bad);
        }

        crFinish();
        return true;
    }

private:
    bcast_tree_triggered();
    bcast_tree_triggered(const application& a);
    void operator=(bcast_tree_triggered const&);

    bool init;
    portals *ptl;

    SimTime_t start_time;
    int radix;
    int i, j;

    int msg_size;
    int chunk_size;
    int comm_size;
    int count;

    char *in_buf;
    char *out_buf;
    char *bounce_buf;
    
    ptl_handle_ct_t bounce_ct_h; /* short (me), long (me) */
    ptl_handle_me_t bounce_me_h; /* short, long */
    ptl_handle_md_t bounce_md_h; /* short, long */

    ptl_handle_ct_t out_me_ct_h; /* short (me), long (me) */
    ptl_handle_me_t out_me_h; /* short, long */
    ptl_handle_ct_t out_md_ct_h; /* long (md) */
    ptl_handle_md_t out_md_h; /* short, long */

    ptl_handle_ct_t ack_ct_h; /* short (me) */
    ptl_handle_me_t ack_me_h; /* short */
    ptl_handle_md_t ack_md_h; /* short */

    int my_root;
    std::vector<int> my_children;
    int num_children;

//     static const int PT_BOUNCE = 0;
//     static const int PT_ACK    = 1;
//     static const int PT_OUT    = 2;

    ptl_pt_index_t PT_BOUNCE;
    ptl_pt_index_t PT_OUT;

    uint64_t algo_count;
};

}
}
#endif // COMPONENTS_TRIG_CPU_BCAST_TREE_TRIGGERED_H
