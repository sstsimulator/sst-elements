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


#ifndef COMPONENTS_TRIG_CPU_PINGPONG_BW_H
#define COMPONENTS_TRIG_CPU_PINGPONG_BW_H

#include <string>
#include <map>
#include <vector>

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include <string.h>		       // for memcpy()
#include <stdio.h>

namespace SST {
namespace Portals4_sm {

#define MAX(a, b)  (a < b) ? b : a

//#define DEBUG(a) printf a
#define DEBUG(a)

/* match/ignore bit manipulation
 *
 * 0123 4567 01234567 01234567 01234567 01234567 01234567 01234567 01234567
 *     |             |                 |
 * ^   | context id  |      source     |            message tag
 * |   |             |                 |
 * +---- protocol
 */

#define PTL_PROTOCOL_MASK 0xF000000000000000ULL
#define PTL_CONTEXT_MASK  0x0FFF000000000000ULL
#define PTL_SOURCE_MASK   0x0000FFFF00000000ULL
#define PTL_TAG_MASK      0x00000000FFFFFFFFULL

#define PTL_PROTOCOL_IGNR PTL_PROTOCOL_MASK
#define PTL_CONTEXT_IGNR  PTL_CONTEXT_MASK
#define PTL_SOURCE_IGNR   PTL_SOURCE_MASK
#define PTL_TAG_IGNR      0x000000007FFFFFFFULL

#define PTL_SHORT_MSG     0x1000000000000000ULL
#define PTL_LONG_MSG      0x2000000000000000ULL
#define PTL_MID_MSG     0x4000000000000000ULL

/* send posting */
#define PTL_SET_SEND_BITS(match_bits, contextid, source, tag, type)     \
    {                                                                   \
        match_bits = contextid;                                         \
        match_bits = (match_bits << 16);                                \
        match_bits |= source;                                           \
        match_bits = (match_bits << 32);                                \
        match_bits |= (PTL_TAG_MASK & tag) | type;                      \
    }

/* receive posting */
#define PTL_SET_RECV_BITS(match_bits, ignore_bits, contextid, source, tag) \
    {                                                                   \
        match_bits = 0;                                                 \
        ignore_bits = PTL_PROTOCOL_IGNR;                                \
                                                                        \
        match_bits = contextid;                                         \
        match_bits = (match_bits << 16);                                \
        match_bits |= source;                                           \
        match_bits = (match_bits << 32);                                \
        match_bits |= (PTL_TAG_MASK & tag);                             \
    }

#define PTL_IS_SHORT_MSG(match_bits)            \
    (0 != (PTL_SHORT_MSG & match_bits))
#define PTL_IS_LONG_MSG(match_bits)             \
    (0 != (PTL_LONG_MSG & match_bits))
#define PTL_IS_MID_MSG(match_bits)            \
    (0 != (PTL_MID_MSG & match_bits))
#define PTL_IS_SYNC_MSG(ev)                  \
    (0 != ev->hdr_data)

#define PTL_GET_TAG(match_bits) ((int)(match_bits & PTL_TAG_MASK))
#define PTL_GET_SOURCE(match_bits) ((int)((match_bits & PTL_SOURCE_MASK) >> 32))


class pingpong_bw :  public application {
public:
    pingpong_bw(trig_cpu *cpu, const std::string &proto, bool delay) : application(cpu)
    {
        ptl = cpu->getPortalsHandle();

        start_len = 1;
        stop_len = 1 * 1024 * 1024;
        perturbation = 3;
        nrepeat = 10;
        send_buf = big_send_buf = (char*) malloc(stop_len * 2);
        recv_buf = big_recv_buf = (char*) malloc(stop_len * 2);

        eager_len = 32 * 1024;
        long_len = 128 * 1024; /* only used in two protocol, see below */

        contextid = 1;
        tag = 2;
        send_count = recv_count = 0;

        if (!proto.compare("eager")) {
            protocol = eager;
            printf("Protocol: eager\n");
            strcpy(base_filename, "eager");
        } else if (!proto.compare("rndv")) {
            protocol = rndv;
            printf("Protocol: rendezvous\n");
            strcpy(base_filename, "rndv");
        } else if (!proto.compare("triggered")) {
            protocol = triggered;
            printf("Protocol: triggered\n");
            strcpy(base_filename, "triggered");
        } else if (!proto.compare("probe")) {
            protocol = probe;
            printf("Protocol: probe\n");
            strcpy(base_filename, "probe");
        } else if (!proto.compare("two")) {
            protocol = two;
            printf("Protocol: two\n");
            strcpy(base_filename, "two");
        } else if (!proto.compare("two-probe")) {
            protocol = two_probe;
            printf("Protocol: two-probe\n");
            strcpy(base_filename, "two-probe");
        } else {
            printf("Unknown protocol: %s\n", proto.c_str());
            abort();
        }

        if (delay) {
            delays.push_back(0.05);
            delays.push_back(0.10);
            delays.push_back(0.15);
            delays.push_back(0.25);
            delays.push_back(0.50);
            delays.push_back(0.75);
            delays.push_back(1.00);
        }

        probe_freqs.push_back(1);
        probe_freqs.push_back(2);
        probe_freqs.push_back(4);
        probe_freqs.push_back(8);
        probe_freqs.push_back(16);

        probe_freq = 1;
    }


    bool
    operator()(Event *event)
    {
        crInit();

        crFuncStart(short_msg_repost);
        {
            ptl_me_t me;

            me.start = need_repost;
            me.length = (eager_len + 8) * 1024;
            me.ct_handle = PTL_CT_NONE;
            me.min_free = eager_len + 8;
            me.options = PTL_ME_OP_PUT | PTL_ME_MANAGE_LOCAL | PTL_ME_NO_TRUNCATE | 
                PTL_ME_MAY_ALIGN | PTL_ME_ACK_DISABLE | PTL_ME_EVENT_COMM_DISABLE;
            me.match_bits = PTL_SHORT_MSG;
            if (protocol == triggered) {
                me.ignore_bits = ~0ULL;
            } else {
                me.ignore_bits = PTL_CONTEXT_MASK | PTL_SOURCE_MASK | PTL_TAG_MASK;
            }

            ptl->PtlMEAppend(send_pt,
                             me,
                             PTL_OVERFLOW,
                             need_repost,
                             overflow_me_h);
            crReturn();
        }
        crFuncEnd();

        crFuncStart(short_msg_init);
        {
            for (i = 0 ; i < 2 ; ++i) {
                need_repost = (char*) malloc((eager_len + 8) * 1024);
                crFuncCall(short_msg_repost);
            }
        }
        crFuncEnd();

        crFuncStart(send);
        send_count++;

        if (cur_len < eager_len) {
            ptl_md_t md;

            PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_SHORT_MSG);

            md.start = send_buf;
            md.length = cur_len;
            md.options = 0;
            md.eq_handle = send_eq_h;
            md.ct_handle = PTL_CT_NONE;
    
            ptl->PtlMDBind(md, &send_md_h);
            crReturn();

            DEBUG(("%02d: send: posting short send\n", my_id)); 
            ptl->PtlPut(send_md_h,
                        0,
                        cur_len,
                        PTL_NO_ACK_REQ,
                        peer,
                        send_pt,
                        match_bits,
                        0,
                        NULL,
                        0);
            crReturn();

        } else {
            if (protocol == eager || protocol == probe) {
                ptl_md_t md;
                ptl_me_t me;

                PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_LONG_MSG);

                md.start = send_buf;
                md.length = cur_len;
                md.options = 0;
                md.eq_handle = send_eq_h;
                md.ct_handle = PTL_CT_NONE;

                ptl->PtlMDBind(md, &send_md_h);
                crReturn();

                me.start = send_buf;
                me.length = cur_len;
                me.ct_handle = PTL_CT_NONE;
                me.min_free = 0;
                me.options = PTL_ME_OP_GET | PTL_ME_USE_ONCE;
                me.match_bits = send_count;
                me.ignore_bits = 0;

                ptl->PtlMEAppend(read_pt,
                                 me,
                                 PTL_PRIORITY_LIST,
                                 NULL,
                                 send_me_h);
                crReturn();

                ptl->PtlPut(send_md_h,
                            0,
                            cur_len,
                            PTL_ACK_REQ,
                            peer,
                            send_pt,
                            match_bits,
                            0,
                            NULL,
                            send_count);
                crReturn();

            } else if (protocol == rndv) {
                ptl_md_t md;
                ptl_me_t me;

                PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_LONG_MSG);

                md.start = send_buf;
                md.length = eager_len;
                md.options = 0;
                md.eq_handle = send_eq_h;
                md.ct_handle = PTL_CT_NONE;

                ptl->PtlMDBind(md, &send_md_h);
                crReturn();

                me.start = send_buf;
                me.length = cur_len;
                me.ct_handle = PTL_CT_NONE;
                me.min_free = 0;
                me.options = PTL_ME_OP_GET | PTL_ME_USE_ONCE;
                me.match_bits = (send_count << 32) | cur_len;
                me.ignore_bits = 0;

                ptl->PtlMEAppend(read_pt,
                                 me,
                                 PTL_PRIORITY_LIST,
                                 NULL,
                                 send_me_h);
                crReturn();

                ptl->PtlPut(send_md_h,
                            0,
                            eager_len,
                            PTL_NO_ACK_REQ,
                            peer,
                            send_pt,
                            match_bits,
                            0,
                            NULL,
                            (send_count << 32) | cur_len);
                crReturn();

            } else if (protocol == triggered) {
                ptl_md_t md;
                ptl_me_t me;

                PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_LONG_MSG);

                md.start = send_buf;
                md.length = cur_len + 8;
                md.options = 0;
                md.eq_handle = send_eq_h;
                md.ct_handle = PTL_CT_NONE;

                ptl->PtlMDBind(md, &send_md_h);
                crReturn();

                me.start = send_buf;
                me.length = cur_len;
                me.ct_handle = PTL_CT_NONE;
                me.min_free = 0;
                me.options = PTL_ME_OP_GET | PTL_ME_USE_ONCE;
                me.match_bits = send_count;
                me.ignore_bits = 0;

                DEBUG(("%02d: send: posting me with send count %lu\n", my_id, send_count));
                ptl->PtlMEAppend(read_pt,
                                 me,
                                 PTL_PRIORITY_LIST,
                                 NULL,
                                 send_me_h);
                crReturn();

                ptl->PtlPut(send_md_h,
                            0,
                            eager_len + 8,
                            PTL_NO_ACK_REQ,
                            peer,
                            send_pt,
                            match_bits,
                            0,
                            NULL,
                            send_count);
                crReturn();

            } else  if (protocol == two || protocol == two_probe) {
                if (cur_len < long_len) {
                    ptl_md_t md;
                    ptl_me_t me;

                    PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_MID_MSG);

                    md.start = send_buf;
                    md.length = cur_len;
                    md.options = 0;
                    md.eq_handle = send_eq_h;
                    md.ct_handle = PTL_CT_NONE;

                    ptl->PtlMDBind(md, &send_md_h);
                    crReturn();

                    me.start = send_buf;
                    me.length = cur_len;
                    me.ct_handle = PTL_CT_NONE;
                    me.min_free = 0;
                    me.options = PTL_ME_OP_GET | PTL_ME_USE_ONCE;
                    me.match_bits = send_count;
                    me.ignore_bits = 0;

                    ptl->PtlMEAppend(read_pt,
                                     me,
                                     PTL_PRIORITY_LIST,
                                     NULL,
                                     send_me_h);
                    crReturn();

                    ptl->PtlPut(send_md_h,
                                0,
                                cur_len,
                                PTL_ACK_REQ,
                                peer,
                                send_pt,
                                match_bits,
                                0,
                                NULL,
                                send_count);
                    crReturn();
                } else {
                    ptl_md_t md;
                    ptl_me_t me;

                    PTL_SET_SEND_BITS(match_bits, contextid, my_id, tag, PTL_LONG_MSG);

                    md.start = send_buf;
                    md.length = eager_len;
                    md.options = 0;
                    md.eq_handle = send_eq_h;
                    md.ct_handle = PTL_CT_NONE;

                    ptl->PtlMDBind(md, &send_md_h);
                    crReturn();

                    me.start = send_buf;
                    me.length = cur_len;
                    me.ct_handle = PTL_CT_NONE;
                    me.min_free = 0;
                    me.options = PTL_ME_OP_GET | PTL_ME_USE_ONCE;
                    me.match_bits = (send_count << 32) | cur_len;
                    me.ignore_bits = 0;

                    ptl->PtlMEAppend(read_pt,
                                     me,
                                     PTL_PRIORITY_LIST,
                                     NULL,
                                     send_me_h);
                    crReturn();

                    ptl->PtlPut(send_md_h,
                                0,
                                eager_len,
                                PTL_NO_ACK_REQ,
                                peer,
                                send_pt,
                                match_bits,
                                0,
                                NULL,
                                (send_count << 32) | cur_len);
                    crReturn();
                }
            }
        }
        crFuncEnd();

        crFuncStart(send_wait);
        if (cur_len < eager_len) {
#if 0
            while (!ptl->PtlEQWait(send_eq_h, &ev)) { crReturn(); }
            if (ev.type != PTL_EVENT_SEND) { printf("short send received unknown event type %d\n", (int) ev.type); abort(); }
#endif
            ptl->PtlMDRelease(send_md_h);
        } else {
            while (true) {
                while (!ptl->PtlEQWait(send_eq_h, &ev)) { crReturn(); }
                DEBUG(("%02d: send: event %d\n", my_id, ev.type));
                if (ev.type == PTL_EVENT_ACK) {
                    ptl->PtlMEUnlink(send_me_h);
                    crReturn();
                    ptl->PtlMDRelease(send_md_h);
                    crReturn();
                    goto long_send_done;
                } else if (ev.type == PTL_EVENT_GET) {
                    ptl->PtlMDRelease(send_md_h);
                    crReturn();
                    goto long_send_done;
                }
            }
        }
    long_send_done:
        crFuncEnd();

        crFuncStart(recv);
            ptl_me_t me;

            recv_count++;

            probe_this_iter = (0 == recv_count % probe_freq) ? 1 : 0;

            PTL_SET_RECV_BITS(match_bits, ignore_bits, contextid, peer, tag);

            if (protocol == triggered && cur_len >= eager_len) {
                ptl->PtlCTAlloc(PTL_CT_BYTE, ct_h);
                crReturn();
            }

            if (protocol == triggered && cur_len >= eager_len) {
                ptl_md_t md;

                md.start = recv_buf + eager_len;
                md.length = cur_len - eager_len;
                md.options = 0;
                md.eq_handle = recv_eq_h;
                md.ct_handle = PTL_CT_NONE;

                ptl->PtlMDBind(md, &recv_md_h);
                crReturn();

                DEBUG(("%02d: recv: posting triggered with match %lu\n", 
                        my_id, (unsigned long) recv_count));
                ptl->PtlTriggeredGet(recv_md_h, 
                                     0, 
                                     cur_len - eager_len, 
                                     peer,
                                     read_pt,
                                     recv_count,
                                     NULL,
                                     eager_len,
                                     ct_h,
                                     eager_len + 8);
                crReturn();

            }

            me.start = recv_buf;
            if (protocol == triggered) {
                me.length = cur_len + 8;
            } else {
                me.length = cur_len;
            }
            if (probe_this_iter) {
                if (protocol == probe && cur_len >= eager_len) {
                    me.length = 8;
                } else if (protocol == two_probe && (cur_len >= eager_len && cur_len < long_len)) {
                    me.length = 8;
                }
            }
            if (protocol == triggered && cur_len >= eager_len) {
                me.ct_handle = ct_h;
            } else {
                me.ct_handle = PTL_CT_NONE;
            }
            me.min_free = 0;
            me.options = PTL_ME_OP_PUT | PTL_ME_USE_ONCE | PTL_ME_EVENT_UNLINK_DISABLE | PTL_ME_EVENT_CT_BYTES;
            if (probe_this_iter && (protocol == probe || protocol == two_probe)) {
                me.options |= PTL_ME_ACK_DISABLE;
            }
            me.match_bits = match_bits;
            me.ignore_bits = ignore_bits;

            DEBUG(("%02d: recv: posting ME of size %d (%d), match bits %lx\n", 
                   my_id, (int) me.length, (int) cur_len, (unsigned long) match_bits));
            ptl->PtlMEAppend(send_pt,
                             me,
                             PTL_PRIORITY_LIST,
                             NULL,
                             recv_me_h);
            crReturn();
            crFuncEnd();

            crFuncStart(recv_wait);
            while (true) {
                while (!ptl->PtlEQWait(recv_eq_h, &ev)) { crReturn(); }
                if (ev.type == PTL_EVENT_PUT) {
                    if (!PTL_IS_SHORT_MSG(ev.match_bits) && (protocol == probe) && probe_this_iter) {
                        ptl_md_t md;
                        md.start = recv_buf;
                        md.length = cur_len;
                        md.options = 0;
                        md.eq_handle = recv_eq_h;
                        md.ct_handle = PTL_CT_NONE;

                        ptl->PtlMDBind(md, &recv_md_h);
                        crReturn();

                        DEBUG(("%02d: recv: posting probe long expected get\n", my_id));
                        ptl->PtlGet(recv_md_h,
                                    0,
                                    cur_len,
                                    ev.initiator,
                                    read_pt,
                                    ev.hdr_data,
                                    0,
                                    NULL);
                        crReturn();

                    } else if (!PTL_IS_SHORT_MSG(ev.match_bits) && (protocol == rndv)) {
                        ptl_md_t md;
                        md.start = recv_buf + eager_len;
                        md.length = (ev.hdr_data & 0xFFFFFFFFULL) - eager_len;
                        md.options = 0;
                        md.eq_handle = recv_eq_h;
                        md.ct_handle = PTL_CT_NONE;

                        ptl->PtlMDBind(md, &recv_md_h);
                        crReturn();

                        DEBUG(("%02d: recv: posting rndv long expected get\n", my_id));
                        ptl->PtlGet(recv_md_h,
                                    eager_len,
                                    (ev.hdr_data & 0xFFFFFFFFULL) - eager_len,
                                    ev.initiator,
                                    read_pt,
                                    ev.hdr_data,
                                    0,
                                    NULL);
                        crReturn();

                    } else if (!PTL_IS_SHORT_MSG(ev.match_bits) && 
                               (protocol == two || protocol == two_probe)) {
                        if (PTL_IS_MID_MSG(ev.match_bits)) {
                            if (protocol == two_probe && probe_this_iter) {
                                /* treat as unexpected */
                                ptl_md_t md;
                                md.start = recv_buf;
                                md.length = cur_len;
                                md.options = 0;
                                md.eq_handle = recv_eq_h;
                                md.ct_handle = PTL_CT_NONE;

                                ptl->PtlMDBind(md, &recv_md_h);
                                crReturn();

                                DEBUG(("%02d: recv: posting two_probe long expected get\n", my_id));
                                ptl->PtlGet(recv_md_h,
                                            0,
                                            cur_len,
                                            ev.initiator,
                                            read_pt,
                                            ev.hdr_data,
                                            0,
                                            NULL);
                                crReturn();
                                
                            } else {
                                /* mid-sized message with a put means totally delivered */
                                goto long_recv_done;
                            }
                        } else {
                            /* long message, rndv */
                            ptl_md_t md;
                            md.start = recv_buf + eager_len;
                            md.length = (ev.hdr_data & 0xFFFFFFFFULL) - eager_len;
                            md.options = 0;
                            md.eq_handle = recv_eq_h;
                            md.ct_handle = PTL_CT_NONE;

                            ptl->PtlMDBind(md, &recv_md_h);
                            crReturn();

                            DEBUG(("%02d: recv: posting two long expected get\n", my_id));
                            ptl->PtlGet(recv_md_h,
                                        eager_len,
                                        (ev.hdr_data & 0xFFFFFFFFULL) - eager_len,
                                        ev.initiator,
                                        read_pt,
                                        ev.hdr_data,
                                        0,
                                        NULL);
                            crReturn();
                        }

                    } else if (!PTL_IS_SHORT_MSG(ev.match_bits) && (protocol == triggered)) {
                        /* nothing to do here */
                    } else {
                        /* we're totally done */
                        goto long_recv_done;
                    }
                } else if (ev.type == PTL_EVENT_PUT_OVERFLOW) {
                    if (PTL_IS_SHORT_MSG(ev.match_bits)) {
                        /* don't actually copy, just assume it's there.  Add some copy time */
                        DEBUG(("%02d: recv: copying short unexpected\n", my_id));
                        long latency;
                        char timetmp[100];
                        latency = 60 + (cur_len * 1000000000 / 20000000000);
                        snprintf(timetmp, 100, "%ldns", latency);
                        cpu->addBusyTime(timetmp);
                        memcpy(recv_buf, ev.start, cur_len);
                        crReturn();
                        goto long_recv_done;
                    } else if (protocol != triggered) {
                        ptl_md_t md;
                        
                        if (protocol == eager || protocol == probe) {
                            send_len = ev.rlength;
                        } else if (protocol == rndv) {
                            send_len = ev.hdr_data & 0xFFFFFFFFULL;
                        } else if (protocol == two) {
                            if (PTL_IS_MID_MSG(ev.match_bits)) {
                                send_len = ev.rlength;                                
                            } else {
                                send_len = ev.hdr_data & 0xFFFFFFFFULL;
                            }
                        }

                        md.start = recv_buf;
                        md.length = (send_len > (unsigned) cur_len) ? cur_len : send_len;
                        md.options = 0;
                        md.eq_handle = recv_eq_h;
                        md.ct_handle = PTL_CT_NONE;

                        ptl->PtlMDBind(md, &recv_md_h);
                        crReturn();

                        DEBUG(("%02d: recv: posting long unexpected get\n", my_id));
                        ptl->PtlGet(recv_md_h,
                                    0,
                                    (send_len > (unsigned) cur_len) ? cur_len : send_len,
                                    ev.initiator,
                                    read_pt,
                                    ev.hdr_data,
                                    0,
                                    NULL);
                        crReturn();
                    }
                } else if (ev.type == PTL_EVENT_REPLY) {
                    DEBUG(("%02d: recv: processing reply event\n", my_id));
                    ptl->PtlMDRelease(recv_md_h);
                    crReturn();
                    if (protocol == triggered) {
                        ptl->PtlCTFree(ct_h);
                    }
                    /* we're totally done */
                    goto long_recv_done;
                } else if (ev.type == PTL_EVENT_AUTO_UNLINK) {
                    /* nothing to do */
                } else if (ev.type == PTL_EVENT_AUTO_FREE) {
                    DEBUG(("%02d: recv: reposting short buffer\n", my_id));
                    need_repost = ev.user_ptr;
                    crFuncCall(short_msg_repost);
                } else {
                    printf("%02d: recv: Unexpected event type in recv protocol: %d\n", my_id, (int) ev.type);
                }
            }
    long_recv_done:
        crFuncEnd();

        crFuncStart(barrier);
        DEBUG(("%02d: barrier start\n", my_id));
        cur_len = 0;
        if (my_id == 0) {
            peer = 1;
            tag = 2;
            crFuncCall(recv);
            crFuncCall(recv_wait);
            crFuncCall(send);
            crFuncCall(send_wait);
        } else if (my_id == 1) {
            peer = 0;
            tag = 2;
            crFuncCall(send);
            crFuncCall(send_wait);
            crFuncCall(recv);
            crFuncCall(recv_wait);
        }
        DEBUG(("%02d: barrier end\n", my_id));
        crFuncEnd();

        crFuncStart(send_delay);
        {
            long latency;
            char timetmp[100];
            latency = send_delay * run_times[cur_len] * 1000000000;
            snprintf(timetmp, 100, "%ldns", latency);
            cpu->addBusyTime(timetmp);
        }
        crFuncEnd();

        crFuncStart(recv_delay);
        {
            long latency;
            char timetmp[100];
            latency = recv_delay * run_times[cur_len] * 1000000000;
            snprintf(timetmp, 100, "%ldns", latency);
            cpu->addBusyTime(timetmp);
        }
        crFuncEnd();

        crFuncStart(bw_test);
        /* set tlast to ~1.5us, which is about what our 1 byte latency
           will be.  Gives a good starting point for nrepeat without
           actually doing the test */
        tlast = 1.5 / 1000000.0;

        /* the real loop */
        inc = (start_len > 1) ? start_len / 2 : 1;
        nq = (start_len > 1) ? 1 : 0;

        for (n = 0, len = start_len ; len <= stop_len ; len += inc, nq++) {
            if (nq > 2) inc = ((nq % 2))? inc + inc: inc;
            for (pert = ((perturbation > 0) && (inc > perturbation+1)) ? -perturbation : 0;
                 pert <= perturbation; 
                 n++, pert += ((perturbation > 0) && (inc > perturbation+1)) ? perturbation : perturbation+1) {

                cur_len = len + pert;
                /* tlast is shared at the bottom, so is always the same on both nodes */
                nrepeat = MAX((.005 / (((double) cur_len / MAX(1, (cur_len - inc + 1.0))) * tlast)), 8);

                crFuncCall(barrier);

                /* fix me; barrier resets curr len */
                cur_len = len + pert;
                start_time = cpu->getCurrentSimTimeNano();
                for (i = 0 ; i < nrepeat ; ++i) {
                    if (my_id == 0) {
                        peer = 1;
                        tag = 1;
                        DEBUG(("00: send: start to 01\n"));
                        crFuncCall(send);
                        crFuncCall(send_delay);
                        crFuncCall(send_wait);
                        DEBUG(("00: recv: start from 01\n"));
                        crFuncCall(recv);
                        crFuncCall(recv_delay);
                        crFuncCall(recv_wait);
                    } else if (my_id == 1) {
                        peer = 0;
                        tag = 1;
                        DEBUG(("01: recv: start from 00\n"));
                        crFuncCall(recv);
                        crFuncCall(recv_delay);
                        crFuncCall(recv_wait);
                        DEBUG(("01: send: start to 00\n"));
                        crFuncCall(send);
                        crFuncCall(send_delay);
                        crFuncCall(send_wait);
                    }
                }
                crReturn();
                stop_time = cpu->getCurrentSimTimeNano();
                
                tlast = ((double) (stop_time - start_time)) / 1000000000.0 / nrepeat / 2;
                cur_len = sizeof(tlast);
                if (my_id == 0) {
                    peer = 1;
                    send_buf = (char*) &tlast;
                    crFuncCall(send);
                    crFuncCall(send_wait);
                    send_buf = big_send_buf;
                } else if (my_id == 1) {
                    peer = 0;
                    recv_buf = (char*) &tlast;
                    crFuncCall(recv);
                    crFuncCall(recv_wait);
                    recv_buf = big_recv_buf;
                }
                cur_len = len + pert;
                if (0.0 == cur_delay) { 
                    run_times[cur_len] = tlast;
                }

                if (my_id == 0) {
                    printf("%3d: %7d bytes %6d times --> %8.2lf MBps in %10.2lf usec\n", 
                           n, (int) cur_len, nrepeat, cur_len / tlast / 1024 / 1024, 
                           tlast * 1000000);
                    fprintf(fp, "%8d\t%lf\t%12.8lf\n", (int) cur_len, cur_len / tlast / 1024 / 1024, tlast);
                    fflush(NULL);
                }
            }
        }
        crFuncEnd();

        crStart();
        
        ptl->PtlEQAlloc(1024, &send_eq_h);
        crReturn();
        ptl->PtlEQAlloc(1024, &recv_eq_h);
        crReturn();

        ptl->PtlPTAlloc(0, recv_eq_h, 0, &send_pt);
        crReturn();
        ptl->PtlPTAlloc(0, send_eq_h, 1, &read_pt);
        crReturn();

        /* register long truncate */
        if (protocol != triggered) {
            ptl_me_t me;
            me.start = 0;
            me.length = 0;
            me.ct_handle = PTL_CT_NONE;
            me.min_free = 0;
            me.options = PTL_ME_OP_PUT | PTL_ME_ACK_DISABLE;
            me.match_bits = PTL_LONG_MSG;
            me.ignore_bits = PTL_CONTEXT_MASK | PTL_SOURCE_MASK | PTL_TAG_MASK;
            ptl->PtlMEAppend(send_pt,
                             me,
                             PTL_OVERFLOW,
                             0,
                             overflow_me_h);
            crReturn();

            if (protocol == two || protocol == two_probe) {
                ptl_me_t me;
                me.start = 0;
                me.length = 0;
                me.ct_handle = PTL_CT_NONE;
                me.min_free = 0;
                me.options = PTL_ME_OP_PUT | PTL_ME_ACK_DISABLE;
                me.match_bits = PTL_MID_MSG;
                me.ignore_bits = PTL_CONTEXT_MASK | PTL_SOURCE_MASK | PTL_TAG_MASK;
                ptl->PtlMEAppend(send_pt,
                                 me,
                                 PTL_OVERFLOW,
                                 0,
                                 overflow_mid_me_h);
                crReturn();
            }
        }

        crFuncCall(short_msg_init);

        if (protocol == probe || protocol == two_probe) {
            for (probe_freqs_iter = probe_freqs.begin() ; 
                 probe_freqs_iter != probe_freqs.end() ; 
                 ++probe_freqs_iter) {
                probe_freq = *probe_freqs_iter;
                cur_delay = send_delay = recv_delay = 0.0;

                sprintf(cur_filename, "%s-%d-nodelay.out", base_filename, probe_freq);
                fp = fopen(cur_filename, "w");
                crFuncCall(bw_test);

                for (delays_iter = delays.begin() ; delays_iter != delays.end() ; ++delays_iter) {
                    cur_delay = *delays_iter;

                    send_delay = 0.0; recv_delay = cur_delay;
                    sprintf(cur_filename, "%s-%d-recv-%1.2lf.out", base_filename, probe_freq, recv_delay);
                    fclose(fp); fp = fopen(cur_filename, "w");
                    crFuncCall(bw_test);

                    send_delay = cur_delay; recv_delay = 0.0;
                    sprintf(cur_filename, "%s-%d-send-%1.2lf.out", base_filename, probe_freq, send_delay);
                    fclose(fp); fp = fopen(cur_filename, "w");
                    crFuncCall(bw_test);
                }

            }

        } else {
            cur_delay = send_delay = recv_delay = 0.0;
            sprintf(cur_filename, "%s-nodelay.out", base_filename);
            fp = fopen(cur_filename, "w");
            crFuncCall(bw_test);

            for (delays_iter = delays.begin() ; delays_iter != delays.end() ; ++delays_iter) {
                cur_delay = *delays_iter;

                send_delay = 0.0; recv_delay = cur_delay;
                sprintf(cur_filename, "%s-recv-%1.2lf.out", base_filename, recv_delay);
                fclose(fp); fp = fopen(cur_filename, "w");
                crFuncCall(bw_test);

                send_delay = cur_delay; recv_delay = 0.0;
                sprintf(cur_filename, "%s-send-%1.2lf.out", base_filename, send_delay);
                fclose(fp); fp = fopen(cur_filename, "w");
                crFuncCall(bw_test);
            }
        }

        crFinish();
        return true;
    }

private:
    pingpong_bw();
    pingpong_bw(const application& a);
    void operator=(pingpong_bw const&);

    portals *ptl;

    SimTime_t start_time, stop_time;
    int i, n, len, start_len, stop_len, inc, nq, pert, perturbation, nrepeat, peer, handle;
    char *send_buf, *recv_buf;
    char *big_send_buf, *big_recv_buf;
    double tlast;
    unsigned long cur_len, send_count, recv_count, eager_len, long_len;
    unsigned long send_len;

    ptl_pt_index_t send_pt, read_pt;
    ptl_handle_eq_t send_eq_h, recv_eq_h;
    ptl_handle_md_t send_md_h, recv_md_h;
    ptl_handle_me_t send_me_h, recv_me_h, overflow_me_h, overflow_mid_me_h;
    int contextid, tag;
    void *need_repost;
    ptl_match_bits_t match_bits, ignore_bits;
    ptl_event_t ev;
    ptl_handle_ct_t ct_h;
    int probe_freq;
    std::vector<int> probe_freqs;
    std::vector<int>::iterator probe_freqs_iter;
    int probe_this_iter;

    FILE *fp;

    enum { eager, rndv, triggered, probe, two, two_probe } protocol;

    std::vector<double> delays;
    std::vector<double>::iterator delays_iter;
    std::map<long, double> run_times;
    double send_delay, recv_delay, cur_delay;
    char base_filename[100], cur_filename[100];
};

}
}
#endif // COMPONENTS_TRIG_CPU_PINGPONG_BW_H
