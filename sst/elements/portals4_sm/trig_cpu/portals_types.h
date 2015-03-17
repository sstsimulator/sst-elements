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


#ifndef COMPONENTS_TRIG_CPU_PORTALS_TYPES_H
#define COMPONENTS_TRIG_CPU_PORTALS_TYPES_H

#include <list>

#include "sst/sst_stdint.h"
#include <stdio.h>

namespace SST {
namespace Portals4_sm {

#define PACKET_SIZE 64

typedef uint32_t  ptl_size_t;

typedef int16_t   ptl_handle_ct_t;
typedef int16_t   ptl_handle_eq_t;
typedef int8_t    ptl_pt_index_t;
typedef uint32_t  ptl_ack_req_t;

#define PTL_ACK_REQ     0
#define PTL_NO_ACK_REQ  1
#define PTL_CT_ACK_REQ  2
#define PTL_OC_ACK_REQ  3

#define PTL_UNDEF (~0)
#define PTL_PTR_UNDEF (NULL)

// typedef enum {PTL_ACK_REQ,
// 	      PTL_NO_ACK_REQ,
// 	      PTL_CT_ACK_REQ,
// 	      PTL_OC_ACK_REQ
// } ptl_ack_req_t;



typedef uint64_t  ptl_process_t;
typedef uint64_t  ptl_time_t;
typedef uint64_t  ptl_hdr_data_t;
typedef uint64_t  ptl_match_bits_t;

typedef uint32_t  ptl_uid_t;
typedef uint32_t  ptl_jid_t;
typedef uint8_t   ptl_ni_fail_t;


typedef struct {
    ptl_size_t success;
    ptl_size_t failure;
} ptl_ct_event_t;

typedef enum {
    PTL_CT_OPERATION, PTL_CT_BYTE
} ptl_ct_type_t;

// typedef enum {
//     PTL_MIN, PTL_MAX,
//     PTL_SUM, PTL_PROD,
//     PTL_LOR, PTL_LAND,
//     PLT_BOR, PTL_BAND,
//     PTL_LXOR, PTL_BXOR,
//     PTL_SWAP, PTL_CSWAP, PTL_MSWAP
// } ptl_op_t;

#define PTL_MIN    0
#define PTL_MAX    1
#define PTL_SUM    2
#define PTL_PROD   3
#define PTL_LOR    4
#define PTL_LAND   5
#define PTL_BOR    6
#define PTL_BAND   7
#define PTL_LXOR   8
#define PTL_BXOR   9
#define PTL_SWAP  10
#define PTL_CSWAP 11
#define PTL_MSWAP 12

typedef uint8_t ptl_op_t;

// typedef enum {
//     PTL_CHAR, PTL_UCHAR,
//     PTL_SHORT, PTL_USHORT,
//     PTL_INT, PTL_UINT,
//     PTL_LONG, PTL_ULONG,
//     PTL_FLOAT, PTL_DOUBLE
// } ptl_datatype_t;

#define PTL_CHAR     0
#define PTL_UCHAR    1
#define PTL_SHORT    2
#define PTL_USHORT   3
#define PTL_INT      4
#define PTL_UINT     5
#define PTL_LONG     6
#define PTL_ULONG    7
#define PTL_FLOAT    8
#define PTL_DOUBLE   9

typedef uint8_t ptl_datatype_t;

typedef enum {
    PTL_EVENT_GET,
    PTL_EVENT_PUT,
    PTL_EVENT_PUT_OVERFLOW,
    PTL_EVENT_ATOMIC,
    PTL_EVENT_ATOMIC_OVERFLOW,
    PTL_EVENT_REPLY,
    PTL_EVENT_SEND,
    PTL_EVENT_ACK,
    PTL_EVENT_PT_DISABLED,
    PTL_EVENT_AUTO_UNLINK,
    PTL_EVENT_AUTO_FREE,
    PTL_EVENT_PROBE
} ptl_event_kind_t;

struct ptl_event_t {
    ptl_event_kind_t       type;
    ptl_process_t          initiator;
    ptl_pt_index_t         pt_index;
    ptl_uid_t              uid;
    ptl_jid_t              jid;
    ptl_match_bits_t       match_bits;
    ptl_size_t             rlength;
    ptl_size_t             mlength;
    ptl_size_t             remote_offset;
    void*                  start;
    void*                  user_ptr;
    ptl_hdr_data_t         hdr_data;
    ptl_ni_fail_t          ni_fail_type;
    ptl_op_t               atomic_operation;
    ptl_datatype_t         atomic_type;

    void print() {
	switch ( type ) {
	case PTL_EVENT_GET: printf("  type: PTL_EVENT_GET\n"); break;
	case PTL_EVENT_PUT: printf("  type: PTL_EVENT_PUT\n"); break;
	case PTL_EVENT_PUT_OVERFLOW: printf("  type: PTL_EVENT_PUT_OVERFLOW\n"); break;
	case PTL_EVENT_ATOMIC: printf("  type: PTL_EVENT_ATOMIC\n"); break;
	case PTL_EVENT_ATOMIC_OVERFLOW: printf("  type: PTL_EVENT_ATOMIC_OVERFLOW\n"); break;
	case PTL_EVENT_REPLY: printf("  type: PTL_EVENT_REPLY\n"); break;
	case PTL_EVENT_SEND: printf("  type: PTL_EVENT_SEND\n"); break;
	case PTL_EVENT_ACK: printf("  type: PTL_EVENT_ACK\n"); break;
	case PTL_EVENT_PT_DISABLED: printf("  type: PTL_EVENT_PT_DISABLED\n"); break;
	case PTL_EVENT_AUTO_UNLINK: printf("  type: PTL_EVENT_AUTO_UNLINK\n"); break;
	case PTL_EVENT_AUTO_FREE: printf("  type: PTL_EVENT_AUTO_FREE\n"); break;
	case PTL_EVENT_PROBE: printf("  type: PTL_EVENT_PROBE\n"); break;
	}
	
	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK || type == PTL_EVENT_AUTO_UNLINK ||
	     type == PTL_EVENT_AUTO_FREE ) printf("  initiator: N/A\n");
	else printf("  initiator: %lu\n", (long unsigned) initiator);

	
	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK ) printf("  pt_index: N/A\n");
	else printf("  pt_index: %d\n",pt_index);

// 	printf("  uid: %d\n",uid);
// 	printf("  jid: %d\n",jid);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK || type == PTL_EVENT_AUTO_UNLINK ||
	     type == PTL_EVENT_AUTO_FREE ) printf("  match_bits: N/A\n");
	else printf("  match_bits: %lx\n", (long) match_bits);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK || type == PTL_EVENT_AUTO_UNLINK ||
	     type == PTL_EVENT_AUTO_FREE ) printf("  rlength: N/A\n");
	else printf("  rlength: %d\n",rlength);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_AUTO_UNLINK || type == PTL_EVENT_AUTO_FREE) printf("  mlength: N/A\n");
	else printf("  mlength: %d\n",mlength);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_AUTO_UNLINK || type == PTL_EVENT_AUTO_FREE) printf("  remote_offset: N/A\n");
	else printf("  remote_offset: %d\n",remote_offset);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK || type == PTL_EVENT_AUTO_UNLINK ||
	     type == PTL_EVENT_AUTO_FREE ) printf("  start: N/A\n");
	else printf("  start: %p\n",start);

	printf("  user_ptr: %p\n",user_ptr);

	if ( type == PTL_EVENT_REPLY || type == PTL_EVENT_SEND ||
	     type == PTL_EVENT_ACK || type == PTL_EVENT_AUTO_UNLINK ||
	     type == PTL_EVENT_AUTO_FREE || type == PTL_EVENT_GET ) printf("  hdr_data: N/A\n");	
	else printf("  hdr_data: %lu\n", (unsigned long) hdr_data);

	if ( type != PTL_EVENT_ATOMIC && type != PTL_EVENT_ATOMIC_OVERFLOW ) printf("  atomic_operation: N/A\n");
	else printf("  atomic_operation: %d\n",atomic_operation);

	if ( type != PTL_EVENT_ATOMIC && type != PTL_EVENT_ATOMIC_OVERFLOW ) printf("  atomic_type: N/A\n");
	else printf("  atomic_type: %d\n",atomic_type);	
    }
};
    

// Operation types
#define PTL_OP_PUT      0
#define PTL_OP_GET      1
#define PTL_OP_GET_RESP 2
#define PTL_OP_ATOMIC   3
#define PTL_OP_CT_INC   4
#define PTL_OP_ACK      5

#define PTL_EQ_NONE (-1)
#define PTL_CT_NONE (-1)

typedef enum { 
    PTL_PRIORITY_LIST, PTL_OVERFLOW, PTL_PROBE_ONLY 
} ptl_list_t;


// Here's the MD
typedef struct { 
    void *start;
    ptl_size_t length;
    unsigned int options;  // not used
    ptl_handle_eq_t eq_handle; 
    ptl_handle_ct_t ct_handle; 
} ptl_md_t;

// Options for ME
#define PTL_MD_EVENT_CT_SEND   0x1
#define PTL_MD_EVENT_CT_REPLY  0x2
#define PTL_MD_EVENT_CT_ACK    0x4
#define PTL_MD_EVENT_CT_BYTES  0x8

typedef ptl_md_t* ptl_handle_md_t;


// Here's the ME
typedef struct { 
    void *start; 
    ptl_size_t length; 
    ptl_handle_ct_t ct_handle; 
    ptl_size_t min_free;
    //  ptl_ac_id_t ac_id;
    uint32_t options; 
    //  ptl_process_t match_id; 
    ptl_match_bits_t match_bits; 
    ptl_match_bits_t ignore_bits; 
} ptl_me_t;

// Options for the ME
#define PTL_ME_OP_PUT                     0x1
#define PTL_ME_OP_GET                     0x2
#define PTL_ME_MANAGE_LOCAL               0x4
#define PTL_ME_NO_TRUNCATE                0x8
#define PTL_ME_USE_ONCE                  0x10
#define PTL_ME_MAY_ALIGN                 0x20
#define PTL_ME_ACK_DISABLE               0x40
#define PTL_IOVEC                        0x80
#define PTL_ME_EVENT_COMM_DISABLE       0x100
#define PTL_ME_EVENT_FLOWCTRL_DISABLE   0x200
#define PTL_ME_EVENT_SUCCESS_DISABLE    0x400
#define PTL_ME_EVENT_OVER_DISABLE       0x800
#define PTL_ME_EVENT_UNLINK_DISABLE    0x1000
#define PTL_ME_EVENT_CT_COMM           0x2000
#define PTL_ME_EVENT_CT_OVERFLOW       0x4000
#define PTL_ME_EVENT_CT_BYTES          0x8000
#define PTL_ME_EVENT_CT_USE_JID       0x10000

#define PTL_OK 0
#define PTL_EQ_EMPTY 1
#define PTL_TIME_FOREVER (-1)

// Internal data structures
typedef uint32_t ptl_op_type_t;

typedef struct {
    ptl_me_t me;
    bool active;
    void *user_ptr;
    ptl_handle_eq_t eq_handle;
    ptl_pt_index_t pt_index;
    ptl_list_t ptl_list;
    ptl_size_t managed_offset;
    ptl_size_t header_count;
} ptl_int_me_t;

typedef ptl_int_me_t* ptl_handle_me_t;

typedef struct {
    void* start;
    ptl_size_t length;
    ptl_size_t offset;
    ptl_process_t target_id;
    ptl_handle_ct_t ct_handle;
    ptl_size_t ct_increment;
    ptl_handle_eq_t eq_handle;
    ptl_event_t* event;
    bool end;
    int stream;
} ptl_int_dma_t;

// typedef struct {
//     ptl_pt_index_t   pt_index;       // 1 bytes
//     uint8_t          op;             // 1 bytes
//     ptl_op_t         atomic_op;      // 1 bytes
//     ptl_datatype_t   atomic_datatype;// 1 bytes
//     ptl_handle_ct_t  get_ct_handle;  // 2 bytes
//     uint16_t         options;        // 2 bytes
//     ptl_match_bits_t match_bits;     // 8 bytes
//     uint32_t         length;         // 4 bytes
//     ptl_size_t       remote_offset;  // 4 bytes
//     void *           get_start;      // 8 bytes
//     uint64_t         header_data;    // 8 bytes
// } ptl_header_t;

typedef struct {
    ptl_pt_index_t   pt_index;       // 1 bytes
    uint8_t          op;             // 1 bytes
    ptl_op_t         atomic_op;      // 1 bytes
    ptl_datatype_t   atomic_datatype;// 1 bytes
    uint16_t         out_msg_index;  // 2 bytes
//     ptl_handle_ct_t  get_ct_handle;  // 2 bytes
    uint16_t         options;        // 2 bytes
    ptl_match_bits_t match_bits;     // 8 bytes
    uint32_t         length;         // 4 bytes
    ptl_size_t       remote_offset;  // 4 bytes
//     void *           get_start;      // 8 bytes
    uint64_t         header_data;    // 8 bytes
} __attribute__((__may_alias__)) ptl_header_t;

typedef struct {
    ptl_header_t header;
    ptl_int_me_t* me;
    ptl_size_t offset;
    ptl_size_t mlength;
    ptl_process_t src;
} ptl_int_header_t;

typedef struct {
    ptl_op_type_t op_type;
    ptl_process_t target_id;
    ptl_pt_index_t pt_index;
    ptl_match_bits_t match_bits;
    ptl_handle_ct_t ct_handle;
    ptl_size_t increment;
    int8_t           atomic_op;
    int8_t           atomic_datatype;
    ptl_int_dma_t* dma;
    ptl_header_t* ptl_header;
} ptl_int_op_t;

typedef struct {
    void*            user_ptr;       // 8 bytes
    void*            get_start;      // 8 bytes
    ptl_handle_ct_t  ct_handle;      // 2 bytes
    ptl_handle_ct_t  eq_handle;      // 2 bytes
    uint8_t          op;             // 1 byte
    ptl_ack_req_t    ack_req;         // 4 bytes
} ptl_int_msg_info_t;

typedef struct {
    ptl_size_t threshold;
    ptl_handle_ct_t trig_ct_handle;
    ptl_int_op_t* op;
    ptl_int_msg_info_t* msg_info;
} ptl_int_trig_op_t;

// Structs, etc needed by internals
typedef std::list<ptl_int_me_t*> me_list_t;
typedef std::list<ptl_int_trig_op_t*> trig_op_list_t;
typedef std::list<ptl_int_header_t*> overflow_header_list_t;

typedef struct {
    bool allocated;
    ptl_ct_event_t ct_event;
    ptl_ct_type_t ct_type;
    trig_op_list_t trig_op_list;
} ptl_int_ct_t;

typedef struct {
    ptl_ct_event_t ct_event;
    ptl_handle_ct_t ct_handle;
} ptl_update_ct_event_t;
  
typedef struct {
    me_list_t* priority_list;
    me_list_t* overflow;
    overflow_header_list_t* overflow_headers;
    
} ptl_entry_t;

// Data structure to pass stuff to the NIC
typedef enum {
    PTL_NO_OP, PTL_DMA, PTL_DMA_RESPONSE,
    PTL_CREDIT_RETURN,
    PTL_NIC_PROCESS_MSG,
    PTL_NIC_ME_APPEND_PRIORITY, PTL_NIC_ME_APPEND_OVERFLOW,
    PTL_NIC_TRIG, PTL_NIC_TRIG_PUTV,
    PTL_NIC_PROCESS_TRIG, PTL_NIC_POST_CT,
    PTL_NIC_CT_SET, PTL_NIC_CT_INC, PTL_NIC_EQ,
    PTL_NIC_UPDATE_CPU_CT, PTL_NIC_INIT_FOR_SEND_RECV,
    PTL_ACK
} ptl_int_nic_op_type_t;


typedef struct {
    ptl_int_nic_op_type_t op_type;
    union {
	ptl_int_me_t* me;
	ptl_int_trig_op_t* trig;
        ptl_update_ct_event_t* ct;
	ptl_handle_ct_t ct_handle;
	ptl_int_dma_t* dma;
    } data;
} ptl_int_nic_op_t;

typedef struct {
    ptl_handle_ct_t ct_handle;
    ptl_size_t success;
    ptl_size_t failure;
    bool clear_op_list;
} ptl_int_ct_alloc_t;


// defines to put flags in the header flit of the packet
#define PTL_HDR_PORTALS             0x1
#define PTL_HDR_HEAD_PACKET         0x2
#define PTL_HDR_STREAM_PIO          0x10000000
#define PTL_HDR_STREAM_DMA          0x20000000
#define PTL_HDR_STREAM_TRIG         0x30000000
#define PTL_HDR_STREAM_GET          0x40000000
#define PTL_HDR_STREAM_TRIG_ATOMIC  0x50000000

}
}

#endif // COMPONENTS_TRIG_CPU_PORTALS_TYPES_H
