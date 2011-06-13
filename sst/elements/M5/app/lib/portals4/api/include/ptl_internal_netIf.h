#ifndef PTL_INTERNAL_NETIF_H
#define PTL_INTERNAL_NETIF_H

#include <portals4_types.h>
#include <ptl_internal_handles.h>

#define NET_IF_NAME_LEN 32
#define NUM_PTL_NI ( 1 << HANDLE_NI_BITS )

struct PtlAPI {
    int (*PtlNIInit)(
                struct PtlAPI*,
                unsigned int      options,
                ptl_pid_t         pid,
                ptl_ni_limits_t   *desired,
                ptl_ni_limits_t   *actual,
                ptl_size_t        map_size,
                ptl_process_t     *desired_mapping,
                ptl_process_t     *actual_mapping
    );
    int (*PtlNIFini)( struct PtlAPI* );

    int (*PtlPTAlloc)(
                struct PtlAPI*,
                unsigned int     options,
                ptl_handle_eq_t  eq_handle,
                ptl_pt_index_t   pt_index_req
    );
    int (*PtlPTFree)( struct PtlAPI*, ptl_pt_index_t );
    int (*PtlPTDisable)( struct PtlAPI*, ptl_pt_index_t );
    int (*PtlPTEnable)( struct PtlAPI*, ptl_pt_index_t );

    int (*PtlMDBind)( struct PtlAPI*, ptl_md_t* );
    int (*PtlMDRelease)( struct PtlAPI*, ptl_handle_md_t );

    int (*PtlMEAppend)( struct PtlAPI*,
                ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void *              user_ptr );
    int (*PtlMEUnlink)( struct PtlAPI*, ptl_handle_me_t me_handle);

    int (*PtlGetId)( struct PtlAPI*, ptl_process_t* id );

    int (*PtlCTAlloc)( struct PtlAPI* );
    int (*PtlCTFree)( struct PtlAPI*, ptl_handle_ct_t );
    int (*PtlCTWait)( struct PtlAPI*, 
            ptl_handle_ct_t   ct_handle,
              ptl_size_t        test,
              ptl_ct_event_t *  event);

    int (*PtlPut)(struct PtlAPI*,
            ptl_handle_md_t  md_handle,
            ptl_size_t       local_offset,
            ptl_size_t       length,
            ptl_ack_req_t    ack_req,
            ptl_process_t    target_id,
            ptl_pt_index_t   pt_index,
            ptl_match_bits_t match_bits,
            ptl_size_t       remote_offset,
            void *           user_ptr,
            ptl_hdr_data_t   hdr_data);

    int (*PtlEQAlloc)( struct PtlAPI*, ptl_size_t );
    int (*PtlEQFree)( struct PtlAPI*, ptl_handle_eq_t ); 
    int (*PtlEQWait)( struct PtlAPI*, ptl_handle_eq_t eq_handle,
              ptl_event_t * event);


    void* data;
};

struct PtlIF {
    struct PtlAPI* (*init)( struct PtlIF* );
    void (*fini)( struct PtlAPI* );
    void *data;
};

struct NetIF {
    struct PtlIF* (*init)(void);
    void (*fini)( struct PtlIF* );
    void *data;
};

typedef struct {
    char            name[NET_IF_NAME_LEN];
    struct NetIF*   netIF;
    struct PtlIF*   ptlIF;
    struct PtlAPI*  ptlAPI[ NUM_PTL_NI ];
} NetIFEntry;

extern NetIFEntry ifTable[]; 

static inline struct PtlAPI* GetPtlAPI( ptl_internal_handle_converter_t ni ) {
    return ifTable[ni.s.iface].ptlAPI[ni.s.ni]; 
}

#ifdef __cplusplus
extern "C" {
#endif
void ptl_internal_register_netIF( const char* const name, struct NetIF* );
#ifdef __cplusplus
}
#endif

#endif
