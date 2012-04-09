/*!
 * @file portals4.h
 * @brief Portals4 Reference Implementation
 *
 * Copyright (c) 2010 Sandia Corporation
 */

#ifndef PORTALS4_H
#define PORTALS4_H

#include <stdint.h> /* assume C99, for uint64_t */

#define __ABC__ (128) 

#define MAX_PT_INDEX __ABC__ 
#define MAX_CTS      __ABC__ 
#define MAX_MDS      __ABC__
#define MAX_ENTRIES  __ABC__
#define MAX_EQS      __ABC__
#define MAX_LIST_SIZE __ABC__


/*****************
* Return Values *
*****************/
/*! The set of all possible return codes. */
enum ptl_retvals {
    PTL_OK = 0,          /*!< Indicates success */
    PTL_ARG_INVALID,     /*!< One of the arguments is invalid. */
    PTL_CT_NONE_REACHED, /*!< Timeout reached before any counting event reached the test. */
    PTL_EQ_DROPPED,      /*!< At least one event has been dropped. */
    PTL_EQ_EMPTY,        /*!< No events available in an event queue. */
    PTL_FAIL,            /*!< Indicates a non-specific error */
    PTL_IGNORED,         /*!< Logical map set failed. */
    PTL_IN_USE,          /*!< The specified resource is currently in use. */
    PTL_INTERRUPTED,     /*!< Wait/get operation was interrupted. */
    PTL_LIST_TOO_LONG,   /*!< The resulting list is too long (interface-dependent). */
    PTL_NO_INIT,         /*!< Init has not yet completed successfully. */
    PTL_NO_SPACE,        /*!< Sufficient memory for action was not available. */
    PTL_PID_IN_USE,      /*!< PID is in use. */
    PTL_PT_FULL,         /*!< Portal table has no empty entries. */
    PTL_PT_EQ_NEEDED,    /*!< Flow control is enabled and there is no EQ provided. */
    PTL_PT_IN_USE        /*!< Portal table index is busy. */
};
#define PTL_STATUS_LAST (PTL_PT_IN_USE + 1)

/**************
* Base Types *
**************/
typedef uint64_t ptl_size_t;          /*!< Unsigned 64-bit integral type used for representing sizes. */
typedef uint32_t ptl_pt_index_t;      /*!< Integral type used for representing portal table indices. */
typedef uint64_t ptl_match_bits_t;    /*!< Capable of holding unsigned 64-bit integer values. */
typedef uint64_t ptl_hdr_data_t;      /*!< 64 bits of out-of-band user data. */
typedef unsigned int ptl_time_t;      /*!< Time in milliseconds (used for timeout specification). */
//typedef unsigned int ptl_interface_t; /*!< Integral type used for identifying different network interfaces. */
typedef const char* ptl_interface_t; /*!< Integral type used for identifying different network interfaces. */
typedef uint32_t ptl_nid_t;           /*!< Integral type used for representing node identifiers. */
typedef uint32_t ptl_pid_t;           /*!< Integral type used for representing
                                      * process identifiers when physical
                                      * addressing is used in the network
                                      * interface (PTL_NI_PHYSICAL is set). */
typedef uint32_t ptl_rank_t;          /*!< Integral type used for representing
                                       * process identifiers when logical
                                       * addressing is used in the network
                                       * interface (PTL_NI_LOGICAL is set). */
typedef uint32_t ptl_uid_t;           /*!< Integral type for representing user
                                       * identifiers. */
/* Defines the types of indexes that can be used to access the status
 * registers. */
typedef enum {
    PTL_SR_DROP_COUNT,             /*!< Specifies the status register that counts the
                                    * dropped requests for the interface. */
    PTL_SR_PERMISSIONS_VIOLATIONS, /*!< Specifies the status register that
                                    * counts the number of attempted permission
                                    * violations. */
    PTL_SR_OPERATIONS_VIOLATIONS   /*!< Specifies the status register that counts
                                    * the number of attempted operation
                                    * violations. */
} ptl_sr_index_t;
#define PTL_SR_LAST (PTL_SR_PERMISSIONS_VIOLATIONS + 1)
typedef int ptl_sr_value_t;             /*!< Signed integral type that defines
                                         * the types of values held in status
                                         * registers. */
/* Handles */
typedef uint32_t ptl_handle_any_t;        /*!< generic handle */
typedef ptl_handle_any_t ptl_handle_ni_t; /*!< A network interface handle */
typedef ptl_handle_any_t ptl_handle_eq_t; /*!< An event queue handle */
typedef ptl_handle_any_t ptl_handle_ct_t; /*!< A counting type event handle */
typedef ptl_handle_any_t ptl_handle_md_t; /*!< A memory descriptor handle */
typedef ptl_handle_any_t ptl_handle_le_t; /*!< A list entry handle */
typedef ptl_handle_any_t ptl_handle_me_t; /*!< A match list entry handle */

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
    void *start;               /*!< Specify the starting address for the memory
                                * region associated with the memory descriptor.
                                * There are no alignment restrictions on the
                                * starting address; although unaligned messages may
                                * be slower (i.e. lower bandwidth and/or longer
                                * latency) on some implementations. */
    ptl_size_t   length;       /*!< Specifies the length of the memory region
                                * associated with the memory descriptor. */
    unsigned int options;      /*!<
                                * Specifies the behavior of the memory descriptor. Options include the use of
                                * scatter/gather vectors and disabling of end events associated with this
                                * memory descriptor. Values for this argument can be constructed using a
                                * bitwise OR of the following values:
                                * - \c PTL_MD_EVENT_SUCCESS_DISABLE
                                * - \c PTL_MD_EVENT_CT_SEND
                                * - \c PTL_MD_EVENT_CT_REPLY
                                * - \c PTL_MD_EVENT_CT_ACK
                                * - \c PTL_MD_EVENT_CT_BYTES
                                * - \c PTL_MD_UNORDERED
                                * - \c PTL_IOVEC
                                */
    ptl_handle_eq_t eq_handle; /*!< The event queue handle used to log the
                                * operations performed on the memory region. If
                                * this member is \c PTL_EQ_NONE, operations
                                * performed on this memory descriptor are not
                                * logged. */
    ptl_handle_ct_t ct_handle; /*!< A handle for counting type events
                                * associated with the memory region. If this
                                * argument is \c PTL_CT_NONE, operations
                                * performed on this memory descriptor are not
                                * counted. */
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
    void      *iov_base; /*!< The byte aligned start address of the vector element. */
    ptl_size_t iov_len;  /*!< The length (in bytes) of the vector element. */
} ptl_iovec_t;
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
    void *start;

    /*! Specify the length of the memory region associated with the match list
     *  entry. */
    ptl_size_t length;

    /*! A handle for counting type events associated with the memory region. If
     *  this argument is \c PTL_CT_NONE, operations performed on this list
     *  entry are not counted. */
    ptl_handle_ct_t ct_handle;

    /*! Specifies the user ID that may access this list entry. The user
     * ID may be set to a wildcard (\c PTL_UID_ANY). If the access control
     * check fails, then the message is dropped without modifying Portals
     * state. This is treated as a permissions failure and the PtlNIStatus()
     * register indexed by \c PTL_SR_PERMISSIONS_VIOLATIONS is incremented.
     * This failure is also indicated to the initiator. If a full event is
     * delivered to the initiator, the \a ni_fail_type in the \c PTL_EVENT_ACK
     * must be set to \c PTL_NI_PERM_VIOLATION. */
    ptl_uid_t uid;

    /*! Specifies the behavior of the list entry. The following options can be
     *  selected: enable put operations (yes or no), enable get operations (yes
     *  or no), offset management (local or remote), message truncation (yes or
     *  no), acknowledgment (yes or no), use scatter/gather vectors and disable
     *  events. Values for this argument can be constructed using a bitwise OR
     *  of the following values:
     * - \c PTL_LE_OP_PUT
     * - \c PTL_LE_OP_GET
     * - \c PTL_LE_USE_ONCE
     * - \c PTL_LE_ACK_DISABLE
     * - \c PTL_LE_UNEXPECTED_HDR_DISABLE
     * - \c PTL_IOVEC
     * - \c PTL_LE_EVENT_COMM_DISABLE
     * - \c PTL_LE_EVENT_FLOWCTRL_DISABLE
     * - \c PTL_LE_EVENT_SUCCESS_DISABLE
     * - \c PTL_LE_EVENT_OVER_DISABLE
     * - \c PTL_LE_EVENT_UNLINK_DISABLE
     * - \c PTL_LE_EVENT_CT_COMM
     * - \c PTL_LE_EVENT_CT_OVERFLOW
     * - \c PTL_LE_EVENT_CT_BYTES
     */
    unsigned int options;
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

/*************
* Constants *
*************/
/*! Indicate the absence of an event queue. */
#define PTL_EQ_NONE ((ptl_handle_eq_t)0x3fffffff)

/*! Indicate the absence of a counting type event. */
#define PTL_CT_NONE ((ptl_handle_ct_t)0x5fffffff)

/*! Represent an invalid handle. */
#define PTL_INVALID_HANDLE ((ptl_handle_any_t)0xffffffff)

/*! Identify the default interface. */
//#define PTL_IFACE_DEFAULT ((ptl_interface_t)0xffffffff)
#define PTL_IFACE_DEFAULT "PtlNic" 

/*! Match any process identifier. */
#define PTL_PID_ANY ((ptl_pid_t)0xffff)

/* The biggest process identifier. */
#define PTL_PID_MAX ((ptl_pid_t)1024)

/*! Match any node identifier. */
#define PTL_NID_ANY ((ptl_nid_t)0xffff)

/*! Match any user identifier. */
#define PTL_UID_ANY ((ptl_uid_t)0xffffffff)

/*! Wildcard for portal table entry identifier fields. */
#define PTL_PT_ANY ((ptl_pt_index_t)0xffffffff)

/*! Match any rank. */
#define PTL_RANK_ANY ((ptl_rank_t)0xffffffff)

/*! Enables an infinite timeout. */
#define PTL_TIME_FOREVER ((ptl_time_t)0xffffffff)

/******************************
* Initialization and Cleanup *
******************************/
/*!
 * @addtogroup INC Initialization and Cleanup
 * @{
 * @fn PtlInit(void)
 * @brief Initialize the portals API.
 * @details Initializes the portals library. PtlInit must be called at least
 *      once by a process before any thread makes a portals function call but
 *      may be safely called more than once. Each call to PtlInit() increments
 *      a reference count.
 * @see PtlFini()
 * @retval PTL_OK   Indicates success.
 * @retval PTL_FAIL Indicates some sort of failure in initialization.
 */
int PtlInit(void);
/*!
 * @fn PtlFini(void)
 * @brief Shut down the portals API.
 * @details Allows an application to clean up after the portals library is no
 *      longer needed by a process. Each call to PtlFini() decrements the
 *      reference count that was incremented by PtlInit(). When the reference
 *      count reaches zero, all portals resources are freed. Once the portals
 *      resources are freed, calls to any of the functions defined by the
 *      portals API or use the structures set up by the portals API will result
 *      in undefined behavior. Each call to PtlInit() should be matched by a
 *      corresponding PtlFini().
 * @see PtlInit()
 */
void PtlFini(void);
/*! @} */

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
 * logical interfaces within a single process that are associated with a single
 * physical interface must share a single node ID and Portals process ID.
 */
enum ni_types {
    NI_T_MATCHING,
    NI_T_NMATCHING,
    NI_T_LOGICAL,
    NI_T_PHYSICAL,
    NI_T_OPTIONS_MASK
};

/*! Request that the interface specified in \a iface be opened with matching
 * enabled. */
#define PTL_NI_MATCHING (1 << NI_T_MATCHING)

/*! Request that the interface specified in \a iface be opened with matching
 * disabled. \c PTL_NI_MATCHING and \c PTL_NI_NO_MATCHING are mutually
 * exclusive. */
#define PTL_NI_NO_MATCHING (1 << NI_T_NMATCHING)

/*! Request that the interface specified in \a iface be opened with logical
 * end-point addressing (e.g.\ MPI communicator and rank or SHMEM PE). */
#define PTL_NI_LOGICAL (1 << NI_T_LOGICAL)

/*! Request that the interface specified in \a iface be opened with physical
 * end-point addressing (e.g.\ NID/PID). \c PTL_NI_LOGICAL and \c
 * PTL_NI_PHYSICAL are mutually exclusive */
#define PTL_NI_PHYSICAL (1 << NI_T_PHYSICAL)

#define PTL_NI_INIT_OPTIONS_MASK ((1 << NI_T_OPTIONS_MASK) - 1)

/*! @typedef ptl_ni_fail_t
 * A network interface can use this integral type to define specific
 * information regarding the failure of an operation. */
typedef enum {
    PTL_NI_OK,             /*!< Used in successful end events to indicate that there has
                            * been no failure. */
    PTL_NI_UNDELIVERABLE,  /*!< Indicates a system failure that prevents message
                            * delivery. */
    PTL_NI_DROPPED,        /*!< Indicates that a message was dropped for some reason. */
    PTL_NI_FLOW_CTRL,      /*!< Indicates that the remote node has exhausted its
                            * resources, enabled flow control, and dropped this
                            * message. */
    PTL_NI_PERM_VIOLATION, /*!< Indicates that the remote Portals addressing
                            * indicated a permissions violation for this
                            * message. */
    PTL_NI_OP_VIOLATION,   /*!< Indicates that the remote Portals addressing
                            * indicated an operations violation for this message.
                            */
    PTL_NI_NO_MATCH,       /*!< Indicates that the search did not find an entry in the
                            * unexpected list. */
    PTL_NI_SEGV            /*!< Indicates that a message attempted to access
                            * inaccessible memory */
} ptl_ni_fail_t;

enum ni_features {
    NI_BIND_INACCESSIBLE,
    NI_DATA_ORDERING,
    NI_OPTIONS_MASK
};

/*! Indicates that the Portals implementation allows MEs/LEs to bind inaccessible memory. */
#define PTL_TARGET_BIND_INACCESSIBLE (1 << NI_BIND_INACCESSIBLE)

/*! Indicates that the Portals implementation supports total data ordering. */
#define PTL_TOTAL_DATA_ORDERING (1 << NI_DATA_ORDERING)

/*!
 * @struct ptl_ni_limits_t
 * @brief The network interface (NI) limits type */
typedef struct {
    int max_entries;                  /*!< Maximum number of match list entries that
                                       * can be allocated at any one time. */
    int max_unexpected_headers;       /*!< Maximum number of unexpected headers
                                       * that can be queued at any one time. */
    int max_mds;                      /*!< Maximum number of memory descriptors that
                                       * can be allocated at any one time. */
    int max_cts;                      /*!< Maximum number of counting events that can
                                       * be allocated at any one time. */
    int max_eqs;                      /*!< Maximum number of event queues that can be
                                       * allocated at any one time. */
    int max_pt_index;                 /*!< Largest portal table index for this
                                      * interface, valid indexes range from 0 to
                                      * max_pt_index, inclusive. An interface must
                                      * have a max_pt_index of at least 63. */
    int        max_iovecs;            /*!< Maximum number of I/O vectors for a single
                                       * memory descriptor for this interface. */
    int        max_list_size;         /*!< Maximum number of match list entries that
                                       * can be attached to any portal table index. */
    int        max_triggered_ops;     /*!< Maximum number of triggered operations
                                       * that can be outstanding. */
    ptl_size_t max_msg_size;          /*!< Maximum size (in bytes) of a message (put,
                                       * get, or reply). */
    ptl_size_t max_atomic_size;       /*!< Maximum size (in bytes) that can be passed
                                       * to an atomic operation. */
    ptl_size_t max_fetch_atomic_size; /*!< Maximum size (in bytes) that can be passed
                                       * to an atomic operation that returns the prior
                                       * value to the initiator. */
    ptl_size_t max_waw_ordered_size;  /*!< Maximum size (in bytes) of a message
                                       * that will guarantee “per-address” data
                                       * ordering for a write followed by a write
                                       * (consecutive put or atomic or a mixture
                                       * of the two) and a write followed by a
                                       * read (put followed by a get) An
                                       * interface must provide a
                                       * \a max_waw_ordered_size of at least 64
                                       * bytes. */
    ptl_size_t max_war_ordered_size;  /*!< Maximum size (in bytes) of a message
                                       * that will guarantee “per-address” data
                                       * ordering for a read followed by a write
                                       * (get followed by a put or atomic). An
                                       * interface must provide a
                                       * \a max_war_ordered_size of at least 8
                                       * bytes. */
    ptl_size_t max_volatile_size;     /*!< Maximum size (in bytes) that can be
                                       * passed as the length of a put or atomic for
                                       * a memory descriptor with the
                                       * PTL_MD_VOLATILE option set. */
    unsigned int features;            /*!< A bit mask of features supported by the the
                                       * portals implementation. Currently, the features
                                       * that are defined are PTL_TARGET_BIND_INACCESSIBLE and
                                       * PTL_TOTAL_DATA_ORDERING. */
} ptl_ni_limits_t;

/*!
 * @fn PtlNIInit(ptl_interface_t         iface,
 *               unsigned int            options,
 *               ptl_pid_t               pid,
 *               const ptl_ni_limits_t * desired,
 *               ptl_ni_limits_t *       actual,
 *               ptl_handle_ni_t *       ni_handle)
 * @brief Initialize a network interface.
 * @details Initializes the portals API for a network interface (NI). A process
 *      using portals must call this function at least once before any other
 *      functions that apply to that interface. For subsequent calls to
 *      PtlNIInit() from within the same process (either by different threads
 *      or the same thread), the desired limits will be ignored and the call
 *      will return the existing network interface handle and the actual
 *      limits. Calls to PtlNIInit() increment a reference count on the network
 *      interface and must be matched by a call to PtlNIFini().
 * @param[in] iface         Identifies the network interface to be initialized.
 * @param[in] options       This field contains options that are requested for
 *                          the network interface. Values for this argument can
 *                          be constructed using a bitwise OR of the values
 *                          defined below. Either \c PTL_NI_MATCHING or \c
 *                          PTL_NI_NO_MATCHING must be set, but not both.
 *                          Either \c PTL_NI_LOGICAL or \c PTL_NI_PHYSICAL must
 *                          be set, but not both.
 * @param[in] pid           Identifies the desired process identifier (for well
 *                          known process identifiers). The value \c
 *                          PTL_PID_ANY may be used to let the portals library
 *                          select a process identifier.
 * @param[in] desired       If not \c NULL, points to a structure that holds
 *                          the desired limits.
 * @param[out] actual       If not \c NULL, on successful return, the location
 *                          pointed to by \a actual will hold the actual
 *                          limits.
 * @param[out] ni_handle    On successful return, this location will hold the
 *                          interface handle.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a iface is not a valid
 *                              network interface or \a pid is not a valid
 *                              process identifier.
 * @retval PTL_PID_IN_USE       Indicates that \a pid is currently in use.
 * @retval PTL_SIZE_INVALID     The requested \a map_size is different from the
 *                              \a map_size that was previously used to
 *                              initialize the interface.
 * @retval PTL_NO_SPACE         Indicates that PtlNIInit() was not able to
 *                              allocate the memory required to initialize this
 *                              interface.
 * @see PtlNIFini(), PtlNIStatus()
 */
int PtlNIInit(ptl_interface_t        iface,
              unsigned int           options,
              ptl_pid_t              pid,
              const ptl_ni_limits_t *desired,
              ptl_ni_limits_t       *actual,
              ptl_handle_ni_t       *ni_handle);
/*!
 * @fn PtlNIFini(ptl_handle_ni_t ni_handle)
 * @brief Shut down a network interface.
 * @details Used to release the resources allocated for a network interface.
 *      The release of network interface resources is based on a reference
 *      count that is incremented by PtlNIInit() and decremented by
 *      PtlNIFini(). Resources can only be released when the reference count
 *      reaches zero. Once the release of resources has begun, the results of
 *      pending API operations (e.g. operation initiated by another thread) for
 *      this interface are undefined. Similarly the efects of incoming
 *      operations (put, get, atomic) or return values (acknowledgment and
 *      reply) for this interface are undefined.
 * @param[in] ni_handle     An interface handle to shut down.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle.
 * @see PtlNIInit(), PtlNIStatus()
 * @implnote If PtlNIInit() gets called more than once per logical interface,
 * then the implementation should fill in \a actual and \a ni_handle. It should
 * ignore \a pid. PtlGetId() can be used to retrieve the \a pid.
 */
int PtlNIFini(ptl_handle_ni_t ni_handle);
/*!
 * @fn PtlNIStatus(ptl_handle_ni_t ni_handle,
 *                 ptl_sr_index_t status_register,
 *                 ptl_sr_value_t *status)
 * @brief Read a network interface status register.
 * @details Returns the value of a status register for the specified interface.
 * @param[in]   ni_handle       An interface handle.
 * @param[in]   status_register The index of the status register.
 * @param[out]  status          On successful return, this location will hold
 *                              the current value of the status register.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a ni_handle is not a
 *                              valid network interface handle or \a
 *                              status_register is not a valid status register.
 * @see PtlNIInit(), PtlNIFini()
 */
int PtlNIStatus(ptl_handle_ni_t ni_handle,
                ptl_sr_index_t  status_register,
                ptl_sr_value_t *status);
/*!
 * @fn PtlNIHandle(ptl_handle_any_t handle,
 *                 ptl_handle_ni_t *ni_handle)
 * @brief Get the network Interface handle for an object.
 * @details Returns the network interface handle with which the object
 *      identified by \a handle is associated. If the object identified by \a
 *      handle is a network interface, this function returns the same value it
 *      is passed.
 * @param[in]  handle       The object handle.
 * @param[out] ni_handle    On successful return, this location will hold the
 *                          network interface handle associated with \a handle.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a handle is not a valid handle.
 * @implnote Every handle should encode the network interface and the object
 * identifier relative to this handle.
 */
int PtlNIHandle(ptl_handle_any_t handle,
                ptl_handle_ni_t *ni_handle);
/*!
 * @fn PtlSetMap(ptl_handle_ni_t       ni_handle,
 *               ptl_size_t            map_size,
 *               const ptl_process_t  *mapping)
 * @brief Initialize the mapping from logical identifiers (rank) to physical
 *      identifiers (nid/pid).
 * @details A process using a logical network interface must call this function
 *      at least once before any other functions that apply to that interface.
 *      Subsequent calls (either by different threads or the same thread) to
 *      PtlSetMap() will overwrite any mapping associated with the network
 *      interface; hence, libraries must take care to ensure reasonable
 *      interoperability.
 * @param[in] ni_handle The interface handle identifying the network interface
 *                      which should be initialized with \a mapping.
 * @param[in] map_size  Contains the size of the map being passed in.
 * @param[in] mapping   Points to an array of ptl_process_t structures where
 *                      entry N in the array contains the NID/PID pair that is
 *                      associated with the logical rank N.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that an invalid argument was passed.
 * @retval PTL_NO_SPACE         Indicates that PtlNIInit() was not able to
 *                              allocate the memory required to initialize the
 *                              map.
 * @retval PTL_IGNORED          Indicates that the implementation does not
 *                              support dynamic changing of the logical
 *                              identifier map, likely due to integration with
 *                              a static run-time system.
 */
int PtlSetMap(ptl_handle_ni_t      ni_handle,
              ptl_size_t           map_size,
              const ptl_process_t *mapping);
/*!
 * @fn PtlGetMap(ptl_handle_ni_t ni_handle,
 *               ptl_size_t      map_size,
 *               ptl_process_t  *mapping,
 *               ptl_size_t     *actual_map_size)
 * @brief Retrieves the mapping from logical identifiers (rank) to physical
 *      identifiers (nid/pid).
 * @param[in] ni_handle        The network interface hadle from which the map
 *                             should be retrieved.
 * @param[in] map_size         Contains the size of the size of the buffer
 *                             being passed in elements.
 * @param[out] mapping         Points to an array of ptl_process_t structures
 *                             where entry N in the array will be populated
 *                             with the NID/PID pair that is associated with
 *                             the logical rank N.
 * @param[out] actual_map_size Contains the size of the map currently
 *                             associated with the logical interface. May be
 *                             bigger than map_size or the mapping array.
 * @retval PTL_OK              Indicates success.
 * @retval PTL_NO_INIT         Indicates that the portals API has not been
 *                             successfully initialized.
 * @retval PTL_ARG_INVALID     Indicates that an invalid argument was passed.
 */
int PtlGetMap(ptl_handle_ni_t ni_handle,
              ptl_size_t      map_size,
              ptl_process_t  *mapping,
              ptl_size_t     *actual_map_size);
/*! @} */

/************************
* Portal Table Entries *
************************/
/*!
 * @addtogroup PT (PT) Portal Table Entries
 * @{ */
enum pt_options {
    ONLY_USE_ONCE,
    FLOWCTRL,
    ONLY_TRUNCATE,
    PT_OPTIONS_MASK
};
/*! Hint to the underlying implementation that all entries attached to this
 * portal table entry will have the \c PTL_ME_USE_ONCE or \c PTL_LE_USE_ONCE
 * option set. */
#define PTL_PT_ONLY_USE_ONCE (1 << ONLY_USE_ONCE)

/*! Enable flow control on this portal table entry. */
#define PTL_PT_FLOWCTRL (1 << FLOWCTRL)

/*! Hint to the underlying implementation that all entries attached to the
 * priority list on this portal table entry will not have the \c
 * PTL_ME_NO_TRUNCATE option set. */
#define PTL_PT_ONLY_TRUNCATE (1 << ONLY_TRUNCATE)

#define PTL_PT_ALLOC_OPTIONS_MASK ((1 << PT_OPTIONS_MASK) - 1)

/*!
 * @fn PtlPTAlloc(ptl_handle_ni_t   ni_handle,
 *                unsigned int      options,
 *                ptl_handle_eq_t   eq_handle,
 *                ptl_pt_index_t    pt_index_req,
 *                ptl_pt_index_t*   pt_index)
 * @brief Allocate a free portal table entry.
 * @details Allocates a portal table entry and sets flags that pass options to
 *      the implementation.
 * @param[in] ni_handle     The interface handle to use.
 * @param[in] options       This field contains options that are requested for
 *                          the portal index. Values for this argument can be
 *                          constructed using a bitwise OR of the values \c
 *                          PTL_PT_ONLY_USE_ONCE and \c PTL_PT_FLOWCTRL.
 * @param[in] eq_handle     The event queue handle used to log the operations
 *                          performed on match list entries attached to the
 *                          portal table entry. The \a eq_handle attached to a
 *                          portal table entry must refer to an event queue
 *                          containing ptl_target_event_t type events. If this
 *                          argument is \c PTL_EQ_NONE, operations performed on
 *                          this portal table entry are not logged.
 * @param[in] pt_index_req  The value of the portal index that is requested. If
 *                          the value is set to \c PTL_PT_ANY, the
 *                          implementation can return any portal index.
 * @param[out] pt_index     On successful return, this location will hold the
 *                          portal index that has been allocated.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle.
 * @retval PTL_PT_FULL      Indicates that there are no free entries in the
 *                          portal table.
 * @retval PTL_PT_IN_USE    Indicates that the Portal table entry requested is
 *                          in use.
 * @retval PTL_PT_EQ_NEEDED Indicates that flow control is enabled and there is
 *                          no EQ attached.
 * @see PtlPTFree()
 */
int PtlPTAlloc(ptl_handle_ni_t ni_handle,
               unsigned int    options,
               ptl_handle_eq_t eq_handle,
               ptl_pt_index_t  pt_index_req,
               ptl_pt_index_t *pt_index);
/*!
 * @fn PtlPTFree(ptl_handle_ni_t    ni_handle,
 *               ptl_pt_index_t     pt_index)
 * @brief Free a portal table entry.
 * @details Releases the resources associated with a portal table entry.
 * @param[in] ni_handle The interface handle on which the \a pt_index should be
 *                      freed.
 * @param[in] pt_index  The index of the portal table entry that is to be freed.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a pt_index is not a valid
 *                              portal table index or \a ni_handle is not a
 *                              valid network interface handle.
 * @retval PTL_IN_USE           Indicates that \a pt_index is currently in use
 *                              (e.g. a match list entry is still attached).
 * @see PtlPTAlloc()
 */
int PtlPTFree(ptl_handle_ni_t ni_handle,
              ptl_pt_index_t  pt_index);
/*!
 * @fn PtlPTDisable(ptl_handle_ni_t ni_handle,
 *                  ptl_pt_index_t  pt_index)
 * @brief Disable a portal table entry.
 * @details Indicates to an implementation that no new messages should be
 *      accepted on that portal table entry. The function blocks until the
 *      portal table entry status has been updated, all messages being actively
 *      processed are completed, and all events are posted. Since
 *      PtlPTDisable() waits until the portal table entry is disabled before it
 *      returns, it does not generate a \c PTL_EVENT_PT_DISABLED.
 * @param[in] ni_handle The interface handle to use.
 * @param[in] pt_index  The portal index that is to be disabled.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle.
 * @see PtlPTEnable()
 * @note After successful completion, no other messages will be accepted on
 *      this portal table entry and no more events associated with this portal
 *      table entry will be delivered. Replies arriving at this initiator will
 *      continue to succeed.
 */
int PtlPTDisable(ptl_handle_ni_t ni_handle,
                 ptl_pt_index_t  pt_index);
/*!
 * @fn PtlPTEnable(ptl_handle_ni_t  ni_handle,
 *                 ptl_pt_index_t   pt_index)
 * @brief Enable a portal table entry that has been disabled.
 * @details Indicates to an implementation that a previously disabled portal
 *      table entry should be re-enabled. This is used to enable portal table
 *      entries that were automatically or manually disabled. The function
 *      blocks until the portal table entry is enabled.
 * @param[in] ni_handle The interface handle to use.
 * @param[in] pt_index  The portal index that is to be enabled.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle.
 * @see PtlPTDisable()
 */
int PtlPTEnable(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index);
/*! @} */
/***********************
* User Identification *
***********************/
/*!
 * @addtogroup UI User Identification
 * @{
 * @fn PtlGetUid(ptl_handle_ni_t    ni_handle,
 *               ptl_uid_t*         uid)
 * @brief Get the network interface specific user identifier.
 * @details Retrieves the user identifier of a process.
 *      Every process runs on behalf of a user. User identifiers travel in the
 *      trusted portion of the header of a portals message. They can be used at
 *      the \a target to limit access via access controls.
 * @param[in] ni_handle A network interface handle.
 * @param[out] uid      On successful return, this location will hold the user
 *                      identifier for the calling process.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle.
 */
int PtlGetUid(ptl_handle_ni_t ni_handle,
              ptl_uid_t      *uid);
/*! @} */
/**************************
* Process Identification *
**************************/
/*!
 * @addtogroup PI Process Identification
 * @{
 * @fn PtlGetId(ptl_handle_ni_t     ni_handle,
 *              ptl_process_t*      id)
 * @brief Get the identifier for the current process.
 * @details Retrieves the process identifier of the calling process.
 * @param[in] ni_handle A network interface handle.
 * @param[out] id       On successful return, this location will hold the
 *                      identifier for the calling process.
 * @note Note that process identifiers and ranks are dependent on the network
 *      interface(s). In particular, if a node has multiple interfaces, it may
 *      have multiple process identifiers and multiple ranks.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle
 */
int PtlGetId(ptl_handle_ni_t ni_handle,
             ptl_process_t  *id);
/*!
 * @addtogroup PI Process Identification
 * @{
 * @fn PtlGetPhysId(ptl_handle_ni_t     ni_handle,
 *                  ptl_process_t*      id)
 * @brief Get the identifier for the current process.
 * @details Retrieves the process identifier of the calling process.
 * @param[in] ni_handle A network interface handle.
 * @param[out] id       On successful return, this location will hold the
 *                      identifier for the calling process.
 * @note Note that process identifiers and ranks are dependent on the network
 *      interface(s). In particular, if a node has multiple interfaces, it may
 *      have multiple process identifiers and multiple ranks.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle
 */
int PtlGetPhysId(ptl_handle_ni_t ni_handle,
                 ptl_process_t  *id);
/*! @} */
/**********************
* Memory Descriptors *
**********************/
/*!
 * @addtogroup MD (MD) Memory Descriptors
 * @{ */
enum md_options {
    MD_EVNT_CT_SEND = 1,
    MD_EVNT_CT_REPLY,
    MD_EVNT_CT_ACK,
    MD_UNORDERED,
    MD_EVNT_SUCCESS_DISABLE,
    MD_VOLATILE,
    MD_OPTIONS_MASK
};

/*! Specifies that this memory descriptor should not generate events that
 * indicate success. This is useful in scenarios where the application does not
 * need normal events, but does require failure information to enhance
 * reliability. */
#define PTL_MD_EVENT_SUCCESS_DISABLE (1 << MD_EVNT_SUCCESS_DISABLE)

/*! Enable the counting of \c PTL_EVENT_SEND events. */
#define PTL_MD_EVENT_CT_SEND (1 << MD_EVNT_CT_SEND)

/*! Enable the counting of \c PTL_EVENT_REPLY events. */
#define PTL_MD_EVENT_CT_REPLY (1 << MD_EVNT_CT_REPLY)

/*! Enable the counting of \c PTL_EVENT_ACK events. */
#define PTL_MD_EVENT_CT_ACK (1 << MD_EVNT_CT_ACK)

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. The increment is by the number of
 * bytes counted (\e length for \c PTL_EVENT_SEND events and \e mlength for
 * other events). Failure events always increment the count by one. */
#define PTL_MD_EVENT_CT_BYTES (1 << MELE_EVNT_CT_BYTES)

/*! Indicate to the portals implementation that messages sent from this memory
 * descriptor do not have to arrive at the target in order. */
#define PTL_MD_UNORDERED (1 << MD_UNORDERED)

/*! Indicate to the Portals implementation that the application may modify the
 * buffer associated with this memory buffer immediately following the return
 * from a portals operation. Operations should not return until it is safe for
 * the application to reuse the buffer. The Portals implementation is not
 * required to honor this option unless the size of the operation is less than
 * or equal to \a max_volatile_size. */
#define PTL_MD_VOLATILE (1 << MD_VOLATILE)

#define PTL_MD_OPTIONS_MASK (((1 << MD_OPTIONS_MASK) - 1) | PTL_MD_EVENT_CT_BYTES)

/*!
 * @fn PtlMDBind(ptl_handle_ni_t    ni_handle,
 *               const ptl_md_t*          md,
 *               ptl_handle_md_t*   md_handle)
 * @brief Create a free-floating memory descriptor.
 * @details Used to create a memory descriptor to be used by the \a initiator.
 *      On systems that require memory registration, the PtlMDBind() operation
 *      would invoke the appropriate memory registration functions.
 * @param[in] ni_handle     The network interface handle with which the memory
 *                          descriptor will be associated.
 * @param[in] md            Provides initial values for the user-visible parts
 *                          of a memory descriptor. Other than its use for
 *                          initialization, there is no linkage between this
 *                          structure and the memory descriptor maintained by
 *                          the API.
 * @param[out] md_handle    On successful return, this location will hold the
 *                          newly created memory descriptor handle. The \a
 *                          md_handle argument must be a valid address and
 *                          cannot be \c NULL.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that either \a ni_handle is not a valid
 *                          network interface handle, \a md is not a legal
 *                          memory descriptor (this may happen because the
 *                          memory region defined in \a md is invalid or
 *                          because the network interface associated with the
 *                          \a eq_handle or the \a ct_handle in \a md is not
 *                          the same as the network interface, \a ni_handle),
 *                          the event queue associated with \a md is not valid,
 *                          or the counting event associated with \a md is not
 *                          valid.
 * @retval PTL_NO_SPACE     Indicates that there is insufficient memory to
 *                          allocate the memory descriptor.
 * @implnote Because the \a eq_handle and \a ct_handle are bound to the memory
 *      descriptor on the initiator, there are usage models where it is
 *      necessary to create numerous memory descriptors that only differ in
 *      their eq_handle or ct_handle field. Implementations should support this
 *      usage model and may desire to optimize for it.
 * @see PtlMDRelease()
 */
int PtlMDBind(ptl_handle_ni_t  ni_handle,
              const ptl_md_t  *md,
              ptl_handle_md_t *md_handle);
/*!
 * @fn PtlMDRelease(ptl_handle_md_t md_handle)
 * @brief Release resources associated with a memory descriptor.
 * @details Releases the internal resources associated with a memory descriptor.
 *      (This function does not free the memory region associated with the
 *      memory descriptor; i.e., the memory the user allocated for this memory
 *      descriptor.) Only memory descriptors with no pending operations may be
 *      unlinked.
 * @implnote An implementation will be greatly simplified if the encoding of
 *      memory descriptor handles does not get reused. This makes debugging
 *      easier and it avoids race conditions between threads calling
 *      PtlMDRelease() and PtlMDBind().
 * @param[in] md_handle The memory descriptor handle to be released
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a md_handle is not a valid memory
 *                          descriptor handle.
 * @see PtlMDBind()
 */
int PtlMDRelease(ptl_handle_md_t md_handle);
/*! @} */

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
    PTL_OVERFLOW_LIST  /*!< The overflow list associated with a portal table entry. */
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
    PTL_SEARCH_ONLY,  /*!< Use the LE/ME to search the unexpected list, without
                       * consuming an item in the list. */
    PTL_SEARCH_DELETE /*!< Use the LE/ME to search the unexpected list and
                       * delete the item from the list. */
} ptl_search_op_t;

/*! Specifies that the list entry will respond to \p put operations. By
 * default, list entries reject \p put operations. If a \p put operation
 * targets a list entry where \c PTL_LE_OP_PUT is not set, it is treated as an
 * operations failure and \c PTL_SR_OPERATIONS_VIOLATIONS is incremented. If a
 * full event is delivered to the initiator, the \a ni_fail_type in the \c
 * PTL_EVENT_ACK event must be set to \c PTL_NI_OP_VIOLATION. */
#define PTL_LE_OP_PUT PTL_ME_OP_PUT

/*! Specifies that the list entry will respond to \p get operations. By
 * default, list entries reject \p get operations. If a \p get operation
 * targets a list entry where \c PTL_LE_OP_GET is not set, it is treated as an
 * operations failure and \c PTL_SR_OPERATIONS_VIOLATIONS is incremented. If a
 * full event is delivered to the initiator, the \a ni_fail_type in the \c
 * PTL_EVENT_ACK event must be set to \c PTL_NI_OP_VIOLATION.
 * @note It is not considered an error to have a list entry that responds to
 *      both \p put or \p get operations. In fact, it is often desirable for a
 *      list entry used in an \p atomic operation to be configured to respond
 *      to both \p put and \p get operations. */
#define PTL_LE_OP_GET PTL_ME_OP_GET

/*! Specifies that the list entry will only be used once and then unlinked. If
 * this option is not set, the list entry persists until it is explicitly
 * unlinked. */
#define PTL_LE_USE_ONCE PTL_ME_USE_ONCE

/*! Specifies that an acknowledgment should not be sent for incoming \p put
 * operations, even if requested. By default, acknowledgments are sent for \p
 * put operations that request an acknowledgment. This applies to both standard
 * and counting type events. Acknowledgments are never sent for \p get
 * operations. The data sent in the \p reply serves as an implicit
 * acknowledgment. */
#define PTL_LE_ACK_DISABLE PTL_ME_ACK_DISABLE

/*! Specifies that the header for a message delivered to this list entry should
 * not be added to the unexpected list. This option only has meaning if the
 * list entry is inserted into the overflow list. By creating a list entry
 * which truncates messages to zero bytes, disables comm events, and sets this
 * option, a user may create a list entry which consumes no target side
 * resources. */
#define PTL_LE_UNEXPECTED_HDR_DISABLE PTL_ME_UNEXPECTED_HDR_DISABLE

/*! Indicate that this list entry only contains memory addresses that are
 * accessible by the application.
 */
#define PTL_LE_IS_ACCESSIBLE PTL_ME_IS_ACCESSIBLE

/*! Specifies that this list entry should not generate a \c PTL_EVENT_LINK full
 * event indicating the list entry successfully linked. */
#define PTL_LE_EVENT_LINK_DISABLE PTL_ME_EVENT_LINK_DISABLE

/*! Specifies that this list entry should not generate events that indicate a
 * communication operation. */
#define PTL_LE_EVENT_COMM_DISABLE PTL_ME_EVENT_COMM_DISABLE

/*! Specifies that this list entry should not generate events that indicate a
 * flow control failure. */
#define PTL_LE_EVENT_FLOWCTRL_DISABLE PTL_ME_EVENT_FLOWCTRL_DISABLE

/*! Specifies that this list entry should not generate events that indicate
 * success. This is useful in scenarios where the application does not need
 * normal events, but does require failure information to enhance reliability.
 */
#define PTL_LE_EVENT_SUCCESS_DISABLE PTL_ME_EVENT_SUCCESS_DISABLE

/*! Specifies that this list entry should not generate overflow list events.
 */
#define PTL_LE_EVENT_OVER_DISABLE PTL_ME_EVENT_OVER_DISABLE

/*! Specifies that this list entry should not generate unlink (\c
 * PTL_EVENT_AUTO_UNLINK) or free (\c PTL_EVENT_AUTO_FREE) events. */
#define PTL_LE_EVENT_UNLINK_DISABLE PTL_ME_EVENT_UNLINK_DISABLE

/*! Enable the counting of communication events (\c PTL_EVENT_PUT, \c
 * PTL_EVENT_GET, \c PTL_EVENT_ATOMIC). */
#define PTL_LE_EVENT_CT_COMM PTL_ME_EVENT_CT_COMM

/*! Enable the counting of overflow events. */
#define PTL_LE_EVENT_CT_OVERFLOW PTL_ME_EVENT_CT_OVERFLOW

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. Failure events always increment the
 * count by one. */
#define PTL_LE_EVENT_CT_BYTES PTL_ME_EVENT_CT_BYTES

#define PTL_LE_APPEND_OPTIONS_MASK ((1 << LE_OPTIONS_MASK) - 1)
/*!
 * @fn PtlLEAppend(ptl_handle_ni_t  ni_handle,
 *                 ptl_pt_index_t   pt_index,
 *                 const ptl_le_t  *le,
 *                 ptl_list_t       ptl_list,
 *                 void            *user_ptr,
 *                 ptl_handle_le_t *le_handle)
 * @brief Creates a single list entry and appends this entry to the end of the
 *      list specified by \a ptl_list associated with the portal table entry
 *      specified by \a pt_index for the portal table for \a ni_handle. If the
 *      list is currently uninitialized, the PtlLEAppend() function creates
 *      the first entry in the list.
 *
 *      When a list entry is posted to a list, the unexpected list is checked to
 *      see if a message has arrived prior to posting the list entry. If so, an
 *      appropriate overflow full event is generated, the matching header is
 *      removed from the unexpected list, and a list entry with the \c
 *      PTL_LE_USE_ONCE option is not inserted into the priority list. If a
 *      persistent list entry is posted to the priority list, it may cause
 *      multiple overflow events to be generated, one for every matching entry
 *      in the unexpected list. No permissions check is performed on a matching
 *      message in the unexpected list. No searching of the unexpected list is
 *      performed when a list entry is posted to the overflow list. When the
 *      list entry has been linked (inserted) into the specified list, a \c
 *      PTL_EVENT_LINK event is generated; a call to PtlLEAppend() will always
 *      result in either an overflow or \c PTL_EVENT_LINK event being
 *      generated.
 * @param[in] ni_handle     The interface handle to use.
 * @param[in] pt_index      The portal table index where the list entry should
 *                          be appended.
 * @param[in] le            Provides initial values for the user-visible parts
 *                          of a list entry. Other than its use for
 *                          initialization, there is no linkage between this
 *                          structure and the list entry maintained by the API.
 * @param[in] ptl_list      Determines whether the list entry is appended to
 *                          the priority list or appended to the overflow list.
 * @param[in] user_ptr      A user-specified value that is associated with each
 *                          command that can generate an event. The value does
 *                          not need to be a pointer, but must fit in the space
 *                          used by a pointer. This value (along with other
 *                          values) is recorded in events associated with
 *                          operations on this list entry.
 * @param[out] le_handle    On successful return, this location will hold the
 *                          newly created list entry handle.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a ni_handle is not a
 *                              valid network interface handle or \a pt_index
 *                              is not a valid portal table index.
 * @retval PTL_NO_SPACE         Indicates that there is insufficient memory to
 *                              allocate the match list entry.
 * @retval PTL_LE_LIST_TOO_LONG Indicates that the resulting list is too long.
 *                              The maximum length for a list is defined by the
 *                              interface.
 * @see PtlLEUnlink()
 */
int PtlLEAppend(ptl_handle_ni_t  ni_handle,
                ptl_pt_index_t   pt_index,
                const ptl_le_t  *le,
                ptl_list_t       ptl_list,
                void            *user_ptr,
                ptl_handle_le_t *le_handle);
/*!
 * @fn PtlLEUnlink(ptl_handle_le_t le_handle)
 * @brief Used to unlink a list entry from a list. This operation also releases
 *      any resources associated with the list entry. It is an error to use the
 *      list entry handle after calling PtlLEUnlink().
 * @param[in] le_handle The list entry handle to be unlinked.
 * @note If this list entry has pending operations; e.g., an unfinished reply
 *      operation, then PtlLEUnlink() will return \c PTL_IN_USE, and the
 *      list entry will not be unlinked. This essentially creates a race
 *      between the application retrying the unlink operation and a new
 *      operation arriving. This is believed to be reasonable as the
 *      application rarely wants to unlink an LE while new operations are
 *      arriving to it.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a le_handle is not a valid
 *                          list entry handle.
 * @retval PTL_IN_USE       Indicates that the list entry has pending
 *                          operations and cannot be unlinked.
 * @see PtlLEAppend()
 */
int PtlLEUnlink(ptl_handle_le_t le_handle);
/*!
 * @fn PtlLESearch(ptl_handle_ni_t  ni_handle,
 *                 ptl_pt_index_t   pt_index,
 *                 const ptl_le_t  *le,
 *                 ptl_search_op_t  ptl_search_op,
 *                 void            *user_ptr)
 * @brief Used to search for a message in the unexpected list associated with a
 *      specific portal table entry specified by \a pt_index for the portal
 *      table for \a ni_handle. PtlLESearch() uses the exact same search of the
 *      unexpected list as PtlLEAppend(); however, the list entry specified in
 *      the PtlLESearch() call is never linked into a priority list.
 *
 *      The PtlLESearch() function can be called in two modes. If \a ptl_search_op
 *      is set to PTL_SEARCH_ONLY, the unexpected list is searched to support
 *      the MPI_Probe functionality. If \a ptl_search_op is set to
 *      PTL_SEARCH_DELETE, the unexpected list is searched and any matching
 *      items are deleted. A search of the unexpected list will \e always
 *      generate a detailed event. When used with PTL_SEARCH_ONLY, a
 *      PTL_EVENT_SEARCH detailed event is always generated. If a matching
 *      message was found in the unexpected list, PTL_NI_OK is returned in the
 *      detailed event. Otherwise, the detailed event indicates that the search
 *      operation failed. When used with PTL_SEARCH_DELETE, the event that is
 *      generated corresponds to the type of operation that is found (e.g.
 *      PTL_EVENT_PUT_OVERFLOW or PTL_EVENT_ATOMIC_OVERFLOW). If no operation
 *      is found, a PTL_EVENT_SEARCH is generated with a failure indication of
 *      \c PTL_NI_NO_MATCH. If the list entry specified in the PtlLESearch()
 *      call is persistent, an event is generated for every match in the
 *      unexpected list.
 *
 *      Event generation for the search function works just as it would for an
 *      append function. If a search is performed with detailed events disabled
 *      (either through option or through the absence of an event queue on the
 *      portal table entry), the search will succeed, but no detailed events
 *      will be generated. Status registers, however, are handled slightly
 *      differently for a search in that a PtlLESearch()never causes a status
 *      register to be incremented.
 * @note Searches with persistent entries could have unexpected performance and
 *      resource usage characteristics if a large unexpected list has
 *      accumulated, since a PtlLESearch() that uses a persistent LE can cause
 *      multiple matches.
 * @param[in] ni_handle     The interface handle to use
 * @param[in] pt_index      The portal table index to search.
 * @param[in] le            Provides values for the user-visible parts of a
 *                          list entry to use for searching.
 * @param[in] ptl_search_op Determines whether the function will delete any
 *                          list entries that match the search parameters.
 * @param[in] user_ptr      A user-specified value that is associated with each
 *                          command that can generate an event. The value does
 *                          not need to be a pointer, but must fit in the space
 *                          used by a pointer. This value (along with other
 *                          values) is recorded in events associated with
 *                          operations on this list entry.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_ARG_INVALID  Indicates that an invalid argument was passed. THe
 *                          definition of which arguments are checked is
 *                          implementation dependent.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 */
int PtlLESearch(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index,
                const ptl_le_t *le,
                ptl_search_op_t ptl_search_op,
                void           *user_ptr);
/*! @} */

enum mele_options {
    MELE_IOVEC = 0,
    MELE_OP_PUT,
    MELE_OP_GET,
    MELE_USE_ONCE,
    MELE_ACK_DISABLE,
    MELE_UNEXPECTED_HDR_DISABLE,
    MELE_IS_ACCESSIBLE,
    MELE_EVNT_LINK_DISABLE,
    MELE_EVNT_COMM_DISABLE,
    MELE_EVNT_FLOWCTRL_DISABLE,
    MELE_EVNT_SUCCESS_DISABLE,
    MELE_EVNT_OVER_DISABLE,
    MELE_EVNT_UNLINK_DISABLE,
    MELE_EVNT_CT_COMM,
    MELE_EVNT_CT_OVERFLOW,
    MELE_EVNT_CT_BYTES,
    LE_OPTIONS_MASK
};

enum me_options {
    ME_MANAGE_LOCAL = LE_OPTIONS_MASK,
    ME_NO_TRUNCATE,
    ME_MAY_ALIGN,
    ME_OPTIONS_MASK
};

/********************************************
* Matching List Entries and Matching Lists *
********************************************/
/*!
 * @addtogroup MLEML (ME) Matching List Entries and Matching Lists
 * @{
 */
/*! Specifies that the match list entry will respond to \p put operations. By
 * default, match list entries reject \p put operations. If a \p put operation
 * targets a match list entry where \c PTL_LE_OP_PUT is not set, it is treated
 * as an operations failure and \c PTL_SR_OPERATIONS_VIOLATIONS is incremented.
 * If a full event is delivered to the initiator, the \a ni_fail_type in the \c
 * PTL_EVENT_ACK event must be set to \c PTL_NI_OP_VIOLATION. */
#define PTL_ME_OP_PUT (1 << MELE_OP_PUT)

/*! Specifies that the match list entry will respond to \p get operations. By
 * default, match list entries reject \p get operations. If a \p get operation
 * targets a match list entry where \c PTL_LE_OP_GET is not set, it is treated
 * as an operations failure and \c PTL_SR_OPERATIONS_VIOLATIONS is incremented.
 * If a full event is delivered to the initiator, the \a ni_fail_type in the \c
 * PTL_EVENT_ACK event must be set to \c PTL_NI_OP_VIOLATION.
 * @note It is not considered an error to have a match list entry that responds
 *      to both \p put or \p get operations. In fact, it is often desirable for
 *      a match list entry used in an \p atomic operation to be configured to
 *      respond to both \p put and \p get operations. */
#define PTL_ME_OP_GET (1 << MELE_OP_GET)

/*! Specifies that the match list entry will only be used once and then
 * unlinked. If this option is not set, the match list entry persists until
 * another unlink condition is triggered. */
#define PTL_ME_USE_ONCE (1 << MELE_USE_ONCE)

/*! Specifies that an \p acknowledgment should \e not be sent for incoming \p
 * put operations, even if requested. By default, acknowledgments are sent for
 * put operations that request an acknowledgment. This applies to both standard
 * and counting type events. Acknowledgments are never sent for \p get
 * operations. The data sent in the \p reply serves as an implicit
 * acknowledgment. */
#define PTL_ME_ACK_DISABLE (1 << MELE_ACK_DISABLE)

/*! Specifies that the header for a message delivered to this list entry should
 * not be added to the unexpected list. This option only has meaning if the
 * list entry is inserted into the overflow list. By creating a list entry
 * which truncates messages to zero bytes, disables comm events, and sets this
 * option, a user may create a list entry which consumes no target side
 * resources. */
#define PTL_ME_UNEXPECTED_HDR_DISABLE (1 << MELE_UNEXPECTED_HDR_DISABLE)

/*! Indicate that this list entry only contains memory addresses that are
 * accessible by the application.
 */
#define PTL_ME_IS_ACCESSIBLE (1 << MELE_IS_ACCESSIBLE)

/*! Specifies that this match list entry should not generate a \c
 * PTL_EVENT_LINK full event indicating the list entry successfully linked. */
#define PTL_ME_EVENT_LINK_DISABLE (1 << MELE_EVNT_LINK_DISABLE)

/*! Specifies that this match list entry should not generate events that
 * indicate a communication operation. */
#define PTL_ME_EVENT_COMM_DISABLE (1 << MELE_EVNT_COMM_DISABLE)

/*! Specifies that this match list entry should not generate events that
 * indicate a flow control failure. */
#define PTL_ME_EVENT_FLOWCTRL_DISABLE (1 << MELE_EVNT_FLOWCTRL_DISABLE)

/*! Specifies that this match list entry should not generate events that
 * indicate success. This is useful in scenarios where the application does not
 * need normal events, but does require failure information to enhance
 * reliability. */
#define PTL_ME_EVENT_SUCCESS_DISABLE (1 << MELE_EVNT_SUCCESS_DISABLE)

/*! Specifies that this match list entry should not generate overflow list
 * events. */
#define PTL_ME_EVENT_OVER_DISABLE (1 << MELE_EVNT_OVER_DISABLE)

/*! Specifies that this match list entry should not generate unlink (\c
 * PTL_EVENT_AUTO_UNLINK) or free (\c PTL_EVENT_AUTO_FREE) events. */
#define PTL_ME_EVENT_UNLINK_DISABLE (1 << MELE_EVNT_UNLINK_DISABLE)

/*! Enable the counting of communication events (\c PTL_EVENT_PUT, \c
 * PTL_EVENT_GET, \c PTL_EVENT_ATOMIC). */
#define PTL_ME_EVENT_CT_COMM (1 << MELE_EVNT_CT_COMM)

/*! Enable the counting of overflow events. */
#define PTL_ME_EVENT_CT_OVERFLOW (1 << MELE_EVNT_CT_OVERFLOW)

/*! By default, counting events count events. When set, this option causes
 * successful bytes to be counted instead. Failures are still counted as
 * events. */
#define PTL_ME_EVENT_CT_BYTES (1 << MELE_EVNT_CT_BYTES)

/*! Specifies that the offset used in accessing the memory region is managed
 * locally. By default, the offset is in the incoming message. When the offset
 * is maintained locally, the offset is incremented by the length of the
 * request so that the next operation (\p put and/or \p get) will access the next
 * part of the memory region.
 * @note Only one offset variable exists per match list entry. If both \p put and
 *      \p get operations are performed on a match list entry, the value of that
 *      single variable is updated each time. */
#define PTL_ME_MANAGE_LOCAL (1 << ME_MANAGE_LOCAL)

/*! Specifies that the length provided in the incoming request cannot be
 * reduced to match the memory available in the region. This can cause the
 * match to fail. (The memory available in a memory region is determined by
 * subtracting the offset from the length of the memory region.) By default, if
 * the length in the incoming operation is greater than the amount of memory
 * available, the operation is truncated. */
#define PTL_ME_NO_TRUNCATE (1 << ME_NO_TRUNCATE)

/*! Indicates that messages deposited into this match list entry may be
 * aligned by the implementation to a performance optimizing boundary.
 * Essentially, this is a performance hint to the implementation to indicate
 * that the application does not care about the specific placement of the data.
 * This option is only relevant when the \c PTL_ME_MANAGE_LOCAL option is set.
 * */
#define PTL_ME_MAY_ALIGN (1 << ME_MAY_ALIGN)

#define PTL_ME_APPEND_OPTIONS_MASK ((1 << ME_OPTIONS_MASK) - 1)

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
    void *start;

    /*! Specify the length of the memory region associated with the match list
     * entry. */
    ptl_size_t length;

    /*! A handle for counting type events associated with the memory region.
     * If this argument is \c PTL_CT_NONE, operations performed on this match
     * list entry are not counted. */
    ptl_handle_ct_t ct_handle;

    /*! Specifies either the user ID that may access this match list entry. The
     * user ID may be set to a wildcard (\c PTL_UID_ANY). If the access control
     * check fails, then the message is dropped without modifying Portals
     * state. This is treated as a permissions failure and the PtlNIStatus()
     * register indexed by \c PTL_SR_PERMISSIONS_VIOLATIONS is incremented.
     * This failure is also indicated to the initiator. If a full event is
     * delivered to the initiator, the \a ni_fail_type in the \c PTL_EVENT_ACK
     * full event must be set to \c PTL_NI_PERM_VIOLATION. */
    ptl_uid_t uid;

    /*! Specifies the behavior of the match list entry. The following options
     * can be selected: enable put operations (yes or no), enable get
     * operations (yes or no), offset management (local or remote), message
     * truncation (yes or no), acknowledgment (yes or no), use scatter/gather
     * vectors and disable events. Values for this argument can be constructed
     * using a bitwise OR of the following values:
     * - \c PTL_ME_OP_PUT
     * - \c PTL_ME_OP_GET
     * - \c PTL_ME_MANAGE_LOCAL
     * - \c PTL_ME_NO_TRUNCATE
     * - \c PTL_ME_USE_ONCE
     * - \c PTL_ME_MAY_ALIGN
     * - \c PTL_ME_ACK_DISABLE
     * - \c PTL_IOVEC
     * - \c PTL_ME_EVENT_COMM_DISABLE
     * - \c PTL_ME_EVENT_FLOWCTRL_DISABLE
     * - \c PTL_ME_EVENT_SUCCESS_DISABLE
     * - \c PTL_ME_EVENT_OVER_DISABLE
     * - \c PTL_ME_EVENT_UNLINK_DISABLE
     * - \c PTL_ME_EVENT_CT_COMM
     * - \c PTL_ME_EVENT_CT_OVERFLOW
     * - \c PTL_ME_EVENT_CT_BYTES
     */
    unsigned int options;

    /*! Specifies the match criteria for the process identifier of the
     * requester. The constants \c PTL_PID_ANY and \c PTL_NID_ANY can be used
     * to wildcard either of the physical identifiers in the ptl_process_t
     * structure, or \c PTL_RANK_ANY can be used to wildcard the rank for
     * logical addressing. */
    ptl_process_t match_id;

    /*! Specify the match criteria to apply to the match bits in the incoming
     * request. The \a ignore_bits are used to mask out insignificant bits in
     * the incoming match bits. The resulting bits are then compared to the
     * match list entry's match bits to determine if the incoming request meets
     * the match criteria. */
    ptl_match_bits_t match_bits;

    /*! The \a ignore_bits are used to mask out insignificant bits in the
     * incoming match bits. */
    ptl_match_bits_t ignore_bits;

    /*! When the unused portion of a match list entry (length - local offset)
     * falls below this value, the match list entry automatically unlinks. A \a
     * min_free value of 0 disables the \a min_free capability (thre free space
     * cannot fall below 0). This value is only used if \c PTL_ME_MANAGE_LOCAL
     * is set. */
    ptl_size_t min_free;
} ptl_me_t;
/*!
 * @fn PtlMEAppend(ptl_handle_ni_t      ni_handle,
 *                 ptl_pt_index_t       pt_index,
 *                 const ptl_me_t      *me,
 *                 ptl_list_t           ptl_list,
 *                 void                *user_ptr,
 *                 ptl_handle_me_t     *me_handle)
 * @brief Create a match list entry and append it to a portal table.
 * @details Creates a single match list entry. If \c PTL_PRIORITY_LIST or \c
 *      PTL_OVERFLOW_LIST is specified by \a ptl_list, this entry is appended
 *      to the end of the appropriate list specified by \a ptl_list associated
 *      with the portal table entry specified by \a pt_index for the portal
 *      table for \a ni_handle. If the list is currently uninitialized, the
 *      PtlMEAppend() function creates the first entry in the list.
 *
 *      When a match list entry is posted to the priority list, the unexpected
 *      list is searched to see if a matching message has been delivered in the
 *      unexpected list prior to the posting of the match list entry. If so, an
 *      appropriate overflow event is generated, the matching header is removed
 *      from the unexpected list, and a match list entry with the \c
 *      PTL_ME_USE_ONCE option is not inserted into the priority list. If a
 *      persistent match list entry is posted to the priority list, it may
 *      cause multiple overflow events to be generated, one for every matching
 *      entry in the unexpected list. No permissions checking is performed on a
 *      matching message in the unexpected list. No searching of the unexpected
 *      list is performed when a match list entry is posted to the overflow
 *      list. When the list entry has been linked (inserted) into the specified
 *      list, a \c PTL_EVENT_LINK event is generated; a call to PtlMEAppend
 *      will always result in either an overflow or \c PTL_EVENT_LINK event
 *      being generated.
 *
 * @param[in] ni_handle     The interface handle to use.
 * @param[in] pt_index      The portal table index where the match list entry
 *                          should be appended.
 * @param[in] me            Provides initial values for the user-visible parts
 *                          of a match list entry. Other than its use for
 *                          initialization, there is no linkage between this
 *                          structure and the match list entry maintained by
 *                          the API.
 * @param[in] ptl_list      Determines whether the match list entry is appended
 *                          to the priority list or the overflow list.
 * @param[in] user_ptr      A user-specified value that is associated with each
 *                          command that can generate an event. The value does
 *                          not need to be a pointer, but must fit in the space
 *                          used by a pointer. This value (along with other
 *                          values) is recorded in events associated with
 *                          operations on this match list entry.
 * @param[out] me_handle    On successful return, this location will hold the
 *                          newly created match list entry handle.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a ni_handle is not a
 *                              valid network interface handle, \a pt_index is
 *                              not a valid portal table index, or \a match_id
 *                              in the match list entry is not a valid process
 *                              identifier.
 * @retval PTL_NO_SPACE         Indicates that there is insufficient memory to
 *                              allocate the match list entry.
 * @retval PTL_LIST_TOO_LONG    Indicates that the resulting list is too long.
 *                              The maximum length for a list is defined by the
 *                              interface.
 * @implnote Checking whether a \a match_id is a valid process identifier may
 *      require global knowledge. However, PtlMEAppend() is not meant to cause
 *      any communication with other nodes in the system. Therefore, \c
 *      PTL_PROCESS_INVALID may not be returned in some cases where it would
 *      seem appropriate.
 * @see PtlMEUnlink()
 */
int PtlMEAppend(ptl_handle_ni_t  ni_handle,
                ptl_pt_index_t   pt_index,
                ptl_me_t        *me,
                ptl_list_t       ptl_list,
                void            *user_ptr,
                ptl_handle_me_t *me_handle);
/*!
 * @fn PtlMEUnlink(ptl_handle_me_t me_handle)
 * @brief Remove a match list entry from a list and release its resources.
 * @details Used to unlink a match list entry from a list. This operation also
 *      releases any resources associated with the match list entry. It is an
 *      error to use the match list entry handle after calling PtlMEUnlink().
 * @param[in] me_handle The match list entry handle to be unlinked.
 * @note If this match list entry has pending operations; e.g., an unfinished
 *      \p reply operation, then PtlMEUnlink() will return \c PTL_IN_USE,
 *      and the match list entry will not be unlinked. This essentially creates
 *      a race between the application retrying the unlink operation and a new
 *      operation arriving. This is believed to be reasonable as the
 *      application rarely wants to unlink an ME while new operation are
 *      arriving to it.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a me_handle is not a valid
 *                              match list entry handle.
 * @retval PTL_IN_USE           Indicates that the match list entry has pending
 *                              operations and cannot be unlinked.
 * @see PtlMEAppend()
 */
int PtlMEUnlink(ptl_handle_me_t me_handle);
/*!
 * @fn PtlMESearch(ptl_handle_ni_t ni_handle,
 *                 ptl_pt_index_t  pt_index,
 *                 const ptl_me_t *me,
 *                 ptl_search_op_t ptl_search_op,
 *                 void           *user_ptr)
 * @brief The PtlMESearch() function is used to search for a message in the
 *      unexpected list associated with a specific portal table entry specified
 *      by pt_index for the portal table for ni_handle. PtlMESearch() uses the
 *      exact same search of the unexpected list as PtlMEAppend(); however, the
 *      match list entry specified in the PtlMESearch() call is never linked
 *      into a priority list.
 *
 *      The PtlMESearch() function can be called in two modes. If ptl_search_op
 *      is set to PTL_SEARCH_ONLY, the unexpected list is searched to support
 *      the MPI_Probe functionality. If ptl_search_op is set to
 *      PTL_SEARCH_DELETE, the unexpected list is searched and any matching
 *      items are deleted. A search of the unexpected list will always generate
 *      an event. When used with PTL_SEARCH_ONLY, a PTL_EVENT_SEARCH event is
 *      always generated. If a matching message was found in the unexpected list,
 *      PTL_NI_OK is returned in the event. Otherwise, the event indicates that
 *      the search operation failed. When used with PTL_SEARCH_DELETE, the
 *      event that is generated corresponds to the type of operation that is
 *      found (e.g. PTL_EVENT_PUT_OVERFLOW or PTL_EVENT_ATOMIC_OVERFLOW). If no
 *      operation is found, a PTL_EVENT_SEARCH is generated with a failure
 *      indication.
 *
 *      Event generation for the search functions works just as it would for an
 *      append function. If a search is performed with events disabled (either
 *      through option or through the absence of an event queue on the portal
 *      table entry), the search will succeed, but no events will be generated.
 *      Status registers, however, are handled slightly differently for a
 *      search in that a PtlMESearch() never causes a status register to be
 *      incremented.
 * @see PtlLESearch()
 * @param[in] ni_handle     The interface handle to use
 * @param[in] pt_index      The portal table index to search.
 * @param[in] me            Provides values for the user-visible parts of a
 *                          match list entry to use for searching.
 * @param[in] ptl_search_op Determines whether the function will delete any
 *                          match list entries that match the search parameters.
 * @param[in] user_ptr      A user-specified value that is associated with each
 *                          command that can generate an event. The value does
 *                          not need to be a pointer, but must fit in the space
 *                          used by a pointer. This value (along with other
 *                          values) is recorded in events associated with
 *                          operations on this match list entry.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_ARG_INVALID  Indicates that an invalid argument was passed. THe
 *                          definition of which arguments are checked is
 *                          implementation dependent.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 */
int PtlMESearch(ptl_handle_ni_t ni_handle,
                ptl_pt_index_t  pt_index,
                const ptl_me_t *me,
                ptl_search_op_t ptl_search_op,
                void           *user_ptr);
/*! @} */

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
    ptl_size_t success;  /*!< A count associated with successful events that
                          * counts events or bytes. */
    ptl_size_t failure;  /*!< A count associated with failed events that counts
                          * events or bytes. */
} ptl_ct_event_t;
/*!
 * @enum ptl_ct_type_t
 * @brief The type of counting event.
 */
typedef enum {
    PTL_CT_OPERATION,
    PTL_CT_BYTE
} ptl_ct_type_t;
/*!
 * @fn PtlCTAlloc(ptl_handle_ni_t   ni_handle,
 *                ptl_handle_ct_t * ct_handle)
 * @brief Create a counting event.
 * @details Used to allocate a counting event that counts either operations on
 *      the memory descriptor (match list entry) or bytes that flow out of
 *      (into) a memory descriptor (match list entry). While a PtlCTAlloc()
 *      call could be as simple as a malloc of a structure holding the counting
 *      event and a network interface handle, it may be necessary to allocate
 *      the counting event in low memory or some other protected space; thus,
 *      an allocation routine is provided. A newly allocated count is
 *      initialized to zero.
 * @param[in] ni_handle     The interface handle with which the counting event
 *                          will be associated.
 * @param[out] ct_handle    On successful return, this location will hold the
 *                          newly created counting event handle.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ni_handle is not a valid
 *                              network interface handle.
 * @retval PTL_NO_SPACE         Indicates that there is insufficient memory to
 *                              allocate the counting event.
 * @implnote A quality implementation will attempt to minimize the cost of
 *      counting events. This can be done by translating the simple functions
 *      (PtlCTGet(), PtlCTWait(), PtlCTSet(), and PtlCTInc()) into simple
 *      macros that directly access a structure in the applications memory
 *      unless otherwise required by the hardware.
 * @see PtlCTFree()
 */
int PtlCTAlloc(ptl_handle_ni_t  ni_handle,
               ptl_handle_ct_t *ct_handle);
/*!
 * @fn PtlCTFree(ptl_handle_ct_t ct_handle)
 * @brief Free a counting event.
 * @details Releases the resources associated with a counting event. It is up to
 *      the user to ensure that no memory descriptors or match list entries are
 *      associated with the counting event once it is freed.
 * @param[in] ct_handle The counting event handle to be released.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ct_handle is not a valid
 *                              counting event handle.
 * @see PtlCTAlloc()
 */
int PtlCTFree(ptl_handle_ct_t ct_handle);
/*!
 * @fn PtlCTCancelTriggered(ptl_handle_ct_t ct_handle)
 * @brief Cancel pending triggered operations.
 * @details In certain circumstances, it may be necessary to cancel triggered
 *      operations that are pending. For example, an error condition may mean
 *      that a counting event will never reach the designated threshold.
 *      PtlCTCancelTriggered() is provided to handle these circumstances. Upon
 *      return from PtlCTCancelTriggered(), all triggered operations waiting on
 *      \a ct_handle are permanently destroyed. The operations are not
 *      triggered, and will not modify any application-visible state. The other
 *      state associated with ct_handle is left unchanged.
 * @param[in] ct_handle The counting event handle associated with the triggered
 *                      operations to be canceled.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ct_handle is not a valid
 *                              counting event handle.
 */
int PtlCTCancelTriggered(ptl_handle_ct_t ct_handle);
/*!
 * @fn PtlCTGet(ptl_handle_ct_t     ct_handle,
 *              ptl_ct_event_t *    event)
 * @brief Get the current value of a counting event.
 * @details Used to obtain the current value of a counting event. Accesses made
 *      by PtlCTGet() are not atomic relative to modifications made by the
 *      PtlCTSet() and PtlCTInc() functions. Calling PtlCTFree() in a separate
 *      thread while PtlCTGet() is executing may yield undefined results in the
 *      returned value.
 * @param[in] ct_handle     The counting event handle.
 * @param[out] event        On successful return, this location will hold the
 *                          current value associated with the counting event.
 * @retval PTL_OK           Indicates success
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ct_handle is not a valid
 *                          counting event handle.
 * @retval PTL_INTERRUPTED  Indicates that PtlCTFree() or PtlNIFini() was
 *                          called by another thread while this thread was
 *                          in PtlCTGet()
 * @note PtlCTGet() must be as close to the speed of a simple variable
 *      access as possible; hence, PtlCTGet() is not atomic relative to
 *      PtlCTSet() or PtlCTInc() and is undefined if PtlCTFree() or PtlNIFini()
 *      is called during the execution of the function.
 * @see PtlCTWait(), PtlCTPoll()
 */
int PtlCTGet(ptl_handle_ct_t ct_handle,
             ptl_ct_event_t *event);
/*!
 * @fn PtlCTWait(ptl_handle_ct_t    ct_handle,
 *               ptl_size_t         test,
 *               ptl_ct_event_t *   event)
 * @brief Wait for a counting event to reach a certain value.
 * @details Used to wait until the value of a counting event is equal to a test
 *      value.
 * @param[in] ct_handle The counting event handle.
 * @param[in] test      On successful return, the sum of the success and
 *                      failure fields of the counting event will be greater
 *                      than or equal to this value.
 * @param[out] event    On successful return, this location will hold the
 *                      current value associated with the counting event.
 * @retval PTL_OK           Indicates success
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ct_handle is not a valid
 *                          counting event handle.
 * @retval PTL_INTERRUPTED  Indicates that PtlCTFree() or PtlNIFini() was
 *                          called by another thread while this thread was
 *                          waiting in PtlCTWait()
 * @note PtlCTWait() returns when the counting event referenced by the \a
 *      ct_handle is greater than or equal to the test; thus, the actual value
 *      of the counting event is returned to prevent the user from having to
 *      call PtlCTGet().
 * @implnote The return code of \c PTL_INTERRUPTED adds an unfortunate degree
 *      of complexity to the PtlCTWait() function; however, it was deemed
 *      necessary to be able to interrupt waiting functions for the sake of
 *      applications that need to tolerate failures. Hence, this approach to
 *      dealing with the conflict of reading and freeing events was chosen.
 * @see PtlCTGet(), PtlCTPoll()
 */
int PtlCTWait(ptl_handle_ct_t ct_handle,
              ptl_size_t      test,
              ptl_ct_event_t *event);
/*!
 * @fn PtlCTPoll(const ptl_handle_ct_t *  ct_handles,
 *               const ptl_size_t * tests,
 *               unsigned int       size,
 *               ptl_time_t         timeout,
 *               ptl_ct_event_t *   event,
 *               unsigned int *     which)
 * @brief Wait for an array of counting events to reach certain values.
 * @details Used to look for one of an array of counting events that has reached
 *      its respective threshold. Should a counting event reach the test value
 *      for any of the counting events contained in the array of counting event
 *      handles, the value of the counting event will be returned in \a event
 *      and \a which will contain the index of the counting event from which
 *      the value was returned.
 *
 *      PtlCTPoll() provides a timeout to allow applications to poll, block for
 *      a fixed period, or block indefinitely. PtlCTPoll() is sufficiently
 *      general to imlement both PtlCTGet() and PtlCTWait(), but these
 *      functions have been retained in the API, since they can be implemented
 *      in a substantially lighter weight manner.
 * @implnote PtlCTPoll() should test the list of counting events in a
 *      round-robin fashion. This cannot guarantee fairness but meets common
 *      expectations.
 * @param[in] ct_handles    An array of counting event handles. All of the
 *                          handles must refer to the same interface.
 * @param[in] tests         On successful return, the sum of the success and
 *                          failure fields of the counting event indicated by
 *                          \a which will be greater than or equal to the
 *                          corresponding value in this array.
 * @param[in] size          Length of the array.
 * @param[in] timeout       Time in milliseconds to wait for an event to occur
 *                          on one of the event queue handles. The constant
 *                          \c PTL_TIME_FOREVER can be used to indicate an
 *                          infinite timeout.
 * @param[out] event        On successful return, this location will hold the
 *                          current value associated with the counting event
 *                          that caused PtlCTPoll() to return.
 * @param[out] which        On successful return, this location will contain
 *                          the index into \a ct_handles of the counting event
 *                          that reached its test value.
 * @retval PTL_OK               Indicates success.
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates an invalid argument (e.g. a bad \a
 *                              ct_handle).
 * @retval PTL_CT_NONE_REACHED  Indicates that none of the counting events
 *                              reached their test before the timeout was
 *                              reached.
 * @retval PTL_INTERRUPTED      Indicates that PtlCTFree() or PtlNIFini() was
 *                              called by another thread while this thread was
 *                              waiting in PtlCTPoll().
 * @implnote Implementations are discouraged from providing macros for
 *      PtlCTGet() and PtlCTWait() that use PtlCTPoll() instead of providing
 *      these functions. The usage scenario for PtlCTGet() and PtlCTWait() is
 *      expected to depend on minimizing the computational cost of these
 *      routines.
 * @implnote The return code of \c PTL_INTERRUPTED adds an unfortunate degree
 *      of complexity to the PtlCTPoll() function; however, it was deemed
 *      necessary to be able to interrupt waiting functions for the sake of
 *      applications that need to tolerate failures. Hence, this approach to
 *      dealing with the conflict of reading and freeing events was chosen.
 * @see PtlCTWait(), PtlCTGet()
 */
int PtlCTPoll(const ptl_handle_ct_t *ct_handles,
              const ptl_size_t      *tests,
              unsigned int           size,
              ptl_time_t             timeout,
              ptl_ct_event_t        *event,
              unsigned int          *which);
/*!
 * @fn PtlCTSet(ptl_handle_ct_t ct_handle,
 *              ptl_ct_event_t  new_ct)
 * @brief Set a counting event to a certain value.
 * @details Used to set the value of a counting event. Periodically, it is
 *      desirable to reinitialize or adjust the value of a counting event. This
 *      must be done atomically relative to other modifications, so a
 *      functional interface is provided. The PtlCTSet() function is used to
 *      set the value of a counting event. The entire ptl_ct_event_t is updated
 *      atomically relative to other modifications of the counting event;
 *      however, it is not atomic relative to read accesses of the counting
 *      event. The increment field can only be non-zero for either the success
 *      or failure field in a given call to PtlCTInc().
 * @param[in] ct_handle The counting event handle.
 * @param[in] new_ct    On successful return, the value of the counting event
 *                      will have been set to this value.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ct_handle is not a valid
 *                              counting event handle.
 * @see PtlCTInc()
 */
int PtlCTSet(ptl_handle_ct_t ct_handle,
             ptl_ct_event_t  test);
/*!
 * @fn PtlCTInc(ptl_handle_ct_t ct_handle,
 *              ptl_ct_event_t  increment)
 * @brief Increment a counting event by a certain value.
 * @details Used to (atomically) increment the value of a counting event.
 *      In some scenarios, the counting event will need to be incremented by
 *      the application. This must be done atomically relative to other
 *      modifications of the counting event, so a functional interface is
 *      provided. The PtlCTInc() function is used to increment the value of a
 *      counting event. The entire ptl_ct_event_t is updated atomically
 *      relative to other modifications of the counting event; however, it is
 *      not atomic relative to read accesses of the counting event.
 * @param[in] ct_handle The counting event handle.
 * @param[in] increment On successful return, the value of the counting event
 *                      will have been incremented by this value.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ct_handle is not a valid
 *                              counting event handle.
 * @note As an example, a counting event may need to be incremented at the
 *      completion of a message that is received. If the message arrives in the
 *      overflow list, it may be desirable to delay the counting event
 *      increment until the application can place the data in the correct
 *      buffer.
 * @see PtlCTSet()
 */
int PtlCTInc(ptl_handle_ct_t ct_handle,
             ptl_ct_event_t  increment);
/*! @} */

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
 *      process). The value \c PTL_ACK_REQ requests an acknowledgment, the
 *      value \c PTL_NO_ACK_REQ requests that no acknowledgment should be
 *      generated, the value \c PTL_CT_ACK_REQ requests a simple counting
 *      acknowledgment, and the value \c PTL_OC_ACK_REQ requests an operation
 *      completed acknowledgment. When a counting acknowledgment is requested,
 *      the operations in the requesting memory descriptor can be set to count
 *      either events or bytes. If the \c PTL_MD_EVENT_CT_BYTES option is set
 *      and the operation is successful, the requested length (\a rlength) is
 *      counted for a \c PTL_EVENT_SEND event and the modified length (\a
 *      mlength) from the target is counted. If the event would indicate
 *      "failure" or the \c PTL_MD_EVENT_CT_BYTES option is not set, the number
 *      of acknowledgments is counted. The operation completed acknowledgment
 *      is an acknowledgment that simply indicates that the operation has
 *      completed at the target. It \e does \e not indicate what was done with
 *      the message. The message may have been dropped due to a permission
 *      violation or may not have matched in the priority list or overflow
 *      list; however, the operation completed acknowledgment would still be
 *      sent. The operation completed acknowledgment is a subset of the
 *      counting acknowledgment with weaker semantics. That is, it is a
 *      counting type of acknowledgment, but it can only count operations.
 */
typedef enum {
    PTL_NO_ACK_REQ, /*!< Requests that no acknowledgment should be generated. */
    PTL_ACK_REQ,    /*!< Requests an acknowledgment. */
    PTL_CT_ACK_REQ, /*!< Requests a simple counting acknowledgment. */
    PTL_OC_ACK_REQ  /*!< Requests an operation completed acknowledgment. */
} ptl_ack_req_t;
/*!
 * @fn PtlPut(ptl_handle_md_t   md_handle,
 *            ptl_size_t        local_offset,
 *            ptl_size_t        length,
 *            ptl_ack_req_t     ack_req,
 *            ptl_process_t     target_id,
 *            ptl_pt_index_t    pt_index,
 *            ptl_match_bits_t  match_bits,
 *            ptl_size_t        remote_offset,
 *            void             *user_ptr,
 *            ptl_hdr_data_t    hdr_data)
 * @brief Perform a \e put operation.
 * @details Initiates an asynchronous \p put operation. There are several events
 *      associated with a \p put operation: completion of the send on the \e
 *      initiator node (\c PTL_EVENT_SEND) and, when the send completes
 *      successfully, the receipt of an acknowledgment (\c PTL_EVENT_ACK)
 *      indicating that the operation was accepted by the target. The event \c
 *      PTL_EVENT_PUT is used at the \e target node to indicate the end of data
 *      delivery, while c PTL_EVENT_PUT_OVERFLOW can be used on the \e target
 *      node when a message arrives before the corresponding match list entry.
 *
 *      These (local) events will be logged in the event queue associated with
 *      the memory descriptor (\a md_handle) used in the \p put operation.
 *      Using a memory descriptor that does not have an associated event queue
 *      results in these events being discarded. In this case, the caller must
 *      have another mechanism (e.g., a higher level protocol) for determining
 *      when it is safe to modify the memory region associated with the memory
 *      descriptor.
 *
 *      The local (\e initiator) offset is used to determine the starting
 *      address of the memory region within the region specified by the memory
 *      descriptor and the length specifies the length of the region in bytes.
 *      It is an error for the local offset and length parameters to specify
 *      memory outside the memory described by the memory descriptor.
 * @param[in] md_handle     The memory descriptor handle that describes the
 *                          memory to be sent. If the memory descriptor has an
 *                          event queue associated with it, it will be used to
 *                          record events when the message has been sent (\c
 *                          PTL_EVENT_SEND, \c PTL_EVENT_ACK).
 * @param[in] local_offset  Offset from the start of the memory descriptor.
 * @param[in] length        Length of the memory region to be sent.
 * @param[in] ack_req       Controls whether an acknowledgment event is
 *                          requested. Acknowledgments are only sent when they
 *                          are requested by the initiating process \b and the
 *                          memory descriptor has an event queue \b and the
 *                          target memory descriptor enables them.
 * @param[in] target_id     A process identifier for the \e target process.
 * @param[in] pt_index      The index in the \e target portal table.
 * @param[in] match_bits    The match bits to use for message selection at the
 *                          \e target process (only used when matching is
 *                          enabled on the network interface).
 * @param[in] remote_offset The offset into the target memory region (used
 *                          unless the \e target match list entry has the \c
 *                          PTL_ME_MANAGE_LOCAL option set).
 * @param[in] user_ptr      A user-specified value that is associated with each
 *                          command that can generate an event. The value does
 *                          not need to be a pointer, but must fit in the space
 *                          used by a pointer. This value (along with other
 *                          values) is recorded in \e initiator events
 *                          associated with this \p put operation.
 * @param[in] hdr_data      64 bits of user data that can be included in the
 *                          message header. This data is written to an event
 *                          queue entry at the \e target if an event queue is
 *                          present on the match list entry that matches the
 *                          message.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor or \a target_id is not
 *                              a valid process identifier.
 * @see PtlGet()
 */
int PtlPut(ptl_handle_md_t  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_ack_req_t    ack_req,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void            *user_ptr,
           ptl_hdr_data_t   hdr_data);
/*!
 * @fn PtlGet(ptl_handle_md_t   md_handle,
 *            ptl_size_t        local_offset,
 *            ptl_size_t        length,
 *            ptl_process_t     target_id,
 *            ptl_pt_index_t    pt_index,
 *            ptl_match_bits_t  match_bits,
 *            ptl_size_t        remote_offset,
 *            void             *user_ptr)
 * @brief Perform a \e get operation.
 * @details Initiates a remote read operation. There are two events associated
 *      with a get operation. When the data is sent from the \e target node if
 *      the message matched in the priority list. The message can also match in
 *      the unexpected list, which will cause a \c PTL_EVENT_GET event to be
 *      registered on the \e target node and will later cause a \c
 *      PTL_EVENT_GET_OVERFLOW to be registered on the \e target node when a
 *      matching entry is appended. In either case, when the data is returned
 *      from the \e target node, a \c PTL_EVENT_REPLY event is
 *      registered on the \e initiator node.
 *
 *      The local (\e initiator) offset is used to determine the starting
 *      address of the memory region and the length specifies the length of the
 *      region in bytes. It is an error for the local offset and length
 *      parameters to specify memory outside the memory described by the memory
 *      descriptor.
 * @param[in] md_handle     The memory descriptor handle that describes the
 *                          memory into which the requested data will be
 *                          received. The memory descriptor can have an event
 *                          queue associated with it to record events, such as
 *                          when the message receive has started.
 * @param[in] local_offset  Offset from the start of the memory descriptor.
 * @param[in] length        Length of the memory region for the \p reply.
 * @param[in] target_id     A process identifier for the \e target process.
 * @param[in] pt_index      The index in the \e target portal table.
 * @param[in] match_bits    The match bits to use for message selection at the
 *                          \e target process.
 * @param[in] remote_offset The offset into the target match list entry (used
 *                          unless the target match list entry has the \c
 *                          PTL_ME_MANAGE_LOCAL option set.
 * @param[in] user_ptr      See the discussion for PtlPut().
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a valid
 *                              memory descriptor or \a target_id is not a
 *                              valid process identifier.
 * @see PtlPut()
 */
int PtlGet(ptl_handle_md_t  md_handle,
           ptl_size_t       local_offset,
           ptl_size_t       length,
           ptl_process_t    target_id,
           ptl_pt_index_t   pt_index,
           ptl_match_bits_t match_bits,
           ptl_size_t       remote_offset,
           void            *user_ptr);

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
 * The length of the operations performed by a PtlAtomic() are restricted to no
 * more than \a max_atomic_size bytes. PtlFetchAtomic() and PtlSwap() operations
 * can be up to \a max_fetch_atomic_size bytes, except for \c PTL_CSWAP and \c
 * PTL_MSWAP operations and their variants, which are further restricted to the
 * length of the longest native data type in all implementations.
 *
 * While the length of an atomic operation is potentially multiple data items,
 * the granularity of the atomic access is limited to the basic datatype. That
 * is, atomic operations from different sources may be interleaved at the level
 * of the datatype being accessed. Furthermore, atomic operations are only
 * atomic with respect to other calls to the Portals API on the same network
 * interface (\a ni_handle). In addition, an implementation is only required to
 * support Portals atomic operations that are natively aligned to the size of
 * the datatype, but it may choose to provide support for unaligned accesses.
 * Atomicity is only guaranteed for two atomic operations using the same
 * datatype, and overlapping atomic operations that use different datatypes are
 * not atomic with respect to each other. The routine PtlAtomicSync() is
 * provided to enable the host (or atomic operations using other datatypes) to
 * modify memory locations that have been previously touched by an atomic
 * operation.
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
 * 4.0: an operation (ptl_op_t) and a datatype (ptl_datatype_t). All three
 * atomic functions are required to be natively aligned at the target to the
 * size of the ptl_datatype_t used.
 *
 * @implnote To allow upper level libraries with both system-defined datatype
 *      widths and fixed width datatypes to easily map to Portals, Portals
 *      provides fixed width integer types. The one exception is the long
 *      double floating point types (PTL_LONG_DOUBLE). Because of the
 *      variability in long double encodings across systems and the lack of
 *      standard syntax for fixed width floating-point types, Portals uses a
 *      system defined width for PTL_LONG_DOUBLE and PTL_LONG_DOUBLE_COMPLEX.
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
#define PTL_OP_LAST (PTL_MSWAP + 1)
/*!
 * @enum ptl_datatype_t
 * @brief The type of data an atomic operation is operating on
 */
typedef enum {
    PTL_INT8_T,         /*!< 8-bit signed integer */
    PTL_UINT8_T,        /*!< 8-bit unsigned integer */
    PTL_INT16_T,        /*!< 16-bit signed integer */
    PTL_UINT16_T,       /*!< 16-bit unsigned integer */
    PTL_INT32_T,        /*!< 32-bit signed integer */
    PTL_UINT32_T,       /*!< 32-bit unsigned integer */
    PTL_FLOAT,          /*!< 32-bit floating-point number */
    PTL_INT64_T,        /*!< 64-bit signed integer */
    PTL_UINT64_T,       /*!< 64-bit unsigned integer */
    PTL_DOUBLE,         /*!< 64-bit floating-point number */
    PTL_FLOAT_COMPLEX,  /*!< 32-bit floating-point complex number */
    PTL_DOUBLE_COMPLEX, /*!< 64-bit floating-point complex number */
    PTL_LONG_DOUBLE,
    PTL_LONG_DOUBLE_COMPLEX
} ptl_datatype_t;
#define PTL_DATATYPE_LAST (PTL_LONG_DOUBLE_COMPLEX + 1)
/*!
 * @fn PtlAtomic(ptl_handle_md_t    md_handle,
 *               ptl_size_t         local_offset,
 *               ptl_size_t         length,
 *               ptl_ack_req_t      ack_req,
 *               ptl_process_t      target_id,
 *               ptl_pt_index_t     pt_index,
 *               ptl_match_bits_t   match_bits,
 *               ptl_size_t         remote_offset,
 *               void              *user_ptr,
 *               ptl_hdr_data_t     hdr_data,
 *               ptl_op_t           operation,
 *               ptl_datatype_t     datatype)
 * @brief Perform an atomic operation.
 * @details Initiates an asynchronous atomic operation. The events behave like
 *      the PtlPut() function, with the exception of the target side event,
 *      which is a \c PTL_EVENT_ATOMIC (or \c PTL_EVENT_ATOMIC_OVERFLOW)
 *      instead of a \c PTL_EVENT_PUT. Similarly, the arguments mirror PtlPut()
 *      with the addition of a ptl_datatype_t and ptl_op_t to specify the
 *      datatype and operation being performed, respectively.
 * @param[in] md_handle     The memory descriptor handle that describes the
 *                          memory to be sent. If the memory descriptor has an
 *                          event queue associated with it, it will be used to
 *                          record events when the message has been sent.
 * @param[in] local_offset  Offset from the start of the memory descriptor
 *                          referenced by the \a md_handle to use for
 *                          transmitted data.
 * @param[in] length        Length of the memory region to be sent and/or
 *                          received. The \a length field must be less than or
 *                          equal to max_atomic_size.
 * @param[in] ack_req       Controls whether an acknowledgment event is
 *                          requested. Acknowledgments are only sent when they
 *                          are requested by the initiating process \b and the
 *                          memory descriptor has an event queue \b and the
 *                          target memory descriptor enables them.
 * @param[in] target_id     A process identifier for the \e target process.
 * @param[in] pt_index      The index in the \e target portal table.
 * @param[in] match_bits    The match bits to use for message selection at the
 *                          \e target process.
 * @param[in] remote_offset The offset into the target memory region (used
 *                          unless the target match list entry has the \c
 *                          PTL_ME_MANAGE_LOCAL option set).
 * @param[in] user_ptr      See the discussion for PtlPut()
 * @param[in] hdr_data      64 bits of user data that can be included in the
 *                          message header. This data is written to an event
 *                          queue entry at the \e target if an event queue is
 *                          present on the match list entry that the message
 *                          matches.
 * @param[in] operation     The operation to be performed using the initiator
 *                          and target data.
 * @param[in] datatype      The type of data being operated on at the initiator
 *                          and target.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a valid
 *                              memory descriptor or \a target_id is not a
 *                              valid process identifier.
 */
int PtlAtomic(ptl_handle_md_t  md_handle,
              ptl_size_t       local_offset,
              ptl_size_t       length,
              ptl_ack_req_t    ack_req,
              ptl_process_t    target_id,
              ptl_pt_index_t   pt_index,
              ptl_match_bits_t match_bits,
              ptl_size_t       remote_offset,
              void            *user_ptr,
              ptl_hdr_data_t   hdr_data,
              ptl_op_t         operation,
              ptl_datatype_t   datatype);
/*!
 * @fn PtlFetchAtomic(ptl_handle_md_t   get_md_handle,
 *                    ptl_size_t        local_get_offset,
 *                    ptl_handle_md_t   put_md_handle,
 *                    ptl_size_t        local_put_offset,
 *                    ptl_size_t        length,
 *                    ptl_process_t     target_id,
 *                    ptl_pt_index_t    pt_index,
 *                    ptl_match_bits_t  match_bits,
 *                    ptl_size_t        remote_offset,
 *                    void             *user_ptr,
 *                    ptl_hdr_data_t    hdr_data,
 *                    ptl_op_t          operation,
 *                    ptl_datatype_t    datatype)
 * @brief Perform a fetch-and-atomic operation.
 * @details Extends PtlAtomic() to return the value from the target \e prior \e
 *      to \e the \e operation \e being \e performed. This means that both
 *      PtlPut() and PtlGet() style events can be delivered. When data is sent
 *      from the initiator node, a \c PTL_EVENT_SEND event is registered on the
 *      \e initiator node in the event queue specified by the \a put_md_handle.
 *      The event \c PTL_EVENT_ATOMIC is registered on the \e target node to
 *      indicate completion of an atomic operation; and if data is returned
 *      from the \e target node, a \c PTL_EVENT_REPLY event is registered on
 *      the \e initiator node in the event queue specified by the \a
 *      get_md_handle. Note that receiving a \c PTL_EVENT_REPLY inherently
 *      implies that the flow control check has passed on the target node. In
 *      addition, it is an error to use memory descriptors bound to different
 *      network interfaces in a single PtlFetchAtomic() call. The behavior that
 *      occurs when the \a local_get_offset into the \a get_md_handle overlaps
 *      with the \a local_put_offset into the \a put_md_handle is undefined.
 *      Operations performed by PtlFetchAtomic() are constrained to be no more
 *      than \a max_fetch_atomic_size bytes and must be aligned at the target
 *      to the size of ptl_datatype_t passed in the \a datatype argument.
 * @param[in] get_md_handle     The memory descriptor handle that describes the
 *                              memory into which the result of the operation
 *                              will be placed. The memory descriptor can have
 *                              an event queue associated with it to record
 *                              events, such as when the result of the
 *                              operation has been returned.
 * @param[in] local_get_offset  Offset from the start of the memory descriptor
 *                              referenced by the \a get_md_handle to use for
 *                              received data.
 * @param[in] put_md_handle     The memory descriptor handle that describes the
 *                              memory to be sent. If the memory descriptor has
 *                              an event queue associated with it, it will be
 *                              used to record events when the message has been
 *                              sent.
 * @param[in] local_put_offset  Offset from the start of the memory descriptor
 *                              referenced by the \a put_md_handle to use for
 *                              transmitted data.
 * @param[in] length            Length of the memory region to be sent and/or
 *                              received. The \a length field must be less than
 *                              or equal to max_atomic_size.
 * @param[in] target_id         A progress identifier for the \e target
 *                              process.
 * @param[in] pt_index          The index in the \e target portal table.
 * @param[in] match_bits        The match bits to use for message selection at
 *                              the \e target process.
 * @param[in] remote_offset     The offset into the target memory region (used
 *                              unless the target match list entry has the \c
 *                              PTL_ME_MANAGE_LOCAL option set).
 * @param[in] user_ptr          See the discussion for PtlPut().
 * @param[in] hdr_data          64 bits of user data that can be included in
 *                              the message header. This data is written to an
 *                              event queue entry at the target if an event
 *                              queue is present on the match list entry that
 *                              the message matches.
 * @param[in] operation         The operation to be performed using the
 *                              initiator and target data.
 * @param[in] datatype          The type of data being operated on at the
 *                              initiator and target.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a put_md_handle is not a
 *                              valid memory descriptor, \a get_md_handle is
 *                              not a valid memory descriptor, or \a target_id
 *                              is not a valid process identifier.
 */
int PtlFetchAtomic(ptl_handle_md_t  get_md_handle,
                   ptl_size_t       local_get_offset,
                   ptl_handle_md_t  put_md_handle,
                   ptl_size_t       local_put_offset,
                   ptl_size_t       length,
                   ptl_process_t    target_id,
                   ptl_pt_index_t   pt_index,
                   ptl_match_bits_t match_bits,
                   ptl_size_t       remote_offset,
                   void            *user_ptr,
                   ptl_hdr_data_t   hdr_data,
                   ptl_op_t         operation,
                   ptl_datatype_t   datatype);
/*!
 * @fn PtlSwap(ptl_handle_md_t  get_md_handle,
 *             ptl_size_t       local_get_offset,
 *             ptl_handle_md_t  put_md_handle,
 *             ptl_size_t       local_put_offset,
 *             ptl_size_t       length,
 *             ptl_process_t    target_id,
 *             ptl_pt_index_t   pt_index,
 *             ptl_match_bits_t match_bits,
 *             ptl_size_t       remote_offset,
 *             void            *user_ptr,
 *             ptl_hdr_data_t   hdr_data,
 *             const void *     operand,
 *             ptl_op_t         operation,
 *             ptl_datatype_t   datatype)
 * @brief Perform a swap operation.
 * @details Provides an extra argument (the \a operand) beyond the
 *      PtlFetchAtomic() function. PtlSwap() handles the \c PTL_SWAP, \c
 *      PTL_CSWAP (and variants), and \c PTL_MSWAP operations and is subject to
 *      the additional restriction that \c PTL_CSWAP (and variants) and \c
 *      PTL_MSWAP operations can only be as long as a single datatype item.
 *      Like PtlFetchAtomic(), receiving a \c PTL_EVENT_REPLY inherently
 *      implies that the flow control check has passed on the target node. In
 *      addition, it is an error to use memory descriptors bound to different
 *      network interfaces in a single PtlSwap() call. The behavior that
 *      occurs when the \a local_get_offset into the \a get_md_handle overlaps
 *      with the \a local_put_offset into the \a put_md_handle is undefined.
 *      Operations performed by PtlSwap() are constrained to be no more than \a
 *      max_fetch_atomic_size bytes and must be aligned at the target
 *      to the size of ptl_datatype_t passed in the \a datatype argument. \c
 *      PTL_CSWAP and \c PTL_MSWAP operations are further restricted to one
 *      item, whose size is defined by the size of the datatype used.
 * @param[in] get_md_handle     The memory descriptor handle that describes the
 *                              memory into which the result of the operation
 *                              will be placed. The memory descriptor can have
 *                              an event queue associated with it to record
 *                              events, such as when the result of the
 *                              operation has been returned.
 * @param[in] local_get_offset  Offset from the start of the memory descriptor
 *                              referenced by the \a get_md_handle to use for
 *                              received data.
 * @param[in] put_md_handle     The memory descriptor handle that describes the
 *                              memory to be sent. If the memory descriptor has
 *                              an event queue associated with it, it will be
 *                              used to record events when the message has been
 *                              sent.
 * @param[in] local_put_offset  Offset from the start of the memory descriptor
 *                              referenced by the put_md_handle to use for
 *                              transmitted data.
 * @param[in] length            Length of the memory region to be sent and/or
 *                              received. The \a length field must be less than
 *                              or equal to max_atomic_size for \c PTL_SWAP
 *                              operations and can only be as large as a single
 *                              datatype item for \c PTL_CSWAP and \c PTL_MSWAP
 *                              operations, and variants of those.
 * @param[in] target_id         A process identifier for the \e target process.
 * @param[in] pt_index          The index in the \e target portal table.
 * @param[in] match_bits        The match bits to use for message selection at
 *                              the \e target process.
 * @param[in] remote_offset     The offset into the target memory region (used
 *                              unless the target match list entry has the \c
 *                              PTL_ME_MANAGE_LOCAL option set).
 * @param[in] user_ptr          See the discussion for PtlPut()
 * @param[in] hdr_data          64 bits of user data that can be included in
 *                              the message header. This data is written to an
 *                              event queue entry at the \e target if an event
 *                              queue is present on the match list entry that
 *                              the message matches.
 * @param[in] operand           A pointer to the data to be used for the \c
 *                              PTL_CSWAP (and variants) and \c PTL_MSWAP
 *                              operations (ignored for other operations). The
 *                              data pointed to is of the type specified by the
 *                              \a datatype argument and must be included in
 *                              the message.
 * @param[in] operation         The operation to be performed using the
 *                              initiator and target data.
 * @param[in] datatype          The type of data being operated on at the
 *                              initiator and target.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a put_md_handle is not a
 *                              valid memory descriptor, \a get_md_handle is
 *                              not a valid memory descriptor, or \a target_id
 *                              is not a valid process identifier.
 */
int PtlSwap(ptl_handle_md_t  get_md_handle,
            ptl_size_t       local_get_offset,
            ptl_handle_md_t  put_md_handle,
            ptl_size_t       local_put_offset,
            ptl_size_t       length,
            ptl_process_t    target_id,
            ptl_pt_index_t   pt_index,
            ptl_match_bits_t match_bits,
            ptl_size_t       remote_offset,
            void            *user_ptr,
            ptl_hdr_data_t   hdr_data,
            const void      *operand,
            ptl_op_t         operation,
            ptl_datatype_t   datatype);
/*!
 * @fn PtlAtomicSync(void)
 * @brief Synchronize atomic accesses through the API.
 * @details Synchronizes the atomic accesses through the Portals API with
 *      accesses by the host. When a data item is accessed by a Portals atomic
 *      operation, modification of the same data item by the host or by an
 *      atomic operation using a different datatype can lead to undefined
 *      behavior. When PtlAtomicSync() is called, it will block until it is
 *      safe for the host (or other atomic operations with a different
 *      datatype) to modify the data items touched by previous Portals atomic
 *      operations. PtlAtomicSync() is called at the target of atomic
 *      operations.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @implnote The atomicity definition for Portals allows a network interface to
 *      offload atomic operations and to have a noncoherent cache on the
 *      network interface. With a noncoherent cache, any access to a memory
 *      location by an atomic operation makes it impossible to safely modify
 *      that location on the host. PtlAtomicSync() is provided to make
 *      modifications from the host safe again.
 */
int PtlAtomicSync(void);
/*! @} */
/*! @} */

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
    /*! A \p get operation completed on the \e target. Portals will not read
     * from memory on behalf of this operation once this event has been logged.
     */
    PTL_EVENT_GET,

    /*! A list entry posted by PtlLEAppend() or PtlMEAppend() matched a \p get
     * header in the unexpected list. */
    PTL_EVENT_GET_OVERFLOW,

    /*! A \p put operation completed at the \e target. Portals will not alter
     * the memory (on behalf of this operation) once this event has been
     * logged. */
    PTL_EVENT_PUT, // 2

    /*! A list entry posted by PtlLEAppend() or PtlMEAppend() matched a \p put
     * header in the unexpected list. */
    PTL_EVENT_PUT_OVERFLOW,

    /*! An \p atomic operation that does not return data to the \e initiator
     * completed at the \e target. Portals will not read from or alter memory
     * on behalf of this operation once this event has been logged. */
    PTL_EVENT_ATOMIC,

    /*! A list entry posted by PtlLEAppend() or PtlMEAppend() matched an \p atomic
     * header in the unexpected list for an operation which does not return
     * data to the \e initiator. */
    PTL_EVENT_ATOMIC_OVERFLOW,

    /*! An \p atomic operation that returns data to the \e initiator completed
     * at the \e target. Portals will not read from or alter memory on behalf
     * of this operation once this event has been logged. */
    PTL_EVENT_FETCH_ATOMIC,

    /*! A list entry posted by PtlLEAppend() or PtlMEAppend() matched an \p atomic
     * header in the unexpected list for an operation which returns data to the
     * \e initiator. */
    PTL_EVENT_FETCH_ATOMIC_OVERFLOW,

    /*! A \p reply operation has completed at the \e initiator. This event is
     * logged after the data (if any) from the reply has been written into the
     * memory descriptor. */
    PTL_EVENT_REPLY,

    /*! A \p send operation has completed at the \e initiator. This event is
     * logged after it is safe to reuse the buffer, but does not mean the
     * message has been processed by the \e target. */
    PTL_EVENT_SEND, // 9

    /*! An \p acknowledgment was received. This event is logged when the
     * acknowledgment is received. Receipt of a \c PTL_EVENT_ACK indicates
     * remote completion of the operation. Remote completion indicates that
     * local completion has also occurred. */
    PTL_EVENT_ACK,

    /*! Resource exhaustion has occurred on this portal table entry, which has
     * entered a flow control situation. */
    PTL_EVENT_PT_DISABLED,

    /*! A list entry posted by PtlLEAppend() or PtlMEAppend() has successfully
     * linked into the specified list. */
    PTL_EVENT_LINK,

    /*! A list entry/match list entry was automatically unlinked. A \c
     * PTL_EVENT_AUTO_UNLINK event is generated even if the list entry/match
     * list entry passed into the PtlLEAppend()/PtlMEAppend() operation was
     * marked with the \c PTL_LE_USE_ONCE / \c PTL_ME_USE_ONCE option and found
     * a corresponding unexpected message before being "linked" into the
     * priority list. */
    PTL_EVENT_AUTO_UNLINK,

    /*! A list entry/match list entry previously automatically unlinked from
     * the overflow list is now free to be reused by the application. A \c
     * PTL_EVENT_AUTO_FREE event is generated when Portals will not generate
     * any further events which resulted from messages delivered into the
     * specified overflow list entry. This also indicates that the unexpected
     * list contains no more items associated with this entry. */
    PTL_EVENT_AUTO_FREE,

    /*! A PtlLESearch() or PtlMESearch() call completed. If a matching message was
     * found in the unexpected list, \c PTL_NI_OK is returned in the \a
     * ni_fail_type field of the event and the event queue entries are filled
     * in as if it were an overflow event. Otherwise, a failure is recorded in
     * the \a ni_fail_type field using \c PTL_NI_NO_MATCH, the \a user_ptr is
     * filled in correctly, and the other fields are undefined. */
    PTL_EVENT_SEARCH
} ptl_event_kind_t;
/*!
 * @struct ptl_event_t
 * @brief An event queue contains ptl_event_t structures, which contain a \a
 *      type and a union of the \e target specific event structure and the \e
 *      initiator specific event structure. */
typedef struct {
    /*! The starting location (virtual, byte address) where the message has
     * been placed. The \a start variable is the sum of the \a start variable
     * in the match list entry and the offset used for the operation. The
     * offset can be determined by the operation for a remote managed match
     * list entry or by the local memory descriptor.
     *
     * When the PtlMEAppend() call matches a message that has arrived in the
     * unexpected list, the start address points to the address in the overflow
     * list where the matching message resides. This may require the
     * application to copy the message to the desired buffer.
     */
    void *start;

    /*! A user-specified value that is associated with each command that can
     * generate an event. */
    void *user_ptr;

    /*! 64 bits of out-of-band user data. */
    ptl_hdr_data_t hdr_data;

    /*! The match bits specified by the \e initiator. */
    ptl_match_bits_t match_bits;

    /*! The length (in bytes) specified in the request. */
    ptl_size_t rlength;

    /*! The length (in bytes) of the data that was manipulated by the
     * operation. For truncated operations, the manipulated length will be the
     * number of bytes specified by the memory descriptor operation (possibly
     * with an offset). For all other operations, the manipulated length will
     * be the length of the requested operation. */
    ptl_size_t mlength;

    /*! The offset requested/used by the other end of the link. At the
     * initiator, this is the displacement (in bytes) into the memory region
     * that the operation used at the target. The offset can be determined by
     * the operation for a remote managed offset in a match list entry or by
     * the match list entry at the target for a locally managed offset. At the
     * target, this is the offset requested by th initiator. */
    ptl_size_t remote_offset;

    /*! The user identifier of the \e initiator. */
    ptl_uid_t uid;

    /*! The identifier of the \e initiator. */
    ptl_process_t initiator;

    /*! Indicates the type of the event. */
    ptl_event_kind_t type;

    /*! The portal table index where the message arrived. */
    ptl_pt_index_t pt_index;

    /*! Is used to convey the failure of an operation. Success is indicated by
     * \c PTL_NI_OK. */
    ptl_ni_fail_t ni_fail_type;

    /*! If this event corresponds to an atomic operation, this indicates the
     * atomic operation that was performed. */
    ptl_op_t atomic_operation;

    /*! If this event corresponds to an atomic operation, this indicates the
     * data type of the atomic operation that was performed. */
    ptl_datatype_t atomic_type;
} ptl_event_t;
/*!
 * @fn PtlEQAlloc(ptl_handle_ni_t   ni_handle,
 *                ptl_size_t        count,
 *                ptl_handle_eq_t * eq_handle)
 * @brief Create an event queue.
 * @details Used to build an event queue.
 * @param[in] ni_handle     The interface handle with which the event queue
 *                          will be associate.
 * @param[in] count         A hint as to the number of events to be stored in
 *                          the event queue. An implementation may provide
 *                          space for more than the requested number of event
 *                          queue slots.
 * @param[out] eq_handle    On successful return, this location will hold the
 *                          newly created event queue handle.
 * @note An event queue has room for at least count number of events. The event
 *      queue is circular. If flow control is not enabled on the portal table
 *      entry, then older events will be overwritten by new ones if they are
 *      not removed in time by the user -- using the functions PtlEQGet(),
 *      PtlEQWait(), or PtlEQPoll(). It is up to the user to determine the
 *      appropriate size of the event queue to prevent this loss of events.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a ni_handle is not a valid
 *                              network interface handle.
 * @retval PTL_NO_SPACE         Indicates that there is insufficient memory to
 *                              allocate the event queue.
 * @implnote The event queue is designed to reside in user space.
 *      High-performance implementations can be designed so they only need to
 *      write to the event queue but never have to read from it. This limits
 *      the number of protection boundary crossings to update the event queue.
 *      However, implementors are free to place the event queue anywhere they
 *      like; inside the kernel or the NIC for example.
 * @implnote Because flow control may be enabled on the portal table entries
 *      that this EQ is attached to, the implementation should ensure that the
 *      space allocated for the EQ is large enough to hold the requested number
 *      of events plus the number of portal table entries associated with this
 *      \a ni_handle. For each PtlPTAlloc() that enables flow control and uses
 *      a given EQ, one space should be reserved for a \c PTL_EVENT_PT_DISABLED
 *      event associated with that EQ.
 * @see PtlEQFree()
 */
int PtlEQAlloc(ptl_handle_ni_t  ni_handle,
               ptl_size_t       count,
               ptl_handle_eq_t *eq_handle);
/*!
 * @fn PtlEQFree(ptl_handle_eq_t eq_handle)
 * @brief Release the resources for an event queue.
 * @details Releases the resources associated with an event queue. It is up to
 *      the user to ensure that no memory descriptors or match list entries are
 *      associated with the event queue once it is freed.
 * @param[in] eq_handle The event queue handle to be released
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a eq_handle is not a valid
 *                              event queue handle.
 * @see PtlEQAlloc()
 */
int PtlEQFree(ptl_handle_eq_t eq_handle);
/*!
 * @fn PtlEQGet(ptl_handle_eq_t eq_handle,
 *              ptl_event_t *   event)
 * @brief Get the next event from an event queue.
 * @details A nonblocking function that can be used to get the next event in an
 *      event queue. The event is removed from the queue.
 * @param[in] eq_handle The event queue handle.
 * @param[out] event    On successful return, this location will hold the
 *                      values associated with the next event in the event queue.
 * @retval PTL_OK           Indicates success
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a eq_handle is not a valid event
 *                          queue handle or \a event is \c NULL.
 * @retval PTL_EQ_EMPTY     Indicates that \a eq_handle is empty or another
 *                          thread is waiting in PtlEQWait().
 * @retval PTL_EQ_DROPPED   Indicates success (i.e., an event is returned) and
 *                          that at least one event between this event and the
 *                          last event obtained -- using PtlEQGet(),
 *                          PtlEQWait(), or PtlEQPoll() -- from this event
 *                          queue has been dropped due to limited space in the
 *                          event queue.
 * @implnote The return code of \c PTL_INTERRUPTED adds an unfortunate degree of
 *      complexity to the PtlEQGet(), PtlEQWait(), and PtlEQPoll() functions;
 *      however, it was deemed necessary to be able to interrupt waiting
 *      functions for the sake of applications that need to tolerate failures.
 *      Hence, this approach to dealing with the conflict of reading and
 *      freeing events was chosen.
 * @see PtlEQWait(), PtlEQPoll()
 */
int PtlEQGet(ptl_handle_eq_t eq_handle,
             ptl_event_t    *event);
/*!
 * @fn PtlEQWait(ptl_handle_eq_t    eq_handle,
 *               ptl_event_t *      event)
 * @brief Wait for a new event in an event queue.
 * @details Used to block the calling process or thread until there is an event
 *      in an event queue. This function returns the next event in the event
 *      queue and removes this event from the queue. In the event that multiple
 *      threads are waiting on the same event queue, PtlEQWait() is guaranteed
 *      to wake exactly one thread, but the order in which they are awakened is
 *      not specified.
 * @param[in] eq_handle     The event queue handle to wait on. The calling
 *                          process (thread) will be blocked until the event
 *                          queue is not empty.
 * @param[out] event        On successful return, this location will hold the
 *                          values associated with the next even tin the event
 *                          queue.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that \a eq_handle is not a valid
 *                              event queue handle or that \a event is \c NULL.
 * @retval PTL_EQ_DROPPED       Indicates success (i.e., an event is returned)
 *                              and that at least one event between this event
 *                              and the last event obtained -- using
 *                              PtlEQGet(), PtlEQWait(), or PtlEQPoll() -- from
 *                              this event queue has been dropped due to
 *                              limited space in the event queue.
 * @retval PTL_INTERRUPTED      Indicates that PtlEQFree() or PtlNIFini() was
 *                              called by another thread while this thread was
 *                              waiting in PtlEQWait().
 * @see PtlEQGet(), PtlEQPoll()
 */
int PtlEQWait(ptl_handle_eq_t eq_handle,
              ptl_event_t    *event);
/*!
 * @fn PtlEQPoll(const ptl_handle_eq_t *eq_handles,
 *               unsigned int           size,
 *               ptl_time_t             timeout,
 *               ptl_event_t           *event,
 *               unsigned int          *which)
 * @brief Poll for a new event on multiple event queues.
 * @details Looks for an event from a set of event queues. Should an event arrive
 *      on any of the queues contained in the array of event queue handles, the
 *      event will be returned in \a event and \a which will contain the index
 *      of the event queue from which the event was taken.
 *
 *      If PtlEQPoll() returns success, the corresponding event is consumed.
 *      PtlEQPoll() provides a timeout to allow applications to poll, block for
 *      a fixed period, or block indefinitely. PtlEQPoll() is sufficiently
 *      general to implement both PtlEQGet() and PtlEQWait(), but these
 *      functions have been retained in the PI for backward compatibility.
 * @implnote PtlEQPoll() should poll the list of queues in a round-robin
 *      fashion. This cannot guarantee fairness but meets common expectations.
 * @param[in] eq_handles    An array of event queue handles. All the handles
 *                          must refer to the same interface.
 * @param[in] size          Length of the array.
 * @param[in] timeout       Time in milliseconds to wait for an event to occur
 *                          in one of the event queue handles. The constant \c
 *                          PTL_TIME_FOREVER can be used to indicate an
 *                          infinite timeout.
 * @param[out] event        On successful return (\c PTL_OK or \c
 *                          PTL_EQ_DROPPED), this location will hold the values
 *                          associated with the next event in the queue.
 * @param[out] which        On successful return, this location will contain
 *                          the index into \a eq_handles of the event queue
 *                          from which the event was taken.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that an invalid argument was passed.
 *                              The definition of which arguments are checked
 *                              is implementation dependent.
 * @retval PTL_EQ_EMPTY         Indicates that the timeout has been reached and
 *                              all of the event queues are empty.
 * @retval PTL_EQ_DROPPED       Indicates success (i.e., an event is returned)
 *                              and that at least one event between this event
 *                              and the last event obtained from the event
 *                              queue indicated by \a which has been dropped
 *                              due to limited space in the event queue.
 * @retval PTL_INTERRUPTED      Indicates that PtlEQFree() or PtlNIFini() was
 *                              called by another thread while this thread was
 *                              waiting in PtlEQPoll().
 * @implnote Implementations are free to provide macros for PtlEQGet() and
 *      PtlEQWait() that use PtlEQPoll() instead of providing these functions.
 * @implnote Not all of the members of the ptl_event_t structure (and
 *      corresponding ptl_initiator_event_t or ptl_target_event_t sub-fields)
 *      are relevant to all operations. The offset and mlength fields of a
 *      ptl_initiator_event_t are undefined for a \c PTL_EVENT_SEND event. The
 *      atomic_operation and atomic_type fields of a ptl_target_event_t are
 *      undefined for operations other than atomic operations. The match_bits
 *      field is undefined for a non-matching NI. All other fields must be
 *      filled in with valid information.
 * @see PtlEQGet(), PtlEQWait()
 */
int PtlEQPoll(const ptl_handle_eq_t *eq_handles,
              unsigned int           size,
              ptl_time_t             timeout,
              ptl_event_t           *event,
              unsigned int          *which);
/*! @} */

/************************
* Triggered Operations *
************************/
/*!
 * @addtogroup TO Triggered Operations
 * @{
 * For a variety of scenarios, it is desirable to setup a response to incoming
 * messages. As an example, a tree based reduction operation could be performed
 * by having each layer of the tree issue a PtlAtomic() operation to its parent
 * after receiving a PtlAtomic() from all of its children. To provide this
 * operation, triggered versions of each of the data movement operations are
 * provided. To create a triggered operation, a \a trig_ct_handle and an
 * integer \a threshold are added to the argument list. When the count (the sum
 * of the success and failure fields) referenced by the \a trig_ct_handle
 * argument reaches or exceeds the \a threshold (equal to or greater), the
 * operation proceeds \e at \e the \e initiator \e of \e the \e operation. For
 * example, a PtlTriggeredGet() or a PtlTriggeredAtomic() will not leave the \e
 * initiator until the threshold is reached. A triggered operation does not use
 * the state of the buffer when the application calls the Portals function.
 * Instead, it uses the state of the buffer after the threshold condition is
 * met. Pending triggered operations can be canceled using
 * PtlCTCancelTriggered().
 *
 * @note The use of a \a trig_ct_handle and \a threshold enables a variety of
 *      usage models. A single match list entry can trigger one operation (or
 *      several) by using an independent \a trig_ct_handle on the match list
 *      entry. One operation can be triggered by a combination of previous
 *      events (include a combination of initiator and target side events) by
 *      having all of the earlier operations reference a single \a
 *      trig_ct_handle and using an appropriate threshold.
 * @implnote The semantics of triggered operations imply that (at a minimum)
 *      operations will proceed in the order that their trigger threshold is
 *      reached. A quality implementation will also release operations that
 *      reach their threshold simultaneously on the same \a trig_ct_handle in
 *      the order that they are issued.
 *
 * @implnote The most straightforward way to implement triggered operations is
 *      to associate a list of dependent operations with the structure
 *      referenced by a \a trig_ct_handle. Operations depending on the same \a
 *      trig_ct_handle with the same \a threshold \e should proceed in the
 *      order that they were issued; thus, the list of operations associated
 *      with a \a trig_ct_handle may be sorted for faster searching.
 *
 * @implnote The triggered operation is released when the counter referenced by
 *      the \a trig_ct_handle reaches or exceeds the \a threshold. This means
 *      that the triggered operation must check the value of the \a
 *      trig_ct_handle in an atomic way when it is first associated with the \a
 *      trig_ct_handle.
 *
 * @fn PtlTriggeredPut(ptl_handle_md_t  md_handle,
 *                     ptl_size_t       local_offset,
 *                     ptl_size_t       length,
 *                     ptl_ack_req_t    ack_req,
 *                     ptl_process_t    target_id,
 *                     ptl_pt_index_t   pt_index,
 *                     ptl_match_bits_t match_bits,
 *                     ptl_size_t       remote_offset,
 *                     void            *user_ptr,
 *                     ptl_hdr_data_t   hdr_data,
 *                     ptl_handle_ct_t  trig_ct_handle,
 *                     ptl_size_t       threshold)
 * @brief Perform a triggered \e put operation.
 * @details Adds triggered operation semantics to the PtlPut() function.
 * @param[in] md_handle         See PtlPut()
 * @param[in] local_offset      See PtlPut()
 * @param[in] length            See PtlPut()
 * @param[in] ack_req           See PtlPut()
 * @param[in] target_id         See PtlPut()
 * @param[in] pt_index          See PtlPut()
 * @param[in] match_bits        See PtlPut()
 * @param[in] remote_offset     See PtlPut()
 * @param[in] user_ptr          See PtlPut()
 * @param[in] hdr_data          See PtlPut()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor, \a target_id is not a
 *                              valid process identifier, or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredPut(ptl_handle_md_t  md_handle,
                    ptl_size_t       local_offset,
                    ptl_size_t       length,
                    ptl_ack_req_t    ack_req,
                    ptl_process_t    target_id,
                    ptl_pt_index_t   pt_index,
                    ptl_match_bits_t match_bits,
                    ptl_size_t       remote_offset,
                    void            *user_ptr,
                    ptl_hdr_data_t   hdr_data,
                    ptl_handle_ct_t  trig_ct_handle,
                    ptl_size_t       threshold);
/*!
 * @fn PtlTriggeredGet(ptl_handle_md_t  md_handle,
 *                     ptl_size_t       local_offset,
 *                     ptl_size_t       length,
 *                     ptl_process_t    target_id,
 *                     ptl_pt_index_t   pt_index,
 *                     ptl_match_bits_t match_bits,
 *                     ptl_size_t       remote_offset,
 *                     void            *user_ptr,
 *                     ptl_handle_ct_t  trig_ct_handle,
 *                     ptl_size_t       threshold)
 * @brief Perform a triggered \e get operation.
 * @details Adds triggerd operation semantics to the PtlGet() function.
 * @param[in] md_handle         See PtlGet()
 * @param[in] target_id         See PtlGet()
 * @param[in] pt_index          See PtlGet()
 * @param[in] match_bits        See PtlGet()
 * @param[in] user_ptr          See PtlGet()
 * @param[in] remote_offset     See PtlGet()
 * @param[in] local_offset      See PtlGet()
 * @param[in] length            See PtlGet()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor, \a target_id is not a
 *                              valid process identifier, or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredGet(ptl_handle_md_t  md_handle,
                    ptl_size_t       local_offset,
                    ptl_size_t       length,
                    ptl_process_t    target_id,
                    ptl_pt_index_t   pt_index,
                    ptl_match_bits_t match_bits,
                    ptl_size_t       remote_offset,
                    void            *user_ptr,
                    ptl_handle_ct_t  trig_ct_handle,
                    ptl_size_t       threshold);
/*!
 * @fn PtlTriggeredAtomic(ptl_handle_md_t   md_handle,
 *                        ptl_size_t        local_offset,
 *                        ptl_size_t        length,
 *                        ptl_ack_req_t     ack_req,
 *                        ptl_process_t     target_id,
 *                        ptl_pt_index_t    pt_index,
 *                        ptl_match_bits_t  match_bits,
 *                        ptl_size_t        remote_offset,
 *                        void             *user_ptr,
 *                        ptl_hdr_data_t    hdr_data,
 *                        ptl_op_t          operation,
 *                        ptl_datatype_t    datatype,
 *                        ptl_handle_ct_t   trig_ct_handle,
 *                        ptl_size_t        threshold)
 * @brief Perform a triggered atomic operation.
 * @details Extends PtlAtomic() with triggered semantics.
 * @param[in] md_handle         See PtlAtomic()
 * @param[in] local_offset      See PtlAtomic()
 * @param[in] length            See PtlAtomic()
 * @param[in] ack_req           See PtlAtomic()
 * @param[in] target_id         See PtlAtomic()
 * @param[in] pt_index          See PtlAtomic()
 * @param[in] match_bits        See PtlAtomic()
 * @param[in] remote_offset     See PtlAtomic()
 * @param[in] user_ptr          See PtlAtomic()
 * @param[in] hdr_data          See PtlAtomic()
 * @param[in] operation         See PtlAtomic()
 * @param[in] datatype          See PtlAtomic()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor, \a target_id is not a
 *                              valid process identifier, or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredAtomic(ptl_handle_md_t  md_handle,
                       ptl_size_t       local_offset,
                       ptl_size_t       length,
                       ptl_ack_req_t    ack_req,
                       ptl_process_t    target_id,
                       ptl_pt_index_t   pt_index,
                       ptl_match_bits_t match_bits,
                       ptl_size_t       remote_offset,
                       void            *user_ptr,
                       ptl_hdr_data_t   hdr_data,
                       ptl_op_t         operation,
                       ptl_datatype_t   datatype,
                       ptl_handle_ct_t  trig_ct_handle,
                       ptl_size_t       threshold);
/*!
 * @fn PtlTriggeredFetchAtomic(ptl_handle_md_t  get_md_handle,
 *                             ptl_size_t       local_get_offset,
 *                             ptl_handle_md_t  put_md_handle,
 *                             ptl_size_t       local_put_offset,
 *                             ptl_size_t       length,
 *                             ptl_process_t    target_id,
 *                             ptl_pt_index_t   pt_index,
 *                             ptl_match_bits_t match_bits,
 *                             ptl_size_t       remote_offset,
 *                             void            *user_ptr,
 *                             ptl_hdr_data_t   hdr_data,
 *                             ptl_op_t         operation,
 *                             ptl_datatype_t   datatype,
 *                             ptl_handle_ct_t  trig_ct_handle,
 *                             ptl_size_t       threshold)
 * @brief Perform a triggered fetch and atomic operation.
 * @details Extends PtlFetchAtomic() with triggered semantics.
 * @param[in] get_md_handle     See PtlFetchAtomic()
 * @param[in] local_get_offset  See PtlFetchAtomic()
 * @param[in] put_md_handle     See PtlFetchAtomic()
 * @param[in] local_put_offset  See PtlFetchAtomic()
 * @param[in] length            See PtlFetchAtomic()
 * @param[in] target_id         See PtlFetchAtomic()
 * @param[in] pt_index          See PtlFetchAtomic()
 * @param[in] match_bits        See PtlFetchAtomic()
 * @param[in] remote_offset     See PtlFetchAtomic()
 * @param[in] user_ptr          See PtlFetchAtomic()
 * @param[in] hdr_data          See PtlFetchAtomic()
 * @param[in] operation         See PtlFetchAtomic()
 * @param[in] datatype          See PtlFetchAtomic()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor, \a target_id is not a
 *                              valid process identifier, or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredFetchAtomic(ptl_handle_md_t  get_md_handle,
                            ptl_size_t       local_get_offset,
                            ptl_handle_md_t  put_md_handle,
                            ptl_size_t       local_put_offset,
                            ptl_size_t       length,
                            ptl_process_t    target_id,
                            ptl_pt_index_t   pt_index,
                            ptl_match_bits_t match_bits,
                            ptl_size_t       remote_offset,
                            void            *user_ptr,
                            ptl_hdr_data_t   hdr_data,
                            ptl_op_t         operation,
                            ptl_datatype_t   datatype,
                            ptl_handle_ct_t  trig_ct_handle,
                            ptl_size_t       threshold);
/*!
 * @fn PtlTriggeredSwap(ptl_handle_md_t     get_md_handle,
 *                      ptl_size_t          local_get_offset,
 *                      ptl_handle_md_t     put_md_handle,
 *                      ptl_size_t          local_put_offset,
 *                      ptl_size_t          length,
 *                      ptl_process_t       target_id,
 *                      ptl_pt_index_t      pt_index,
 *                      ptl_match_bits_t    match_bits,
 *                      ptl_size_t          remote_offset,
 *                      void               *user_ptr,
 *                      ptl_hdr_data_t      hdr_data,
 *                      const void *        operand,
 *                      ptl_op_t            operation,
 *                      ptl_datatype_t      datatype,
 *                      ptl_handle_ct_t     trig_ct_handle,
 *                      ptl_size_t          threshold)
 * @brief Perform a triggered swap operation.
 * @details Extends PtlSwap() with triggered semantics.
 * @param[in] get_md_handle     See PtlSwap()
 * @param[in] local_get_offset  See PtlSwap()
 * @param[in] put_md_handle     See PtlSwap()
 * @param[in] local_put_offset  See PtlSwap()
 * @param[in] length            See PtlSwap()
 * @param[in] target_id         See PtlSwap()
 * @param[in] pt_index          See PtlSwap()
 * @param[in] match_bits        See PtlSwap()
 * @param[in] remote_offset     See PtlSwap()
 * @param[in] user_ptr          See PtlSwap()
 * @param[in] hdr_data          See PtlSwap()
 * @param[in] operand           See PtlSwap()
 * @param[in] operation         See PtlSwap()
 * @param[in] datatype          See PtlSwap()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a md_handle is not a
 *                              valid memory descriptor, \a target_id is not a
 *                              valid process identifier, or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredSwap(ptl_handle_md_t  get_md_handle,
                     ptl_size_t       local_get_offset,
                     ptl_handle_md_t  put_md_handle,
                     ptl_size_t       local_put_offset,
                     ptl_size_t       length,
                     ptl_process_t    target_id,
                     ptl_pt_index_t   pt_index,
                     ptl_match_bits_t match_bits,
                     ptl_size_t       remote_offset,
                     void            *user_ptr,
                     ptl_hdr_data_t   hdr_data,
                     const void      *operand,
                     ptl_op_t         operation,
                     ptl_datatype_t   datatype,
                     ptl_handle_ct_t  trig_ct_handle,
                     ptl_size_t       threshold);
/*!
 * @fn PtlTriggeredCTInc(ptl_handle_ct_t    ct_handle,
 *                       ptl_ct_event_t     increment,
 *                       ptl_handle_ct_t    trig_ct_handle,
 *                       ptl_size_t         threshold)
 * @brief A triggered increment of a counting event by a certain value.
 * @details The triggered counting event increment extends the counting event
 *      increment (PtlCTInc()) with the triggered operation semantics. It is a
 *      convenient mechanism to provide chaining of dependencies between
 *      counting events. This allows a relatively arbitrary ordering of
 *      operations. For example, a PtlTriggeredPut() and a PtlTriggeredCTInc()
 *      could be dependent on \a ct_handle A with the same threshold. If the
 *      PtlTriggeredCTInc() is set to increment \a ct_handle B and a second
 *      PtlTriggeredPut() is dependent on \a ct_handle B, the second
 *      PtlTriggeredPut() will occur after the first.
 * @param[in] ct_handle         See PtlCTInc()
 * @param[in] increment         See PtlCTInc()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a ct_handle is not a
 *                              valid counting event handle or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredCTInc(ptl_handle_ct_t ct_handle,
                      ptl_ct_event_t  increment,
                      ptl_handle_ct_t trig_ct_handle,
                      ptl_size_t      threshold);
/*!
 * @fn PtlTriggeredCTSet(ptl_handle_ct_t    ct_handle,
 *                       ptl_ct_event_t     new_ct,
 *                       ptl_handle_ct_t    trig_ct_handle,
 *                       ptl_size_t         threshold)
 * @brief A triggered set of a counting event to a certain value.
 * @details Extends the counting event set (PtlCTSet()) with triggered operation
 *      semantics. It is a convenient mechanism to provide reinitialization of
 *      counters between invocations of an algorithm.
 * @param[in] ct_handle         See PtlCTSet()
 * @param[in] new_ct            See PtlCTSet()
 * @param[in] trig_ct_handle    Handle used for triggering the operation.
 * @param[in] threshold         Threshold at which the operation triggers.
 * @retval PTL_OK               Indicates success
 * @retval PTL_NO_INIT          Indicates that the portals API has not been
 *                              successfully initialized.
 * @retval PTL_ARG_INVALID      Indicates that either \a ct_handle is not a
 *                              valid counting event handle or \a trig_ct_handle
 *                              is not a valid counting event handle.
 */
int PtlTriggeredCTSet(ptl_handle_ct_t ct_handle,
                      ptl_ct_event_t  new_ct,
                      ptl_handle_ct_t trig_ct_handle,
                      ptl_size_t      threshold);
/*! @} */

/*************************************
* Deferred Communication Operations *
*************************************/
/*!
 * @addtogroup DCO Deferred Communication Operations
 * @{
 * In many cases, the application has knowledge of its intended usage model
 * that could be used to improve the performance of the implementation if there
 * was a way to convey that knowledge. When an application informs the
 * implementation of an intended usage model, it is binding on the application:
 * the application cannot violate the usage model conveyed to the
 * implementation. In contrast, such information is only a hint to the
 * implementation: the implementation is not required to change its behavior
 * based on the usage model the application describes. One prevalent usage
 * model that many implementations could optimize for is a stream of operations
 * in close temporal proximity. Informing the implementation of an impending
 * stream of operations may allow it to optimize the conveyance of those
 * operations through the messaging system (e.g. across the host bus or even
 * across the network).
 *
 * @fn PtlStartBundle(ptl_handle_ni_t ni_handle)
 * @brief Indicates a group of communication operations is about to start.
 * @details The PtlStartBundle() function is used by the application to
 *      indicate to the implementation that a group of communication operations
 *      is about to start. PtlStartBundle() takes an \a ni_handle as an
 *      argument and only impacts operations on that \a ni_handle.
 *      PtlStartBundle() can be called multiple times, and each call to
 *      PtlStartBundle() increments a reference count and must be matched by a
 *      call to PtlEndBundle(). After a call to PtlStartBundle(), the
 *      implementation may begin deferring communication operations until a
 *      call to PtlEndBundle().
 * @param[in] ni_handle     An interface handle to start bundling operations.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that an invalid argument was passed. The
 *                          definition of which arguments are checked is
 *                          implementation dependent.
 * @note Layered libraries and heavily nested PtlStartBundle() calls can yield
 *      unexpected results. The PtlStartBundle() and PtlEndBundle() interface
 *      was designed for use in short periods of high activity (e.g. during the
 *      setup of a collective operation or duing an inner loop for PGAS
 *      languages). The interval between PtlStartBundle() and the corresponding
 *      PtlEndBundle() should be kept short.
 * @implnote The PtlStartBundle() and PtlEndBundle() interface was designed to
 *      allow the implementation to avoid unnecessary sfence()/memory barrier
 *      operations during periods that the application expects high message
 *      rate usage. A quality implementation will attempt to minimize latency
 *      while maximizing message rate. For example, an implementation that
 *      requires writes into “write-combining” space may require sfence()
 *      operations with every message to have relatively deterministic latency.
 *      Between a PtlStartBundle() and PtlEndBundle(), the implementation might
 *      simply omit the sfence() operations.
 */
int PtlStartBundle(ptl_handle_ni_t ni_handle);

/*!
 * @fn PtlEndBundle(ptl_handle_ni_t ni_handle)
 * @brief Indicates a group of communication operations has ended.
 * @details The PtlEndBundle() function is used by the application to indicate
 *      to the implementation that a group of communication operations has
 *      ended. PtlEndBundle() takes an ni_handle as an argument and only
 *      impacts operations on that ni_handle. PtlEndBundle() must be called
 *      once for each PtlStartBundle() call. At each call to PtlEndBundle(),
 *      the implementation must initiate all communication operations that have
 *      been deferred; however, the implementation is not required to cease
 *      bundling future operations until the reference count reaches zero.
 * @param[in] ni_handle     An interface handle to end bundling operations.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that an invalid argument was passed. The
 *                          definition of which arguments are checked is
 *                          implementation dependent.
 */
int PtlEndBundle(ptl_handle_ni_t ni_handle);
/*! @} */

/*************************
* Operations on Handles *
*************************/
/*!
 * @addtogroup OH Operations on Handles
 * @{
 * @fn PtlHandleIsEqual(ptl_handle_any_t handle1,
 *                      ptl_handle_any_t handle2)
 * @brief Compares two handles to determine if they represent the same object.
 * @details Compares two handles to determine if they represent the same object.
 * @param[in] handle1   An object handle. May be the constant \c PTL_INVALID_HANDLE.
 * @param[in] handle2   An object handle. May be the constant \c PTL_INVALID_HANDLE.
 * @retval PTL_OK       Indicates that the handles are equivalent.
 * @retval PTL_FAIL     Indicates that the handles are not equivalent.
 */
int PtlHandleIsEqual(ptl_handle_any_t handle1,
                     ptl_handle_any_t handle2);
/*! @} */
#endif /* ifndef PORTALS4_H */
/* vim:set expandtab: */
