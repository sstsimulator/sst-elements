/*!
 * @file portals4.h
 * @brief Portals4 Reference Implementation
 *
 * Copyright (c) 2010 Sandia Corporation
 */

#ifndef PORTALS4_H
#define PORTALS4_H

#include <portals4_types.h>

/*************
 * Constants *
 *************/
/*! Indicate the absence of an event queue. */
extern const ptl_handle_eq_t PTL_EQ_NONE;

/*! Indicate the absence of a counting type event. */
extern const ptl_handle_ct_t PTL_CT_NONE;

/*! Represent an invalid handle. */
extern const ptl_handle_any_t PTL_INVALID_HANDLE;

/*! Identify the default interface. */
extern const ptl_interface_t PTL_IFACE_DEFAULT;

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
 * logical interfaces within a single process that are associated witha single
 * physical interface must share a single node ID and Portals process ID.
 */

/*!
 * @fn PtlNIInit(ptl_interface_t    iface,
 *               unsigned int       options,
 *               ptl_pid_t          pid,
 *               ptl_ni_limits_t *  desired,
 *               ptl_ni_limits_t *  actual,
 *               ptl_size_t         map_size,
 *               ptl_process_t *    desired_mapping,
 *               ptl_process_t *    actual_mapping,
 *               ptl_handle_ni_t *  ni_handle)
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
 * @param[in] map_size      Contains the size of the map being passed in. If \a
 *                          map_size is non-zero and \a desired_mapping is \c
 *                          NULL, PtlNIInit() simply returns the actual mapping
 *                          in \a actual_mapping. This field is ignored if the
 *                          \c PTL_NI_LOGICAL option is set.
 * @param[in] desired_mapping   If not \c NULL, points to an array of
 *                              structures that holds the desired mapping of
 *                              logical identifiers to NID/PID pairs. This
 *                              field is ignored if the PTL_NI_LOGICAL option
 *                              is \b not set.
 * @param[out] actual_mapping   If the \c PTL_NI_LOGICAL option is set,
 *                              on successful return, the location pointed to
 *                              by \a actual_mapping will hold the actual
 *                              mapping of logical identifiers to NID/PID
 *                              pairs. The mapping returned will be no longer
 *                              than \a map_size.
 * @param[out] ni_handle        On successful return, this location
 *                              will hold the interface handle.
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
 * @retval PTL_NI_NOT_LOGICAL   Not a logically addressed network interface handle.
 * @retval PTL_NO_SPACE         Indicates that PtlNIInit() was not able to
 *                              allocate the memory required to initialize this
 *                              interface.
 * @see PtlNIFini(), PtlNIStatus()
 */
int PtlNIInit(ptl_interface_t   iface,
              unsigned int      options,
              ptl_pid_t         pid,
              ptl_ni_limits_t   *desired,
              ptl_ni_limits_t   *actual,
              ptl_size_t        map_size,
              ptl_process_t     *desired_mapping,
              ptl_process_t     *actual_mapping,
              ptl_handle_ni_t   *ni_handle);
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
 * then the implementation should fill in \a actual, \a actual_mapping, and \a
 * ni_handle. It should ignore \a pid. PtlGetId() can be used to retrieve the
 * \a pid.
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
                ptl_sr_value_t  *status);
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
int PtlNIHandle(ptl_handle_any_t    handle,
                ptl_handle_ni_t*    ni_handle);
/*! @} */

/************************
 * Portal Table Entries *
 ************************/
/*!
 * @addtogroup PT (PT) Portal Table Entries
 * @{ */

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
int PtlPTAlloc(ptl_handle_ni_t  ni_handle,
               unsigned int     options,
               ptl_handle_eq_t  eq_handle,
               ptl_pt_index_t   pt_index_req,
               ptl_pt_index_t*  pt_index);
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
int PtlPTFree(ptl_handle_ni_t   ni_handle,
              ptl_pt_index_t    pt_index);
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
int PtlPTDisable(ptl_handle_ni_t    ni_handle,
                 ptl_pt_index_t     pt_index);
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
int PtlGetUid(ptl_handle_ni_t   ni_handle,
              ptl_uid_t*        uid);
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
int PtlGetId(ptl_handle_ni_t    ni_handle,
             ptl_process_t*     id);
/*! @} */
/***********************
 * Process Aggregation *
 ***********************/
/*!
 * @addtogroup PA Process Aggregation
 * @{
 * @fn PtlGetJid(ptl_handle_ni_t    ni_handle,
 *               ptl_jid_t*         jid)
 * @brief Get the job identifier for the current process.
 * @details Retrieves the job identifier of the calling process.
 *      It is useful in the context of a parallel machine to represent all of
 *      the processes in a parallel job through an aggregate identifier. The
 *      portals API provides a mechanism for supporting such job identifiers
 *      for these systems. In order to be fully supported, job identifiers must
 *      be included as a trusted part of a message header.
 *
 *      The job identifier is an opaque identifier shared between all of the
 *      distributed processes of an application running on a parallel machine.
 *      All application processes and job-specific support programs, such as
 *      the parallel job launcher, share the same job identifier. This
 *      identifier is assigned by the runtime system upon job launch and is
 *      guaranteed to be unique among application jobs currently running on the
 *      entire distributed system. An individual serial process may be assigned
 *      a job identifier that is not shared with any other processes in the
 *      system or can be assigned the constant \c PTL_JID_NONE.
 * @param[in] ni_handle A network interface handle.
 * @param[out] jid      On successful return, this location will hold the
 *                      job identifier for the calling process. \c PTL_JID_NONE
 *                      may be returned for a serial job, if a job identifier
 *                      is not assigned.
 * @retval PTL_OK           Indicates success.
 * @retval PTL_NO_INIT      Indicates that the portals API has not been
 *                          successfully initialized.
 * @retval PTL_ARG_INVALID  Indicates that \a ni_handle is not a valid network
 *                          interface handle
 */
int PtlGetJid(ptl_handle_ni_t   ni_handle,
             ptl_jid_t*         jid);
/*! @} */
/**********************
 * Memory Descriptors *
 **********************/
/*!
 * @addtogroup MD (MD) Memory Descriptors
 * @{ */

/*!
 * @fn PtlMDBind(ptl_handle_ni_t    ni_handle,
 *               ptl_md_t*          md,
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
int PtlMDBind(ptl_handle_ni_t   ni_handle,
              ptl_md_t*         md,
              ptl_handle_md_t*  md_handle);
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
 * @retval PTL_IN_USE       Indicates that \a md_handle has pending operations
 *                          and cannot be released.
 * @see PtlMDBind()
 */
int PtlMDRelease(ptl_handle_md_t md_handle);
/*! @} */

/**************************
 * List Entries and Lists *
 **************************/
/*!
 * @fn PtlLEAppend(ptl_handle_ni_t  ni_handle,
 *                 ptl_pt_index_t   pt_index,
 *                 ptl_le_t *       le,
 *                 ptl_list_t       ptl_list,
 *                 void*            user_ptr,
 *                 ptl_handle_le_t* le_handle)
 * @brief Creates a single list entry and appends this entry to the end of the
 *      list specified by \a ptl_list associated with the portal table entry
 *      specified by \a pt_index for the portal table for \a ni_handle. If the
 *      list is currently uninitialized, the PtlLEAppend() function creates
 *      the first entry in the list.
 *
 *      When a list entry is posted to a list, the overflow list is checked to
 *      see if a message has arrived prior to posting the list entry. If so, a
 *      \c PTL_EVENT_PUT_OVERFLOW event is generated. No searching is performed
 *      when a list entry is posted to the overflow list.
 * @param[in] ni_handle     The interface handle to use.
 * @param[in] pt_index      The portal table index where the list entry should
 *                          be appended.
 * @param[in] le            Provides initial values for the user-visible parts
 *                          of a list entry. Other than its use for
 *                          initialization, there is no linkage between this
 *                          structure and the list entry maintained by the API.
 * @param[in] ptl_list      Determines whether the list entry is appended to
 *                          the priority list, appended to the overflow list,
 *                          or simply queries the overflow list.
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
int PtlLEAppend(ptl_handle_ni_t     ni_handle,
                ptl_pt_index_t      pt_index,
                ptl_le_t *          le,
                ptl_list_t          ptl_list,
                void*               user_ptr,
                ptl_handle_le_t*    le_handle);
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
 * @fn PtlLESearch(ptl_handle_ni_t ni_handle,
 *                 ptl_pt_index_t  pt_index,
 *                 ptl_le_t       *le,
 *                 ptl_search_op_t ptl_search_op,
 *                 void           *user_ptr)
 * @brief Used to search for a message in the unexpected list associated with a
 *      specific portal table entry specified by \a pt_index for the portal
 *      table for \a ni_handle. PtlLESearch() uses the exact same search of the
 *      unexpected list as PtlLEAppend(); however, the list entry specified in
 *      the PtlLESearch() call is never linked into a priority list.
 *
 *      The PtlLESearch() function can be called in two modes. If
 *      ptl_search_op_t is set to PTL_SEARCH_ONLY, the unexpected list is
 *      searched to support the MPI_Probe functionality. If ptl_search_op_t is
 *      set to PTL_SEARCH_DELETE, the unexpected list is searched and any
 *      matching items are deleted. A search of the overflow list will always
 *      generate an event. When used with PTL_SEARCH_ONLY, a PTL_EVENT_SEARCH
 *      event is always generated. If a matching message was found in the
 *      overflow list, PTL_NI_OK is returned in the event. Otherwise, the event
 *      indicates that the search operation failed. When used with
 *      PTL_SEARCH_DELETE, the event that is generated corresponds to the type
 *      of operation that is found (e.g. PTL_EVENT_PUT_OVERFLOW or
 *      PTL_EVENT_ATOMIC_OVERFLOW). If no operation is found, a
 *      PTL_EVENT_SEARCH is generated with a failure indication.
 *
 *      Event generation for the search function works just as it would for an
 *      append function. If a search is performed with events disabled (either
 *      through option or through the absence of an event queue on the portal
 *      table entry), the search will succeed, but no events will be generated.
 *      Status registers, however, are handled slightly differently for a
 *      search in that a PtlLESearch()never causes a status register to be
 *      incremented.
 * @implnote When a persistent LE (or ME) is used to search a list, the entire
 *      overflow list is traversed and multiple matches can be found. This is
 *      because messages arriving in the overflow list are treated like
 *      messages that match in the priority list: a persistent ME/LE would be
 *      able to match multiple incoming messages if it were in the priority
 *      list; hence, it should be able to match multiple unexpected messages in
 *      the overflow list.
 * @note Searches with persistent entries could have unexpected performance and
 *      resource usage characteristics if a large overflow list has
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
                ptl_le_t       *le,
                ptl_search_op_t ptl_search_op,
                void           *user_ptr);
/*! @} */

/********************************************
 * Matching List Entries and Matching Lists *
 ********************************************/
/*!
 * @fn PtlMEAppend(ptl_handle_ni_t      ni_handle,
 *                 ptl_pt_index_t       pt_index,
 *                 ptl_me_t *           me,
 *                 ptl_list_t           ptl_list,
 *                 void *               user_ptr,
 *                 ptl_handle_me_t *    me_handle)
 * @brief Create a match list entry and append it to a portal table.
 * @details Creates a single match list entry. If \c PTL_PRIORITY_LIST or \c
 *      PTL_OVERFLOW is specified by \a ptl_list, this entry is appended to the
 *      end of the appropriate list specified by \a ptl_list associated with
 *      the portal table entry specified by \a pt_index for the portal table
 *      for \a ni_handle. If the list is currently uninitialized, the
 *      PtlMEAppend() function creates the first entry in the list.
 *
 *      When a match list entry is posted to the priority list, the overflow
 *      list is searched to see if a matching message has arrived prior to
 *      posting the match list entry. If so, a \c PTL_EVENT_PUT_OVERFLOW event
 *      is generated. No searching is performed when a match list entry is
 *      posted to the overflow list.
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
 *                          to the priority list, appended to the overflow
 *                          list, or simply queries the overflow list.
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
int PtlMEAppend(ptl_handle_ni_t     ni_handle,
                ptl_pt_index_t      pt_index,
                ptl_me_t *          me,
                ptl_list_t          ptl_list,
                void *              user_ptr,
                ptl_handle_me_t *   me_handle);
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
 *                 ptl_me_t       *me,
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
 *      items are deleted. A search of the overflow list will always generate
 *      an event. When used with PTL_SEARCH_ONLY, a PTL_EVENT_SEARCH event is
 *      always generated. If a matching message was found in the overflow list,
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
                ptl_me_t       *me,
                ptl_search_op_t ptl_search_op,
                void           *user_ptr);
/*! @} */

/*********************************
 * Lightweight "Counting" Events *
 *********************************/
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
int PtlCTAlloc(ptl_handle_ni_t      ni_handle,
               ptl_handle_ct_t *    ct_handle);
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
int PtlCTGet(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t *   event);
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
int PtlCTWait(ptl_handle_ct_t   ct_handle,
              ptl_size_t        test,
              ptl_ct_event_t *  event);
/*!
 * @fn PtlCTPoll(ptl_handle_ct_t *  ct_handles,
 *               ptl_size_t *       tests,
 *               unsigned int       size,
 *               ptl_time_t         timeout,
 *               ptl_ct_event_t *   event,
 *               int *              which)
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
int PtlCTPoll(ptl_handle_ct_t * ct_handles,
              ptl_size_t *      tests,
              unsigned int      size,
              ptl_time_t        timeout,
              ptl_ct_event_t *  event,
              int *             which);
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
int PtlCTSet(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t     test);
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
int PtlCTInc(ptl_handle_ct_t    ct_handle,
             ptl_ct_event_t     increment);
/*! @} */

/****************************
 * Data Movement Operations *
 ****************************/
/*!
 * @fn PtlPut(ptl_handle_md_t   md_handle,
 *            ptl_size_t        local_offset,
 *            ptl_size_t        length,
 *            ptl_ack_req_t     ack_req,
 *            ptl_process_t     target_id,
 *            ptl_pt_index_t    pt_index,
 *            ptl_match_bits_t  match_bits,
 *            ptl_size_t        remote_offset,
 *            void *            user_ptr,
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
 *                          sued by a pointer. This value (along with other
 *                          values) is recorded in \e initiator events
 *                          associated with this \p put operation.
 * @param[in] hdr_data      64 bites of user data that can be included in the
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
           void *           user_ptr,
           ptl_hdr_data_t   hdr_data);
/*!
 * @fn PtlGet(ptl_handle_md_t   md_handle,
 *            ptl_size_t        local_offset,
 *            ptl_size_t        length,
 *            ptl_process_t     target_id,
 *            ptl_pt_index_t    pt_index,
 *            ptl_match_bits_t  match_bits,
 *            ptl_size_t        remote_offset,
 *            void *            user_ptr)
 * @brief Perform a \e get operation.
 * @details Initiates a remote read operation. There are two events associated
 *      with a get operation. When the data is sent from the \e target node, a
 *      \c PTL_EVENT_GET event is registered on the \e target node. When the
 *      data is returned from the \e target node, a \c PTL_EVENT_REPLY event is
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
           void *           user_ptr);

/************************
 * Atomic Operations *
 ************************/
/*!
 * @fn PtlAtomic(ptl_handle_md_t    md_handle,
 *               ptl_size_t         local_offset,
 *               ptl_size_t         length,
 *               ptl_ack_req_t      ack_req,
 *               ptl_process_t      target_id,
 *               ptl_pt_index_t     pt_index,
 *               ptl_match_bits_t   match_bits,
 *               ptl_size_t         remote_offset,
 *               void *             user_ptr,
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
int PtlAtomic(ptl_handle_md_t   md_handle,
              ptl_size_t        local_offset,
              ptl_size_t        length,
              ptl_ack_req_t     ack_req,
              ptl_process_t     target_id,
              ptl_pt_index_t    pt_index,
              ptl_match_bits_t  match_bits,
              ptl_size_t        remote_offset,
              void *            user_ptr,
              ptl_hdr_data_t    hdr_data,
              ptl_op_t          operation,
              ptl_datatype_t    datatype);
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
 *                    void *            user_ptr,
 *                    ptl_hdr_data_t    hdr_data,
 *                    ptl_op_t          operation,
 *                    ptl_datatype_t    datatype)
 * @brief Perform a fetch-and-atomic operation.
 * @details Extends PtlAtomic() to return the value from the target \e prior \e
 *      to \e the \e operation \e being \e performed. This means that both
 *      PtlPut() and PtlGet() style events can be delivered. When data is sent
 *      from the initiator node, a \c PTL_EVENT_SEND event is registered on the
 *      \e initiator node in the event queue specified by the \a put_md_handle.
 *      The even t\c PTL_EVENT_ATOMIC is registered on the \e target node to
 *      indicate completion of an atomic operation; and if data is returned
 *      from the \e target node, a \c PTL_EVENT_REPLY event is registered on
 *      the \e initiator node in the even tqueue specified by the \a
 *      get_md_handle. Note that receiving a \c PTL_EVENT_REPLY inherently
 *      implies that the flow control check has passed on the target node. In
 *      addition, it is an error to use memory descriptors bound to different
 *      network interfaces in a single PtlFetchAtomic() call.
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
                   void *           user_ptr,
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
 *             void *           user_ptr,
 *             ptl_hdr_data_t   hdr_data,
 *             void *           operand,
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
 *      network interfaces in a single PtlSwap() call.
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
int PtlSwap(ptl_handle_md_t     get_md_handle,
            ptl_size_t          local_get_offset,
            ptl_handle_md_t     put_md_handle,
            ptl_size_t          local_put_offset,
            ptl_size_t          length,
            ptl_process_t       target_id,
            ptl_pt_index_t      pt_index,
            ptl_match_bits_t    match_bits,
            ptl_size_t          remote_offset,
            void *              user_ptr,
            ptl_hdr_data_t      hdr_data,
            void *              operand,
            ptl_op_t            operation,
            ptl_datatype_t      datatype);
/*! @} */
/*! @} */

/***************************
 * Events and Event Queues *
 ***************************/
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
int PtlEQAlloc(ptl_handle_ni_t      ni_handle,
               ptl_size_t           count,
               ptl_handle_eq_t *    eq_handle);
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
int PtlEQGet(ptl_handle_eq_t    eq_handle,
             ptl_event_t *      event);
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
int PtlEQWait(ptl_handle_eq_t   eq_handle,
              ptl_event_t *     event);
/*!
 * @fn PtlEQPoll(ptl_handle_eq_t *  eq_handles,
 *               unsigned int       size,
 *               ptl_time_t         timeout,
 *               ptl_event_t *      event,
 *               int *              which)
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
int PtlEQPoll(ptl_handle_eq_t *     eq_handles,
              unsigned int          size,
              ptl_time_t            timeout,
              ptl_event_t *         event,
              int *                 which);
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
 * initiator until the threshold is reached.
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
 *                     void *           user_ptr,
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
int PtlTriggeredPut(ptl_handle_md_t     md_handle,
                    ptl_size_t          local_offset,
                    ptl_size_t          length,
                    ptl_ack_req_t       ack_req,
                    ptl_process_t       target_id,
                    ptl_pt_index_t      pt_index,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_hdr_data_t      hdr_data,
                    ptl_handle_ct_t     trig_ct_handle,
                    ptl_size_t          threshold);
/*!
 * @fn PtlTriggeredGet(ptl_handle_md_t  md_handle,
 *                     ptl_size_t       local_offset,
 *                     ptl_size_t       length,
 *                     ptl_process_t    target_id,
 *                     ptl_pt_index_t   pt_index,
 *                     ptl_match_bits_t match_bits,
 *                     ptl_size_t       remote_offset,
 *                     void *           user_ptr,
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
int PtlTriggeredGet(ptl_handle_md_t     md_handle,
                    ptl_size_t          local_offset,
                    ptl_size_t          length,
                    ptl_process_t       target_id,
                    ptl_pt_index_t      pt_index,
                    ptl_match_bits_t    match_bits,
                    ptl_size_t          remote_offset,
                    void *              user_ptr,
                    ptl_handle_ct_t     trig_ct_handle,
                    ptl_size_t          threshold);
/*!
 * @fn PtlTriggeredAtomic(ptl_handle_md_t   md_handle,
 *                        ptl_size_t        local_offset,
 *                        ptl_size_t        length,
 *                        ptl_ack_req_t     ack_req,
 *                        ptl_process_t     target_id,
 *                        ptl_pt_index_t    pt_index,
 *                        ptl_match_bits_t  match_bits,
 *                        ptl_size_t        remote_offset,
 *                        void *            user_ptr,
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
                       void *           user_ptr,
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
 *                             void *           user_ptr,
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
int PtlTriggeredFetchAtomic(ptl_handle_md_t     get_md_handle,
                            ptl_size_t          local_get_offset,
                            ptl_handle_md_t     put_md_handle,
                            ptl_size_t          local_put_offset,
                            ptl_size_t          length,
                            ptl_process_t       target_id,
                            ptl_pt_index_t      pt_index,
                            ptl_match_bits_t    match_bits,
                            ptl_size_t          remote_offset,
                            void *              user_ptr,
                            ptl_hdr_data_t      hdr_data,
                            ptl_op_t            operation,
                            ptl_datatype_t      datatype,
                            ptl_handle_ct_t     trig_ct_handle,
                            ptl_size_t          threshold);
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
 *                      void *              user_ptr,
 *                      ptl_hdr_data_t      hdr_data,
 *                      void *              operand,
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
int PtlTriggeredSwap(ptl_handle_md_t    get_md_handle,
                     ptl_size_t         local_get_offset,
                     ptl_handle_md_t    put_md_handle,
                     ptl_size_t         local_put_offset,
                     ptl_size_t         length,
                     ptl_process_t      target_id,
                     ptl_pt_index_t     pt_index,
                     ptl_match_bits_t   match_bits,
                     ptl_size_t         remote_offset,
                     void *             user_ptr,
                     ptl_hdr_data_t     hdr_data,
                     void *             operand,
                     ptl_op_t           operation,
                     ptl_datatype_t     datatype,
                     ptl_handle_ct_t    trig_ct_handle,
                     ptl_size_t         threshold);
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
int PtlTriggeredCTInc(ptl_handle_ct_t   ct_handle,
                      ptl_ct_event_t    increment,
                      ptl_handle_ct_t   trig_ct_handle,
                      ptl_size_t        threshold);
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
int PtlTriggeredCTSet(ptl_handle_ct_t   ct_handle,
                      ptl_ct_event_t    new_ct,
                      ptl_handle_ct_t   trig_ct_handle,
                      ptl_size_t        threshold);
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
#endif
