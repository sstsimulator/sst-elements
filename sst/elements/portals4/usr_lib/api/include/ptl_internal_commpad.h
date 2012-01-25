#ifndef PTL_INTERNAL_COMMPAD_H
#define PTL_INTERNAL_COMMPAD_H

#include <portals4.h>

#include <stddef.h>                    /* for size_t */

extern size_t            proc_number;
extern size_t            num_siblings;

#if 0
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include "ptl_internal_handles.h"

extern volatile uint8_t *comm_pad;
extern size_t            per_proc_comm_buf_size;
extern size_t            firstpagesize;

#define HDR_TYPE_PUT         0         /* _____ */
#define HDR_TYPE_GET         1         /* ____1 */
#define HDR_TYPE_ATOMIC      2         /* ___1_ */
#define HDR_TYPE_FETCHATOMIC 3         /* ___11 */
#define HDR_TYPE_SWAP        4         /* __1__ */
#define HDR_TYPE_CMD         5         /* __1_1 */
#define HDR_TYPE_ACKFLAG     8         /* _1___ */
#define HDR_TYPE_ACKMASK     23        /* 1_111 */
#define HDR_TYPE_TRUNCFLAG   16        /* 1____ */
#define HDR_TYPE_BASICMASK   7         /* __111 */
#define HDR_TYPE_TERM        31        /* 11111 */

enum cmd_types {
    CMD_TYPE_CTFREE,
    CMD_TYPE_CHECK,
    CMD_TYPE_ENQUEUE,
};

typedef struct {
    void *volatile   next;
    ptl_match_bits_t match_bits;
    void            *user_ptr;
    ptl_hdr_data_t   hdr_data;                // not used by GETs
#ifdef STRICT_UID_JID
    ptl_jid_t        jid;
#endif
    /* data used for long & truncated messages */
    char                           *moredata;
    void                           *entry;
    uint32_t                        remaining;
    /* data used for GETs and properly processing events */
    ptl_internal_handle_converter_t md_handle1;
    ptl_internal_handle_converter_t md_handle2;
    uint32_t                        length;
    uint16_t                        src;
    uint16_t                        target;
    uint64_t                        local_offset1    : 48;
    uint64_t                        local_offset2    : 48;
    uint64_t                        dest_offset      : 48;
    uint8_t                         pt_index         : 6; // only need 5
    ptl_ack_req_t                   ack_req          : 2; // only used by PUTs and ATOMICs
    unsigned char                   type             : 5; // 0=put, 1=get, 2=atomic, 3=fetchatomic, 4=swap
    unsigned char                   ni               : 2;
    uint8_t                         atomic_operation : 5;
    uint8_t                         atomic_datatype  : 4;
    uint8_t                         data[];
} ptl_internal_header_t;

typedef struct {
    ptl_internal_header_t hdr;
    void                 *unexpected_entry;
    void                 *buffered_data;
} ptl_internal_buffered_header_t;
#endif

#endif /* ifndef PTL_INTERNAL_COMMPAD_H */
/* vim:set expandtab: */
