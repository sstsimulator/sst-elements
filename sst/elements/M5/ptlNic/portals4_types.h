/*!
 * @file portals4.h
 * @brief Portals4 Reference Implementation
 *
 * Copyright (c) 2010 Sandia Corporation
 */

#ifndef PORTALS4_TYPES_H
#define PORTALS4_TYPES_H

#include <stdint.h>

#define MAX_PT_INDEX 63
#define MAX_CTS 63
#define MAX_MDS 63
#define MAX_ENTRIES 63
#define MAX_EQS 63
#define MAX_LIST_SIZE 63

/*****************
 * Return Values *
 *****************/
/*! The set of all possible return codes. */
enum ptl_retvals {
    PTL_OK=0,           /*!< Indicates success */
    PTL_ARG_INVALID,    /*!< One of the arguments is invalid. */
    PTL_CT_NONE_REACHED, /*!< Timeout reached before any counting event reached
                           the test. */
    PTL_EQ_DROPPED,     /*!< At least one event has been dropped. */
    PTL_EQ_EMPTY,       /*!< No events available in an event queue. */
    PTL_FAIL,           /*!< Indicates a non-specific error */
    PTL_IN_USE,         /*!< The specified resource is currently in use. */
    PTL_INTERRUPTED,    /*!< Wait/get operation was interrupted. */
    PTL_LIST_TOO_LONG,  /*!< The resulting list is too long (interface-dependent). */
    PTL_NI_NOT_LOGICAL, /*!< Not a logically addressed network interface handle. */
    PTL_NO_INIT,        /*!< Init has not yet completed successfully. */
    PTL_NO_SPACE,       /*!< Sufficient memory for action was not available. */
    PTL_PID_IN_USE,     /*!< PID is in use. */
    PTL_PT_FULL,        /*!< Portal table has no empty entries. */
    PTL_PT_EQ_NEEDED,   /*!< Flow control is enabled and there is no EQ provided. */
    PTL_PT_IN_USE,      /*!< Portal table index is busy. */
    PTL_SIZE_INVALID    /*!< The requested map size is invalid. */
};

/**************
 * Base Types *
 **************/
typedef uint64_t        ptl_size_t; /*!< Unsigned 64-bit integral type used for
                                      representing sizes. */
typedef unsigned char   ptl_pt_index_t; /*!< Integral type used for
                                          representing portal table indices. */
typedef uint64_t        ptl_match_bits_t; /*!< Capable of holding unsigned
                                            64-bit integer values. */
typedef uint64_t        ptl_hdr_data_t; /*!< 64 bits of out-of-band user data. */
typedef unsigned int    ptl_time_t; /*!< Time in milliseconds (used for timeout
                                      specification). */
#if 0
typedef unsigned int    ptl_interface_t; /*!< Integral type used for
                                           identifying different network
                                           interfaces. */
#endif
typedef const char*    ptl_interface_t; 
typedef uint32_t        ptl_nid_t; /*!< Integral type used for representing
                                     node identifiers. */
typedef uint32_t        ptl_pid_t; /*!< Integral type used for representing
                                     process identifiers when physical
                                     addressing is used in the network
                                     interface (PTL_NI_PHYSICAL is set). */
typedef uint32_t        ptl_rank_t; /*!< Integral type used for representing
                                      process identifiers when logical
                                      addressing is used in the network
                                      interface (PTL_NI_LOGICAL is set). */
typedef uint32_t        ptl_uid_t; /*!< Integral type for representing user
                                     identifiers. */
typedef uint32_t        ptl_jid_t; /*!< Integral type for representing job
                                     identifiers. */
typedef unsigned int    ptl_sr_index_t; /*!< Defines the types of indexes that
                                          can be used to access the status
                                          registers. */
typedef int             ptl_sr_value_t; /*!< Signed integral type that defines
                                          the types of values held in status
                                          registers. */
/* Handles */
typedef uint32_t        ptl_handle_any_t; /*!< generic handle */
typedef ptl_handle_any_t        ptl_handle_ni_t; /*!< A network interface handle */
typedef ptl_handle_any_t        ptl_handle_eq_t; /*!< An event queue handle */
typedef ptl_handle_any_t        ptl_handle_ct_t; /*!< A counting type event handle */
typedef ptl_handle_any_t        ptl_handle_md_t; /*!< A memory descriptor handle */
typedef ptl_handle_any_t        ptl_handle_le_t; /*!< A list entry handle */
typedef ptl_handle_any_t        ptl_handle_me_t; /*!< A match list entry handle */

/*!
 * @union ptl_process_t
 * @brief A union for representing processes either physically or logically.
 * The physical address uses two identifiers to represent the process
 * identifier: a node identifier \a nid and a process identifier \a pid. In
 * turn, a logical address uses a logical index within a translation table
 * specified by the application (the \a rank) to identify another process.
 * @ingroup PI
 */
typedef union {
    ptl_rank_t rank;    /*!< The logical representation of a node. */
    struct {
        ptl_nid_t nid;  /*!< The node identifier. */
        ptl_pid_t pid;  /*!< The process identifier. */
    } phys;             /*!< The physical representation of a node. */
}                       ptl_process_t;
/*!
 * @struct ptl_md_t
 * @brief Defines the visible parts of a memory descriptor. Values of this type
 *      are used to initialize the memory descriptors.
 *
 *      A memory descriptor contains information about a region of a process'
 *      memory and optionally points to an event queue where information about
 *      the operations performed on the memory descriptor are recorded. Memory
 *      descriptors are initiator side resources that are used to encapsulate
 *      an association with a network interface (NI) with a description of a
 *      memory region. They provide an interface to register memory (for
 *      operating systems that require it) and to carry that information across
 *      multiple operations (an MD is persistent until released). PtlMDBind()
 *      is used to create a memory descriptor and PtlMDRelease() is used to
 *      unlink and release the resources associated with a memory descriptor.
 * @ingroup MD
 */
typedef struct {
    void *          start; /*!< Specify the starting address for the memory
                             region associated with the memory descriptor.
                             There are no alignment restrictions on the
                             starting address; although unaligned messages may
                             be slower (i.e. lower bandwidth and/or longer
                             latency) on some implementations. */
    ptl_size_t      length; /*!< Specifies the length of the memory region
                              associated with the memory descriptor. */
    unsigned int    options; /*!<
    Specifies the behavior of the memory descriptor. Options include the use of
    scatter/gather vectors and disabling of end events associated with this
    memory descriptor. Values for this argument can be constructed using a
    bitwise OR of the following values:
    - \c PTL_MD_EVENT_SUCCESS_DISABLE
    - \c PTL_MD_EVENT_CT_SEND
    - \c PTL_MD_EVENT_CT_REPLY
    - \c PTL_MD_EVENT_CT_ACK
    - \c PTL_MD_EVENT_CT_BYTES
    - \c PTL_MD_UNORDERED
    - \c PTL_MD_REMOTE_FAILURE_DISABLE
    - \c PTL_IOVEC
                               */
    ptl_handle_eq_t eq_handle; /*!< The event queue handle used to log the
                                 operations performed on the memory region. If
                                 this member is \c PTL_EQ_NONE, operations
                                 performed on this memory descriptor are not
                                 logged. */
    ptl_handle_ct_t ct_handle; /*!< A handle for counting type events
                                 associated with the memory region. If this
                                 argument is \c PTL_CT_NONE, operations
                                 performed on this memory descriptor are not
                                 counted. */
}                       ptl_md_t;
/*!
 * @struct ptl_iovec_t
 * @brief Used to describe scatter/gather buffers of a match list entry or
 *      memory descriptor in conjunction with the \c PTL_IOVEC option. The
 *      ptl_iovec_t type is intended to be a type definition of the \c struct
 *      \c iovec type on systems that already support this type.
 * @ingroup MD
 * @note Performance conscious users should not mix offsets (local or remote)
 *      with ptl_iovec_t. While this is a supported operation, it is unlikely
 *      to perform well in most implementations.
 * @note When a ptl_iovec_t is passed to a Portals operation (as part of a
 *      memory descriptor, list entry, or match list entry), it must be treated
 *      exactly like a data buffer. That is, it cannot be modified or freed
 *      until Portals indicates that it has completed the operations on that
 *      buffer.
 * @implnote The implementation is required to support the mixing of the
 *      ptl_iovec_t type with offsets (local and remote); however, it will be
 *      difficult to make this perform well in the general case. The correct
 *      behavior in this scenario is to treat the region described by the
 *      ptl_iovec_t type as if it were a single contiguous region. In some
 *      cases, this may require walking the entire scatter/gather list to find
 *      the correct location for depositing the data.
 * @implnote The implementation may choose to copy the ptl_iovec_t to prevent
 *      the application from corrupting it; however, the implementation is also
 *      free to use the buffer in place. The implementation is not allowed to
 *      modify the ptl_iovec_t.
 */
typedef struct {
    void* iov_base; /*!< The byte aligned start address of the vector element. */
    ptl_size_t iov_len; /*!< The length (in bytes) of the vector element. */
} ptl_iovec_t;
/*! @struct ptl_ac_id_t
 * @brief To facilitate access control to both list entries and match list
 *      entries, the ptl_ac_id_t is defined as a union of a job ID and a user
 *      ID. A ptl_ac_id_t is attached to each list entry or match list entry to
 *      control which user (or which job, as selected by an option) can access
 *      the entry. Either field can specify a wildcard.
 * @ingroup LEL
 */
typedef union {
    ptl_uid_t   uid; /*!< The user identifier of the \a initiator that may
                       access the associated list entry or match list entry.
                       This may be set to \c PTL_UID_ANY to allow access by any
                       user. */
    ptl_jid_t   jid; /*!< The job identifier of the \a initiator that may
                       access the associated list entry or match list entry.
                       This may be set to \c PTL_JID_ANY to allow access by any
                       job. */
} ptl_ac_id_t;
/*!
 * @struct ptl_le_t
 * @brief Defines the visible parts of a list entry. Values of this type are
 *      used to initialize the list entries.
 *
 * @ingroup LEL
 * @note The list entry (LE) has a number of fields in common with the memory
 *      descriptor (MD). The overlapping fields have the same meaning in the LE
 *      as in the MD; however, since initiator and target resources are
 *      decoupled, the MD is not a proper subset of the LE, and the options
 *      field has different meaning based on whether it is used at an initiator
 *      or target, it was deemed undesirable and cumbersome to include a
 *      "target MD" structure that would be included as an entry in the LE.
 * @note The default behavior from Portals 3.3 (no truncation and locally
 *      managed offsets) has been changed to match the default semantics of the
 *      list entry, which does not provide matching.
 */
typedef struct {
    /*! Specify the starting address of the memory region associated with the
     *  match list entry. Can be \c NULL provided that \a length is zero.
     *  Zero-length buffers (NULL LE) are useful to record events. Messages
     *  that are outside the bounds of the LE are truncated to zero bytes (e.g.
     *  zero-length buffers or an offset beyond the length of the LE). There
     *  are no alignment restrictions on buffer alignment, the starting
     *  address, or the length of the region; although messages that are not
     *  natively aligned (e.g. to a four byte or eight byte boundary) may be
     *  slower (i.e. lower bandwidth and/or longer latency) on some
     *  implementations. */
    void*           start;

    /*! Specify the length of the memory region associated with the match list
     *  entry. */
    ptl_size_t      length;

    /*! A handle for counting type events associated with the memory region. If
     *  this argument is \c PTL_CT_NONE, operations performed on this list
     *  entry are not counted. */
    ptl_handle_ct_t ct_handle;

    /*! Specifies either the user ID or job ID (as selected by the \a options)
     *  that may access this list entry. Either the user ID or job ID may be
     *  set to a wildcard (\c PTL_UID_ANY or \c PTL_JID_ANY). If the access
     *  control check fails, then the message is dropped without modifying
     *  Portals state. This is treated as a permissions failure and the
     *  PtlNIStatus() register indexed by \c PTL_SR_PERMISSIONS_VIOLATIONS is
     *  incremented. This failure is also indicated to the initiator through
     *  the \a ni_fail_type in the \c PTL_EVENT_SEND event, unless the \c
     *  PTL_MD_REMOTE_FAILURE_DISABLE option is set. */
    ptl_ac_id_t     ac_id;

    /*! Specifies the behavior of the list entry. The following options can be
     *  selected: enable put operations (yes or no), enable get operations (yes
     *  or no), offset management (local or remote), message truncation (yes or
     *  no), acknowledgment (yes or no), use scatter/gather vectors and disable
     *  events. Values for this argument can be constructed using a bitwise OR
     *  of the following values:
 - \c PTL_LE_OP_PUT
 - \c PTL_LE_OP_GET
 - \c PTL_LE_USE_ONCE
 - \c PTL_LE_ACK_DISABLE
 - \c PTL_IOVEC
 - \c PTL_LE_EVENT_COMM_DISABLE
 - \c PTL_LE_EVENT_FLOWCTRL_DISABLE
 - \c PTL_LE_EVENT_SUCCESS_DISABLE
 - \c PTL_LE_EVENT_OVER_DISABLE
 - \c PTL_LE_EVENT_UNLINK_DISABLE
 - \c PTL_LE_EVENT_CT_COMM
 - \c PTL_LE_EVENT_CT_OVERFLOW
 - \c PTL_LE_EVENT_CT_BYTES
 - \c PTL_LE_AUTH_USE_JID
 */
    unsigned int    options;
} ptl_le_t;

/****************************
 * Generic Option Constants *
 ****************************/
/*! Specifies that the \a start member of the relevant structure is a pointer
 * to an array of type ptl_iovec_t and the \a length member is the length of
 * the array of ptl_iovec_t elements. This allows for a scatter/gather
 * capability for memory descriptors. A scatter/gather memory descriptor
 * behaves exactly as a memory descriptor that describes a single virtually
 * contiguous region of memory. This value is re-used by several structures. */
#define PTL_IOVEC (1)

/*! Match any process identifier. */
#define PTL_PID_ANY ((ptl_pid_t) 0xffff)

/*! Match any node identifier. */
#define PTL_NID_ANY ((ptl_nid_t) 0xffff)

/*! Match any user identifier. */
#define PTL_UID_ANY ((ptl_uid_t) 0xffffffff)

/*! Match any job identifier. */
#define PTL_JID_ANY ((ptl_jid_t) 0xffffffff)

/*! Match *no* job identifier. */
#define PTL_JID_NONE ((ptl_jid_t) 0)

/*! Wildcard for portal table entry identifier fields. */
#define PTL_PT_ANY ((ptl_pt_index_t) 0xff)

/*! Match any rank. */
#define PTL_RANK_ANY ((ptl_rank_t) 0xffffffff)

/*! Specifies the status register that counts the dropped requests for the
 * interface. */
#define PTL_SR_DROP_COUNT ((ptl_sr_index_t) 0)

/*! Specifies the status register that counts the number of attempted
 * permission violations. */
#define PTL_SR_PERMISSIONS_VIOLATIONS ((ptl_sr_index_t) 1)

/*! Enables an infinite timeout. */
#define PTL_TIME_FOREVER ((ptl_time_t) 0xffffffff)

/**********************
 * Network Interfaces *
 **********************/
/*!
 * @addtogroup NI (NI) Network Interfaces
 * @{
 * @implnote A logical interface is very similar to a physical interface. Like
 * a physical interface, a logical interface is a "well known" interface --
 * i.e. it is a specific physical interface with a specific set of properties.
 * One additional burden placed on the implementation is the need for the
 * initiator to place 2 bits in the message header to identify to the target
 * the logical interface on which this message was sent. In addition, all
 * logical interfaces within a single process that are associated witha single
 * physical interface must share a single node ID and Portals process ID.
 */
/*! @typedef ptl_ni_fail_t
 * A network interface can use this integral type to define specific
 * information regarding the failure of an operation. */
typedef unsigned char ptl_ni_fail_t;

/*! Request that the interface specified in \a iface be opened with matching
 * enabled. */
#define PTL_NI_MATCHING     (1)

/*! Request that the interface specified in \a iface be opened with matching
 * disabled. \c PTL_NI_MATCHING and \c PTL_NI_NO_MATCHING are mutually
 * exclusive. */
#define PTL_NI_NO_MATCHING  (1<<1)

/*! Request that the interface specified in \a iface be opened with logical
 * end-point addressing (e.g.\ MPI communicator and rank or SHMEM PE). */
#define PTL_NI_LOGICAL      (1<<2)

/*! Request that the interface specified in \a iface be opened with physical
 * end-point addressing (e.g.\ NID/PID). \c PTL_NI_LOGICAL and \c
 * PTL_NI_PHYSICAL are mutually exclusive */
#define PTL_NI_PHYSICAL     (1<<3)

/*! Used in successful end events to indicate that there has been no failure. */
#define PTL_NI_OK               ((ptl_ni_fail_t) 0)

/*! Indicates a system failure that prevents message delivery. */
#define PTL_NI_UNDELIVERABLE    ((ptl_ni_fail_t) 1)

/*! Indicates that a message was dropped for some reason. */
#define PTL_NI_DROPPED          ((ptl_ni_fail_t) 2)

/*! Indicates that the remote node has exhausted its resources, enabled flow
 * control, and dropped this message. */
#define PTL_NI_FLOW_CTRL        ((ptl_ni_fail_t) 3)

/*! Indicates that the remote Portals addressing indicated a permissions
 * violation for this message. */
#define PTL_NI_PERM_VIOLATION   ((ptl_ni_fail_t) 4)

/*! Indicates that the Portals implementation allows MEs/LEs to bind inaccessible memory. */
#define PTL_TARGET_BIND_INACCESSIBLE (1<<0)


/*!
 * @struct ptl_ni_limits_t
 * @brief The network interface (NI) limits type */
typedef struct {
    int max_entries;            /*!< Maximum number of match list entries that
                                  can be allocated at any one time. */
    int max_unexpected_headers; /*!< Maximum number of unexpected headers
                                  that can be queued at any one time. */
    int max_mds;                /*!< Maximum number of memory descriptors that
                                  can be allocated at any one time. */
    int max_cts;                /*!< Maximum number of counting events that can
                                  be allocated at any one time. */
    int max_eqs;                /*!< Maximum number of event queues that can be
                                  allocated at any one time. */
    int max_pt_index;           /*!< Largest portal table index for this
                                  interface, valid indexes range from 0 to
                                  max_pt_index, inclusive. An interface must
                                  have a max_pt_index of at least 63. */
    int max_iovecs;             /*!< Maximum number of I/O vectors for a single
                                  memory descriptor for this interface. */
    int max_list_size;          /*!< Maximum number of match list entries that
                                  can be attached to any portal table index. */
    int max_triggered_ops;      /*!< Maximum number of triggered operations
                                  that can be outstanding. */
    ptl_size_t max_msg_size;    /*!< Maximum size (in bytes) of a message (put,
                                  get, or reply). */
    ptl_size_t max_atomic_size; /*!< Maximum size (in bytes) that can be passed
                                  to an atomic operation. */
    ptl_size_t max_fetch_atomic_size; /*!< Maximum size (in bytes) that can be passed
                                  to an atomic operation that returns the prior
                                  value to the initiator. */
    ptl_size_t max_waw_ordered_size; /*!< Maximum size (in bytes) of a message
				       that will guarantee “per-address” data
				       ordering for a write followed by a write
				       (consecutive put or atomic or a mixture
				       of the two) and a write followed by a
				       read (put followed by a get) An
				       interface must provide a
				       \a max_waw_ordered_size of at least 64
				       bytes. */
    ptl_size_t max_war_ordered_size; /*!< Maximum size (in bytes) of a message
				       that will guarantee “per-address” data
				       ordering for a read followed by a write
				       (get followed by a put or atomic). An
				       interface must provide a
				       \a max_war_ordered_size of at least 8
				       bytes. */
    ptl_size_t max_volatile_size; /*!< Maximum size (in bytes) that can be
				    passed as the length of a put or atomic for
				    a memory descriptor with the
				    PTL_MD_VOLATILE option set. */
    unsigned int features; /*!< A bit mask of features supported by the the
			     portals implementation. Currently, the features
			     that are defined are PTL_LE_BIND_INACCESSIBLE and
			     PTL_ME_BIND_INACCESSIBLE. */
} ptl_ni_limits_t;

/************************
 * Portal Table Entries *
 ************************/
/*!
 * @addtogroup PT (PT) Portal Table Entries
 * @{ */
/*! Hint to the underlying implementation that all entries attached to this
 * portal table entry will have the \c PTL_ME_USE_ONCE or \c PTL_LE_USE_ONCE
 * option set. */
#define PTL_PT_ONLY_USE_ONCE    (1)

/*! Enable flow control on this portal table entry. */
#define PTL_PT_FLOWCTRL         (1<<1)

/**********************
 * Memory Descriptors *
 **********************/
/*!
 * @addtogroup MD (MD) Memory Descriptors
 * @{ */
/*! Specifies that this memory descriptor should not generate events that
 * indicate success. This is useful in scenarios where the application does not
 * need normal events, but does require failure information to enhance
 * reliability. */
#define PTL_MD_EVENT_SUCCESS_DISABLE (1<<5)

/*! Enable the counting of \c PTL_EVENT_SEND events. */
#define PTL_MD_EVENT_CT_SEND         (1<<1)

/*! Enable the counting of \c PTL_EVENT_REPLY events. */
#define PTL_MD_EVENT_CT_REPLY        (1<<2)

/*! Enable the counting of \c PTL_EVENT_ACK events. */
#define PTL_MD_EVENT_CT_ACK          (1<<3)

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. Failure events always increment the
 * count by one. */
#define PTL_MD_EVENT_CT_BYTES        (1<<14)

/*! Indicate to the portals implementation that messages sent from this memory
 * descriptor do not have to arrive at the target in order. */
#define PTL_MD_UNORDERED             (1<<4)

/*! Indicate to the Portals implementation that the application may modify the
 * buffer associated with this memory buffer immediately following the return
 * from a portals operation. Operations should not return until it is safe for
 * the application to reuse the buffer. The Portals implementation is not
 * required to honor this option unless the size of the operation is less than
 * or equal to max_volatile_size. */
#define PTL_MD_VOLATILE              (1<<6)

/**************************
 * List Entries and Lists *
 **************************/
/*!
 * @addtogroup LEL (LE) List Entries and Lists
 * @{
 * @enum ptl_list_t
 * @brief A behavior for list appending.
 * @see PtlLEAppend(), PtlMEAppend()
 * @ingroup LEL
 * @ingroup MLEML
 */
typedef enum {
    PTL_PRIORITY_LIST, /*!< The priority list associated with a portal table entry. */
    PTL_OVERFLOW       /*!< The overflow list associated with a portal table entry. */
} ptl_list_t;

/*!
 * @enum ptl_search_op_t
 * @brief A behavior for list searching.
 * @see PtlLESearch()
 * @see PtlMESearch()
 * @ingroup LEL
 * @ingroup MLEML
 */
typedef enum {
    PTL_SEARCH_ONLY, /*!< Use the LE/ME to search the overflow list, without
                       consuming an item in the list. */
    PTL_SEARCH_DELETE /*!< Use the LE/ME to search the overflow list and
                         delete the item from the list. */
} ptl_search_op_t;

/*! Specifies that the list entry will respond to \p put operations. By
 * default, list entries reject \p put operations. If a \p put operation
 * targets a list entry where \c PTL_LE_OP_PUT is not set, it is treated as a
 * permissions failure. */
#define PTL_LE_OP_PUT                   (1<<1)

/*! Specifies that the list entry will respond to \p get operations. By
 * default, list entries reject \p get operations. If a \p get operations
 * targets a list entry where \c PTL_LE_OP_GET is not set, it is treated as a
 * permissions failure.
 * @note It is not considered an error to have a list entry that responds to
 *      both \p put or \p get operations. In fact, it is often desirable for a
 *      list entry used in an \p atomic operation to be configured to respond
 *      to both \p put and \p get operations. */
#define PTL_LE_OP_GET                   (1<<2)

/*! Specifies that the list entry will only be used once and then unlinked. If
 * this option is not set, the list entry persists until it is explicitly
 * unlinked. */
#define PTL_LE_USE_ONCE                 (1<<3)

/*! Specifies that an acknowledgment should not be sent for incoming \p put
 * operations, even if requested. By default, acknowledgments are sent for \p
 * put operations that request an acknowledgment. This applies to both standard
 * and counting type events. Acknowledgments are never sent for \p get
 * operations. The data sent in the \p reply serves as an implicit
 * acknowledgment. */
#define PTL_LE_ACK_DISABLE              (1<<4)

/*! Specifies that this list entry should not generate events that indicate a
 * communication operation. */
#define PTL_LE_EVENT_COMM_DISABLE       (1<<5)

/*! Specifies that this list entry should not generate events that indicate a
 * flow control failure. */
#define PTL_LE_EVENT_FLOWCTRL_DISABLE   (1<<6)

/*! Specifies that this list entry should not generate events that indicate
 * success. This is useful in scenarios where the application does not need
 * normal events, but does require failure information to enhance reliability.
 */
#define PTL_LE_EVENT_SUCCESS_DISABLE    (1<<7)

/*! Specifies that this list entry should not generate overflow list events.
 */
#define PTL_LE_EVENT_OVER_DISABLE       (1<<8)

/*! Specifies that this list entry should not generate unlink (\c
 * PTL_EVENT_UNLINK) or free (\c PTL_EVENT_FREE) events. */
#define PTL_LE_EVENT_UNLINK_DISABLE     (1<<9)

/*! Enable the counting of communication events (\c PTL_EVENT_PUT, \c
 * PTL_EVENT_GET, \c PTL_EVENT_ATOMIC). */
#define PTL_LE_EVENT_CT_COMM            (1<<10)

/*! Enable the counting of overflow events (\c PTL_EVENT_PUT_OVERFLOW, \c
 * PTL_EVENT_ATOMIC_OVERFLOW). */
#define PTL_LE_EVENT_CT_OVERFLOW        (1<<11)

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. Failure events always increment the
 * count by one. */
#define PTL_LE_EVENT_CT_BYTES           (1<<12)

/*! Use job ID for authentication instead of user ID. By default, the user ID
 * must match to allow a message to access a list entry. */
#define PTL_LE_AUTH_USE_JID             (1<<13)

/********************************************
 * Matching List Entries and Matching Lists *
 ********************************************/
/*!
 * @addtogroup MLEML (ME) Matching List Entries and Matching Lists
 * @{
 */
/*! Specifies that the match list entry will respond to \p put operations. By
 * default, match list entries reject \p put operations. If a \p put operation
 * targets a list entry where \c PTL_ME_OP_PUT is not set, it is treated as a
 * permissions failure. */
#define PTL_ME_OP_PUT                   (1<<1)

/*! Specifies that the match list entry will respond to \p get operations. by
 * default, match list entries reject \p get operations. If a \p get operation
 * targets a list entry where \c PTL_ME_OP_GET is not set, it is treated as a
 * permissions failure.
 * @note It is not considered an error to have a match list entry that responds
 *      to both \p put and \p get operations. In fact, it is often desirable
 *      for a match list entry used in an \p atomic operation to be configured
 *      to respond to both \p put and \p get operations. */
#define PTL_ME_OP_GET                   (1<<2)

/*! Specifies that the match list entry will only be used once and then
 * unlinked. If this option is not set, the match list entry persists until
 * another unlink condition is triggered. */
#define PTL_ME_USE_ONCE                 (1<<3)

/*! Specifies that an \p acknowledgment should \e not be sent for incoming \p
 * put operations, even if requested. By default, acknowledgments are sent for
 * put operations that request an acknowledgment. This applies to both standard
 * and counting type events. Acknowledgments are never sent for \p get
 * operations. The data sent in the \p reply serves as an implicit
 * acknowledgment. */
#define PTL_ME_ACK_DISABLE              (1<<4)

/*! Specifies that this match list entry should not generate events that
 * indicate a communication operation. */
#define PTL_ME_EVENT_COMM_DISABLE       (1<<5)

/*! Specifies that this match list entry should not generate events that
 * indicate a flow control failure. */
#define PTL_ME_EVENT_FLOWCTRL_DISABLE   (1<<6)

/*! Specifies that this match list entry should not generate events that
 * indicate success. This is useful in scenarios where the application does not
 * need normal events, but does require failure information to enhance
 * reliability. */
#define PTL_ME_EVENT_SUCCESS_DISABLE    (1<<7)

/*! Specifies that this match list entry should not generate overflow list
 * events (\c PTL_EVENT_PUT_OVERFLOW events). */
#define PTL_ME_EVENT_OVER_DISABLE       (1<<8)

/*! Specifies that this match list entry should not generate unlink (\c
 * PTL_EVENT_UNLINK) or free (\c PTL_EVENT_FREE) events. */
#define PTL_ME_EVENT_UNLINK_DISABLE     (1<<9)

/*! Enable the counting of communication events (\c PTL_EVENT_PUT, \c
 * PTL_EVENT_GET, \c PTL_EVENT_ATOMIC). */
#define PTL_ME_EVENT_CT_COMM            (1<<10)

/*! Enable the counting of overflow events (\c PTL_EVENT_PUT_OVERFLOW, \c
 * PTL_EVENT_ATOMIC_OVERFLOW). */
#define PTL_ME_EVENT_CT_OVERFLOW        (1<<11)

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. Failures are still counted as
 * events. */
#define PTL_ME_EVENT_CT_BYTES           (1<<12)

/*! Use job ID for authentication instead of user ID. By default, the user ID
 * must match to allow a message to access a match list entry. */
#define PTL_ME_AUTH_USE_JID             (1<<13)

/*! Specifies that the offset used in accessing the memory region is managed
 * locally. By default, the offset is in the incoming message. When the offset
 * is maintained locally, the offset is incremented by the length of the
 * request so that the next operation (\p put and/or \p get) will access the next
 * part of the memory region.
 * @note Only one offset variable exists per match list entry. If both \p put and
 *      \p get operations are performed on a match list entry, the value of that
 *      single variable is updated each time. */
#define PTL_ME_MANAGE_LOCAL             (1<<17)

/*! Specifies that the length provided in the incoming request cannot be
 * reduced to match the memory available in the region. This can cause the
 * match to fail. (The memory available in a memory region is determined by
 * subtracting the offset from the length of the memory region.) By default, if
 * the length in the incoming operation is greater than the amount of memory
 * available, the operation is truncated. */
#define PTL_ME_NO_TRUNCATE              (1<<18)

/*! Indicates that messages deposited into this match list entry may be
 * aligned by the implementation to a performance optimizing boundary.
 * Essentially, this is a performance hint to the implementation to indicate
 * that the application does not care about the specific placement of the data.
 * This option is only relevant when the \c PTL_ME_MANAGE_LOCAL option is set.
 * */
#define PTL_ME_MAY_ALIGN                (1<<19)

/*!
 * @struct ptl_me_t
 * @brief Defines the visible parts of a match list entry. Values of this type
 *      are used to initialize and update the match list entries.
 * @note The match list entry (ME) has a number of fields in common with the
 *      memory descriptor (MD). The overlapping fields have the same meaning in
 *      the ME as in the MD; however, since initiator and target resources are
 *      decoupled, the MD is not a proper subset of the ME, and the options
 *      field has different meaning based on whether it is used at an initiator
 *      or target, it was deemed undesirable and cumbersome to include a
 *      "target MD" structure that would be included as an entry in the ME.
 * @note Incoming match bits are compared to the match bits stored in the match
 *      list entry using the ignore bits as a mask. An optimized version of
 *      this is shown in the following code fragment:
 * @code
 * ((incoming_bits ^ match_bits) & ~ignore_bits) == 0
 * @endcode
 */
typedef struct {
    /*! Specify the starting address of the memory region associated with the
     * match list entry. The \a start member can be \c NULL provided that the
     * \a length member is zero. Zero-length buffers (NULL ME) are useful to
     * record events.Messages that are outside the bounds of the ME are
     * truncated to zero bytes (e.g. zero-length buffers or an offset beyond
     * the length of the ME). There are no alignment restrictions on buffer
     * alignment, the starting address or the length of the region; although
     * unaligned messages may be slower (i.e., lower bandwidth and/or longer
     * latency) on some implementations. */
    void *              start;

    /*! Specify the length of the memory region associated with the match list
     * entry. */
    ptl_size_t          length;

    /*! A handle for counting type events associated with the memory region.
     * If this argument is \c PTL_CT_NONE, operations performed on this match
     * list entry are not counted. */
    ptl_handle_ct_t     ct_handle;

    /*! When the unused portion of a match list entry (length - local offset)
     * falls below this value, the match list entry automatically unlinks. A \a
     * min_free value of 0 disables the \a min_free capability (thre free space
     * cannot fall below 0). This value is only used if \c PTL_ME_MANAGE_LOCAL
     * is set. */
    ptl_size_t          min_free;

    /*! Specifies either the user ID or job ID (as selected by the options)
     * that may access this match list entry. Either the user ID or job ID may
     * be set to a wildcard (\c PTL_UID_ANY or \c PTL_JID_ANY). If the access
     * control check fails, then the message is dropped without modifying
     * Portals state. This is treated as a permissions failure and the
     * PtlNIStatus() register indexed by \c PTL_SR_PERMISSIONS_VIOLATIONS is
     * incremented. This failure is also indicated to the initiator through the
     * \a ni_fail_type in the \c PTL_EVENT_SEND event, unless the \c
     * PTL_MD_REMOTE_FAILURE_DISABLE option is set. */
    ptl_ac_id_t         ac_id;

    /*! Specifies the behavior of the match list entry. The following options
     * can be selected: enable put operations (yes or no), enable get
     * operations (yes or no), offset management (local or remote), message
     * truncation (yes or no), acknowledgment (yes or no), use scatter/gather
     * vectors and disable events. Values for this argument can be constructed
     * using a bitwise OR of the following values:
     - \c PTL_ME_OP_PUT
     - \c PTL_ME_OP_GET
     - \c PTL_ME_MANAGE_LOCAL
     - \c PTL_ME_NO_TRUNCATE
     - \c PTL_ME_USE_ONCE
     - \c PTL_ME_MAY_ALIGN
     - \c PTL_ME_ACK_DISABLE
     - \c PTL_IOVEC
     - \c PTL_ME_EVENT_COMM_DISABLE
     - \c PTL_ME_EVENT_FLOWCTRL_DISABLE
     - \c PTL_ME_EVENT_SUCCESS_DISABLE
     - \c PTL_ME_EVENT_OVER_DISABLE
     - \c PTL_ME_EVENT_UNLINK_DISABLE
     - \c PTL_ME_EVENT_CT_COMM
     - \c PTL_ME_EVENT_CT_OVERFLOW
     - \c PTL_ME_EVENT_CT_BYTES
     - \c PTL_ME_AUTH_USE_JID
     */
    unsigned int        options;

    /*! Specifies the match criteria for the process identifier of the
     * requester. The constants \c PTL_PID_ANY and \c PTL_NID_ANY can be used
     * to wildcard either of the physical identifiers in the ptl_process_t
     * structure, or \c PTL_RANK_ANY can be used to wildcard the rank for
     * logical addressing. */
    ptl_process_t       match_id;

    /*! Specify the match criteria to apply to the match bits in the incoming
     * request. The \a ignore_bits are used to mask out insignificant bits in
     * the incoming match bits. The resulting bits are then compared to the
     * match list entry's match bits to determine if the incoming request meets
     * the match criteria. */
    ptl_match_bits_t    match_bits;

    /*! The \a ignore_bits are used to mask out insignificant bits in the
     * incoming match bits. */
    ptl_match_bits_t    ignore_bits;
} ptl_me_t;

/*********************************
 * Lightweight "Counting" Events *
 *********************************/
/*!
 * @addtogroup LCE (CT) Lightweight "Counting" Events
 * @{
 * @struct ptl_ct_event_t
 * @brief A \a ct_handle refers to a ptl_ct_event_t structure.
 */
typedef struct {
    ptl_size_t  success; /*!< A count associated with successful events that
                           counts events or bytes. */
    ptl_size_t  failure; /*!< A count associated with failed events that counts
                           events or bytes. */
} ptl_ct_event_t;
/*!
 * @enum ptl_ct_type_t
 * @brief The type of counting event.
 */
typedef enum {
    PTL_CT_OPERATION,
    PTL_CT_BYTE
} ptl_ct_type_t;

/****************************
 * Data Movement Operations *
 ****************************/
/*!
 * @addtogroup DMO Data Movement Operations
 * @{
 * @implnote Other than PtlPut(), PtlGet(), PtlAtomic(), PtlFetchAtomic(), and
 *      PtlSwap() (and their triggered variants), no function in the portals
 *      API requires communication with other nodes in the system.
 * @enum ptl_ack_req_t
 * @brief Values of the type ptl_ack_req_t are used to control whether an
 *      acknowledgment should be sent when the operation completes (i.e., when
 *      the data has been written to a match list entry of the \e target
 *      process).
 *
 *      When a counting acknowledgment is requested, either \c PTL_CT_OPERATION
 *      or \c PTL_CT_BYTE can be set in the \a ct_handle. If \c
 *      PTL_CT_OPERATION is set, the number of acknowledgments is counted. If
 *      \c PTL_CT_BYTE is set, the modified length (\a mlength) from the target
 *      is counted at the initiator. The operation completed acknowledgment is
 *      an acknowledgment that simply indicated that the operation has
 *      completed at the target. It \e does \e not indicate what was done with
 *      the message. The message may have been dropped due to a permission
 *      violation or may not have matched in the priority list or overflow
 *      list; however, the operation completed acknowledgment would still be
 *      sent. The operation completed acknowledgment is a subset of the
 *      counting acknowledgment with weaker semantics. That is, it is a
 *      counting type of acknowledgment, but it can only count operations.
 */
typedef enum {
    PTL_ACK_REQ, /*!< Requests an acknowledgment. */
    PTL_NO_ACK_REQ, /*!< Requests that no acknowledgment should be generated. */
    PTL_CT_ACK_REQ, /*!< Requests a simple counting acknowledgment. */
    PTL_OC_ACK_REQ /*!< Requests an operation completed acknowledgment. */
} ptl_ack_req_t;

/************************
 * Atomic Operations *
 ************************/
/*!
 * @addtogroup atomic Atomic Operations
 * @{
 * Portals defines three closely related types of atomic operations. The
 * PtlAtomic() function is a one-way operation that performs an atomic
 * operation on data at the \e target with the data passed in the \p put memory
 * descriptor. The PtlFetchAtomic() function extends PtlAtomic() to be an
 * atomic fetch-and-update operation; thus, the value at the \e target before
 * the operation is returned in a \p reply message and placed into the \p get
 * memory descriptor of the \e initiator. Finally, the PtlSwap() operation
 * atomically swaps data (including compare-and-swap and swap-under-mask, which
 * require an \a operand argument).
 *
 * The length of the operations performed by a PtlAtomic() or PtlFetchAtomic()
 * is restricted to no more than \a max_atomic_size bytes. PtlSwap() operations
 * can also be up to \a max_atomic_size bytes, except for \c PTL_CSWAP and \c
 * PTL_MSWAP operations, which are further restricted to 8 bytes (the length of
 * the longest native data type) in all implementations. While the length of an
 * atomic operation is potentially multiple data items, the granularity of the
 * atomic access is limited to the basic datatype. That is, atomic operations
 * from different sources may be interleaved at the level of the datatype being
 * accessed.
 *
 * The \e target match list entry must be configured to respond to \p put
 * operations and to \p get operations if a reply is desired. The \a length
 * argument at the initiator is used to specify the size of the request.
 *
 * There are several events that can be associated with atomic operations. When
 * data is sent from the \e initiator node, a \c PTL_EVENT_SEND event is
 * registered on the \e initiator node. If data is sent from the \e target
 * node, a \c PTL_EVENT_ATOMIC event is registered on the \e target node; and
 * if data is returned from the \e target node, a \c PTL_EVENT_REPLY event is
 * registered on the \e initiator node. Similarly, a \c PTL_EVENT_ACK can be
 * registered on the \e initiator node in the event queue specified by the \a
 * put_md_handle for the atomic operations that do not return data. Note that
 * the target match list entry must have the \c PTL_ME_OP_PUT flag set and must
 * also set the \c PTL_ME_OP_GET flag to enable a reply.
 *
 * The three atomic functions share two new arguments introduced in Portals
 * 4.0: an operation (ptl_op_t) and a datatype (ptl_datatype_t).
 *
 * @enum ptl_op_t
 * @brief An atomic operation type
 */
typedef enum {
    PTL_MIN,                    /*!< Compute and return the minimum of the
                                 * initiator and target value. */
    PTL_MAX,                    /*!< Compute and return the maximum of the
                                 * initiator and target value. */
    PTL_SUM,                    /*!< Compute and return the sum of the
                                 * initiator and target value. */
    PTL_PROD,                   /*!< Compute and return the product of the
                                 * initiator and target value. */
    PTL_LOR,                    /*!< Compute and return the logical OR of the
                                 * initiator and target value. */
    PTL_LAND,                   /*!< Compute and return the logical AND of the
                                 * initiator and target value. */
    PTL_BOR,                    /*!< Compute and return the bitwise OR of the
                                 * initiator and target value. */
    PTL_BAND,                   /*!< Compute and return the bitwise AND of the
                                 * initiator and target value. */
    PTL_LXOR,                   /*!< Compute and return the logical XOR of the
                                 * initiator and target value. */
    PTL_BXOR,                   /*!< Compute and return the bitwise XOR of the
                                 * initiator and target value. */
    PTL_SWAP,                   /*!< Swap the initiator and target value and
                                 * return the target value. */
    PTL_CSWAP,                  /*!< A conditional swap -- if the value of the
                                 * operand is equal to the target value, the
                                 * initiator and target value are swapped. The
                                 * target value is always returned. This
                                 * operation is limited to single data items.
                                 */
    PTL_CSWAP_NE,               /*!< A conditional swap -- if the value of the
                                 * operand is not equal to the target value,
                                 * the initiator and target value are swapped.
                                 * The target value is always returned. This
                                 * operation is limited to single data items.
                                 */
    PTL_CSWAP_LE,               /*!< A conditional swap -- if the value of the
                                 * operand is less than or equal to the target
                                 * value, the initiator and target value are
                                 * swapped. The target value is always
                                 * returned. This operation is limited to
                                 * single data items. */
    PTL_CSWAP_LT,               /*!< A conditional swap -- if the value of the
                                 * operand is les than the target value, the
                                 * initiator and target value are swapped. The
                                 * target value is always returned. This
                                 * operation is limited to single data items.
                                 */
    PTL_CSWAP_GE,               /*!< A conditional swap -- if the value of the
                                 * operand is greater than or equal to the
                                 * target value, the initiator and target value
                                 * are swapped. The target value is always
                                 * returned. This operation is limited to
                                 * single data items. */
    PTL_CSWAP_GT,               /*!< A conditional swap -- if the value of the
                                 * operand is greater than the target value,
                                 * the initiator and target value are swapped.
                                 * The target value is always returned. This
                                 * operation is limited to single data items.
                                 */
    PTL_MSWAP                   /*!< A swap under mask -- update the bits of
                                 * the target value that are set to 1 in the
                                 * operand and return the target value. This
                                 * operation is limited to single data items.
                                 */
} ptl_op_t;
/*!
 * @enum ptl_datatype_t
 * @brief The type of data an atomic operation is operating on
 */
typedef enum {
    PTL_CHAR, /*!< 8-bit signed integer */
    PTL_UCHAR, /*!< 8-bit unsigned integer */
    PTL_SHORT, /*!< 16-bit signed integer */
    PTL_USHORT, /*!< 16-bit unsigned integer */
    PTL_INT, /*!< 32-bit signed integer */
    PTL_UINT, /*!< 32-bit unsigned integer */
    PTL_LONG, /*!< 64-bit signed integer */
    PTL_ULONG, /*!< 64-bit unsigned integer */
    PTL_FLOAT, /*!< 32-bit floating-point number */
    PTL_DOUBLE, /*!< 64-bit floating-point number */
    PTL_LONG_DOUBLE,
    PTL_FLOAT_COMPLEX,
    PTL_DOUBLE_COMPLEX,
    PTL_LONG_DOUBLE_COMPLEX
} ptl_datatype_t;

/***************************
 * Events and Event Queues *
 ***************************/
/*!
 * @addtogroup EQ (EQ) Events and Event Queues
 * @{
 * @enum ptl_event_kind_t
 * @brief The portals API defines twelve types of events that can be logged in
 *      an event queue.
 * @implnote An implementation is not required to deliver overflow events, if
 *      it can prevent an overflow from happening. For example, if an
 *      implementation used rendezvous at the lowest level, it could always
 *      choose to deliver the message into the memory of the ME that would
 *      eventually be posted.
 */
typedef enum {
    /*! A previously initiated \p get operation completed successfully. */
    PTL_EVENT_GET = 1,

    /*! A previously initiated \p put operation completed successfully. The
     * underlying layers will not alter the memory (on behalf of this
     * operation) once this event has been logged. */
    PTL_EVENT_PUT,

    /*! A match list entry posted by PtMEAppend() matched a message that has
     * already arrived and is managed within the overflow list. All, some, or
     * none of the message may have been captured in local memory as requested
     * by the match list entry and described by the \a rlength and \a mlength
     * in the event. The event will point to the start of the message in the
     * memory region described by the match list entry from the overflow list,
     * if any of the message was captured. When the \a rlength and \a mlength
     * fields do not match (i.e. the message was truncated), the application is
     * responsible for performing the remaining transfer. This typically occurs
     * when the application has provided an overflow list entry designed to
     * accept headers but not message bodies. The transfer is typically done by
     * the initiator creating a match list entry using a unique set of bits and
     * then placing the match bits in the \a hdr_data field. The target can
     * then use the \a hdr_data field (along with other information in the
     * event) to retrieve the message. */
    PTL_EVENT_PUT_OVERFLOW,

    /*! A previously initiated \p atomic operation completed successfully. */
    PTL_EVENT_ATOMIC,

    /*! A match list entry posted by PtlMEAppend() matched an atomic operation
     * message that has already arrived and is managed within the overflow
     * list. The behavior is the same as with the \c PTL_EVENT_PUT_OVERFLOW. */
    PTL_EVENT_ATOMIC_OVERFLOW,

    /*! A previously initiated \p reply operation has completed successfully.
     * This event is logged after the data (if any) from the reply has been
     * written into the memory descriptor. */
    PTL_EVENT_REPLY,

    /*! A previously initiated \p send operation has completed. This event is
     * logged after the entire buffer has been sent and it is safe to reuse the
     * buffer. */
    PTL_EVENT_SEND,

    /*! An \p acknowledgment was received. This event is logged when the
     * acknowledgment is received. */
    PTL_EVENT_ACK,

    /*! Resource exhaustion has occurred on this portal table entry. */
    PTL_EVENT_PT_DISABLED,

    /*! A match list entry was unlinked. */
    PTL_EVENT_AUTO_UNLINK,

    /*! A match list entry in the overflow list that was previously unlinked is
     * now free to be reused by the application. */
    PTL_EVENT_AUTO_FREE,

    /*! A previously initiated PtlMEAppend() call that was set to "probe only"
     * completed. If a match message was found in the overflow list, \c
     * PTL_NI_OK is returned in the \a ni_fail_type field of the event and the
     * event queue entries are filled in as if it were a \c
     * PTL_EVENT_PUT_OVERFLOW event. Otherwise, a failure is recorded in the \a
     * ni_fail_type field, and the \a user_ptr is filled in correctly, and the
     * other fields are undefined. */
    PTL_EVENT_SEARCH
} ptl_event_kind_t;
/*!
 * @struct ptl_event_t
 * @brief An event queue contains ptl_event_t structures, which contain a \a
 *      type and a union of the \e target specific event structure and the \e
 *      initiator specific event structure. */
typedef struct {
    /*! Indicates the type of the event. */
    ptl_event_kind_t    type;

    /*! The identifier of the \e initiator. */
    ptl_process_t       initiator;

    /*! The portal table index where the message arrived. */
    ptl_pt_index_t      pt_index;

    /*! The user identifier of the \e initiator. */
    ptl_uid_t           uid;

    /*! The job identifier of the \e initiator. May be \c PTL_JID_NONE in
     * implementations that do not support job identifiers. */
    ptl_jid_t           jid;

    /*! The match bits specified by the \e initiator. */
    ptl_match_bits_t    match_bits;

    /*! The length (in bytes) specified in the request. */
    ptl_size_t          rlength;

    /*! The length (in bytes) of the data that was manipulated by the
     * operation. For truncated operations, the manipulated length will be the
     * number of bytes specified by the memory descriptor operation (possibly
     * with an offset). For all other operations, the manipulated length will
     * be the length of the requested operation. */
    ptl_size_t          mlength;

    /*! The offset requested/used by the other end of the link. At the
     * initiator, this is the displacement (in bytes) into the memory region
     * that the operation used at the target. The offset can be determined by
     * the operation for a remote managed offset in a match list entry or by
     * the match list entry at the target for a locally managed offset. At the
     * target, this is the offset requested by th initiator. */
    ptl_size_t          remote_offset;

    /*! The starting location (virtual, byte address) where the message has
     * been placed. The \a start variable is the sum of the \a start variable
     * in the match list entry and the offset used for the operation. The
     * offset can be determined by the operation for a remote managed match
     * list entry or by the local memory descriptor.
     *
     * When the PtlMEAppend() call matches a message that has arrived in the
     * overflow list, the start address points to the addres in the overflow
     * list where the matching message resides. This may require the
     * application to copy the message to the desired buffer.
     */
    void *              start;

    /*! A user-specified value that is associated with each command that can
     * generate an event. */
    void *              user_ptr;

    /*! 64 bits of out-of-band user data. */
    ptl_hdr_data_t      hdr_data;

    /*! Is used to convey the failure of an operation. Success is indicated by
     * \c PTL_NI_OK. */
    ptl_ni_fail_t       ni_fail_type;

    /*! If this event corresponds to an atomic operation, this indicates the
     * atomic operation that was performed. */
    ptl_op_t            atomic_operation;

    /*! If this event corresponds to an atomic operation, this indicates the
     * data type of the atomic operation that was performed. */
    ptl_datatype_t      atomic_type;
} ptl_event_t;

#endif
