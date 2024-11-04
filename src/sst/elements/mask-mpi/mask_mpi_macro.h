/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#define MPI_Address(in,out) *out = ((MPI_Aint) in) + 8
#define MPI_Get_address(in,out) MPI_Address(in,out)

#define MPI_Cart_create mask_mpi_cart_create
#define MPI_Cart_get mask_mpi_cart_get
#define MPI_Cart_rank mask_mpi_cart_rank
#define MPI_Cart_shift mask_mpi_cart_shift
#define MPI_Cart_coords mask_mpi_cart_coords
#define MPI_NodeAddress mask_mpi_nodeaddress
#define MPI_Taskid mask_mpi_taskid
#define MPI_Errhandler_set mask_mpi_errhandler_set
#define MPI_Print mask_mpi_print
#define MPI_Init mask_mpi_init
#define MPI_Initialized mask_mpi_initialized
#define MPI_Abort mask_mpi_abort
#define MPI_Finalize mask_mpi_finalize
#define MPI_Wait mask_mpi_wait
#define MPI_Waitall mask_mpi_waitall
#define MPI_Waitany mask_mpi_waitany
#define MPI_Waitsome mask_mpi_waitsome
#define MPI_Get_count mask_mpi_get_count
#define MPI_Test mask_mpi_test
#define MPI_Probe mask_mpi_probe
#define MPI_Cancel mask_mpi_cancel
#define MPI_Request_free mask_mpi_request_free
#define MPI_Comm_dup mask_mpi_comm_dup
#define MPI_Comm_rank mask_mpi_comm_rank
#define MPI_Comm_split mask_mpi_comm_split
#define MPI_Comm_size mask_mpi_comm_size
#define MPI_Comm_free mask_mpi_comm_free
#define MPI_Comm_create mask_mpi_comm_create
#define MPI_Comm_group mask_mpi_comm_group
#define MPI_Group_incl mask_mpi_group_incl
#define MPI_Group_free mask_mpi_group_free
#define MPI_Sendrecv mask_mpi_sendrecv
#define MPI_Send mask_mpi_send
#define MPI_Isend mask_mpi_isend
#define MPI_Recv mask_mpi_recv
#define MPI_Irecv mask_mpi_irecv
#define MPI_Bsend mask_mpi_send
#define MPI_Rsend mask_mpi_send
#define MPI_Ssend mask_mpi_send
#define MPI_Ibsend mask_mpi_isend
#define MPI_Issend mask_mpi_isend
#define MPI_Irsend mask_mpi_isend
#define MPI_Allreduce mask_mpi_allreduce
#define MPI_Reduce mask_mpi_reduce
#define MPI_Barrier mask_mpi_barrier
#define MPI_Bcast mask_mpi_bcast
#define MPI_Scan mask_mpi_scan
#define MPI_Gather mask_mpi_gather
#define MPI_Gatherv mask_mpi_gatherv
#define MPI_Allgather mask_mpi_allgather
#define MPI_Allgatherv mask_mpi_allgatherv
#define MPI_Scatter mask_mpi_scatter
#define MPI_Scatterv mask_mpi_scatterv
#define MPI_Alltoall mask_mpi_alltoall
#define MPI_Iallreduce mask_mpi_iallreduce
#define MPI_Ireduce mask_mpi_ireduce
#define MPI_Ibarrier mask_mpi_ibarrier
#define MPI_Ibcast mask_mpi_ibcast
#define MPI_Iscan mask_mpi_iscan
#define MPI_Igather mask_mpi_igather
#define MPI_Igatherv mask_mpi_igatherv
#define MPI_Iallgather mask_mpi_iallgather
#define MPI_Iallgatherv mask_mpi_iallgatherv
#define MPI_Iscatter mask_mpi_iscatter
#define MPI_Iscatterv mask_mpi_iscatterv
#define MPI_Ialltoall mask_mpi_ialltoall
#define MPI_Ialltoallv mask_mpi_ialltoallv
#define MPI_Ialltoallw mask_mpi_ialltoallw
#define MPI_Wtime mask_mpi_wtime
#define MPI_Disable_Payloads mask_mpi_disable_Payloads
#define MPI_Enable_Payloads mask_mpi_enable_Payloads
#define MPI_Recv_init mask_mpi_recv_init
#define MPI_Startall mask_mpi_startall
#define MPI_Start mask_mpi_start
#define MPI_Testall mask_mpi_testall
#define MPI_Testany mask_mpi_testany
#define MPI_Testsome mask_mpi_testsome
#define MPI_Test_cancelled mask_mpi_test_cancelled
#define MPI_Iprobe mask_mpi_iprobe
#define MPI_Gatherv mask_mpi_gatherv
#define MPI_Alltoallv mask_mpi_alltoallv
#define MPI_Type_contiguous mask_mpi_type_contiguous
#define MPI_Type_commit mask_mpi_type_commit
#define MPI_Type_free mask_mpi_type_free
#define MPI_Wtick mask_mpi_wtick
#define MPI_Type_hvector mask_mpi_type_hvector
#define MPI_Type_vector mask_mpi_type_vector
#define MPI_Type_indexed mask_mpi_type_indexed
#define MPI_Type_extent mask_mpi_type_extent
#define MPI_Type_dup mask_mpi_type_dup
#define MPI_Type_set_name mask_mpi_type_set_name
#define MPI_Type_indexed mask_mpi_type_indexed
#define MPI_Type_size mask_mpi_type_size
#define MPI_Type_get_name mask_mpi_type_get_name
#define MPI_Group_excl mask_mpi_group_excl
#define MPI_Group_range_incl mask_mpi_group_range_incl
#define MPI_Keyval_free mask_mpi_keyval_free
#define MPI_Comm_compare mask_mpi_comm_compare
#define MPI_Type_extent mask_mpi_type_extent
#define MPI_Type_dup mask_mpi_type_dup
#define MPI_Type_set_name mask_mpi_type_set_name
#define MPI_Type_indexed mask_mpi_type_indexed
#define MPI_Type_size mask_mpi_type_size
#define MPI_Type_get_name mask_mpi_type_get_name
#define MPI_Group_excl mask_mpi_group_excl
#define MPI_Testany mask_mpi_testany
#define MPI_Testsome mask_mpi_testsome
#define MPI_Test_cancelled mask_mpi_test_cancelled
#define MPI_Type_indexed mask_mpi_type_indexed
#define MPI_Type_hindexed mask_mpi_type_hindexed
#define MPI_Type_extent mask_mpi_type_extent
#define MPI_Type_size mask_mpi_type_size
#define MPI_Type_lb mask_mpi_type_lb
#define MPI_Type_ub mask_mpi_type_ub
#define MPI_Pack mask_mpi_pack
#define MPI_Unpack mask_mpi_unpack
#define MPI_Pack_size mask_mpi_pack_size
#define MPI_Group_size mask_mpi_group_size
#define MPI_Group_rank mask_mpi_group_rank
#define MPI_Group_range_incl mask_mpi_group_range_incl
#define MPI_Group_compare mask_mpi_group_compare
#define MPI_Group_union mask_mpi_group_union
#define MPI_Group_intersection mask_mpi_group_intersection
#define MPI_Group_difference mask_mpi_group_difference
#define MPI_Group_range_excl mask_mpi_group_range_excl
#define MPI_Keyval_free mask_mpi_keyval_free
#define MPI_Reduce_scatter mask_mpi_reduce_scatter
#define MPI_Finalized mask_mpi_finalized
#define MPI_Type_get_extent mask_mpi_type_get_extent
#define MPI_Type_get_true_extent mask_mpi_type_get_true_extent
#define MPI_Alltoallw mask_mpi_alltoallw
#define MPI_Exscan mask_mpi_exscan
#define MPI_Comm_set_errhandler mask_mpi_comm_set_errhandler
#define MPI_Error_class mask_mpi_error_class
#define MPI_Error_string mask_mpi_error_string
#define MPI_Type_create_struct mask_mpi_type_create_struct
#define MPI_Type_struct mask_mpi_type_create_struct
#define MPI_Buffer_attach mask_mpi_buffer_attach
#define MPI_Buffer_detach mask_mpi_buffer_detach
#define MPI_Init_thread mask_mpi_init_thread
#define MPI_Op_create mask_mpi_op_create
#define MPI_Op_free mask_mpi_op_free
#define MPI_Reduce_scatter_block mask_mpi_reduce_scatter_block
#define MPI_Ireduce_scatter_block mask_mpi_ireduce_scatter_block
#define MPI_Send_init mask_mpi_send_init
#define MPI_Bsend_init mask_mpi_send_init
#define MPI_Rsend_init mask_mpi_send_init
#define MPI_Ssend_init mask_mpi_send_init
#define MPI_Group_translate_ranks mask_mpi_group_translate_ranks
#define MPI_Errhandler_create mask_mpi_errhandler_create
#define MPI_Comm_get_attr mask_mpi_comm_get_attr

#define MPI_Comm_create_group mask_mpi_comm_create_group
#define MPI_Win_flush mask_mpi_win_flush
#define MPI_Win_flush_local mask_mpi_win_flush_local
#define MPI_Win_lock mask_mpi_win_lock
#define MPI_Win_unlock mask_mpi_win_unlock
#define MPI_Win_create mask_mpi_win_create
#define MPI_Win_free mask_mpi_win_free

#define MPI_Get mask_mpi_mpi_get
#define MPI_Put mask_mpi_mpi_put

#define MPI_Intercomm_create error not yet implemented
#define MPI_Comm_remote_size error not yet implemented
#define MPI_Comm_test_inter error not yet implemented
#define MPI_Comm_remote_group error not yet implemented
#define MPI_Intercomm_merge error not yet implemented
#define MPI_Attr_delete error not yet implemented
#define MPI_Dims_create error not yet implemented
#define MPI_Graph_create error not yet implemented
#define MPI_Graphdims_get error not yet implemented
#define MPI_Graph_get error not yet implemented
#define MPI_Cart_sub error not yet implemented
#define MPI_Cart_map error not yet implemented
#define MPI_Graph_map error not yet implemented
#define MPI_Get_processor_name error not yet implemented
#define MPI_Get_version error not yet implemented
#define MPI_Errhandler_get error not yet implemented
#define MPI_Errhandler_free error not yet implemented
#define MPI_Pcontrol error not yet implemented
#define MPI_Close_port error not yet implemented
#define MPI_Comm_accept error not yet implemented
#define MPI_Comm_connect error not yet implemented
#define MPI_Comm_disconnect error not yet implemented
#define MPI_Comm_get_parent error not yet implemented
#define MPI_Comm_join error not yet implemented
#define MPI_Comm_spawn error not yet implemented
#define MPI_Comm_spawn_multiple error not yet implemented
#define MPI_Lookup_name error not yet implemented
#define MPI_Open_port error not yet implemented
#define MPI_Publish_name error not yet implemented
#define MPI_Unpublish_name error not yet implemented
#define MPI_Accumulate error not yet implemented
#define MPI_Win_complete error not yet implemented
#define MPI_Win_fence error not yet implemented
#define MPI_Win_get_group error not yet implemented
#define MPI_Win_post error not yet implemented
#define MPI_Win_start error not yet implemented
#define MPI_Win_test error not yet implemented
#define MPI_Win_wait error not yet implemented
#define MPI_Add_error_class error not yet implemented
#define MPI_Add_error_code error not yet implemented
#define MPI_Add_error_string error not yet implemented
#define MPI_Comm_call_errhandler error not yet implemented
#define MPI_Comm_create_keyval error not yet implemented
#define MPI_Comm_delete_attr error not yet implemented
#define MPI_Comm_free_keyval error not yet implemented
#define MPI_Comm_get_name error not yet implemented
#define MPI_Comm_set_attr error not yet implemented
#define MPI_Comm_set_name error not yet implemented
#define MPI_File_call_errhandler error not yet implemented
#define MPI_Grequest_complete error not yet implemented
#define MPI_Grequest_start error not yet implemented
#define MPI_Is_thread_main error not yet implemented
#define MPI_Query_thread error not yet implemented
#define MPI_Status_set_cancelled error not yet implemented
#define MPI_Status_set_elements error not yet implemented
#define MPI_Type_create_keyval error not yet implemented
#define MPI_Type_delete_attr error not yet implemented
#define MPI_Type_free_keyval error not yet implemented
#define MPI_Type_get_attr error not yet implemented
#define MPI_Type_get_contents error not yet implemented
#define MPI_Type_get_envelope error not yet implemented
#define MPI_Type_set_attr error not yet implemented
#define MPI_Type_match_size error not yet implemented
#define MPI_Win_call_errhandler error not yet implemented
#define MPI_Win_create_keyval error not yet implemented
#define MPI_Win_delete_attr error not yet implemented
#define MPI_Win_free_keyval error not yet implemented
#define MPI_Win_get_attr error not yet implemented
#define MPI_Win_get_name error not yet implemented
#define MPI_Win_set_attr error not yet implemented
#define MPI_Win_set_name error not yet implemented
#define MPI_File_c2f error not yet implemented
#define MPI_Alloc_mem error not yet implemented
#define MPI_Comm_create_errhandler error not yet implemented
#define MPI_Comm_get_errhandler error not yet implemented

#define MPI_File_create_errhandler error not yet implemented
#define MPI_File_get_errhandler error not yet implemented
#define MPI_File_set_errhandler error not yet implemented
#define MPI_Free_mem error not yet implemented
#define MPI_Info_create error not yet implemented
#define MPI_Info_delete error not yet implemented
#define MPI_Info_dup error not yet implemented
#define MPI_Info_free error not yet implemented
#define MPI_Info_get error not yet implemented
#define MPI_Info_get_nkeys error not yet implemented
#define MPI_Info_get_nthkey error not yet implemented
#define MPI_Info_get_valuelen error not yet implemented
#define MPI_Info_set error not yet implemented
#define MPI_Pack_external error not yet implemented
#define MPI_Pack_external_size error not yet implemented
#define MPI_Request_get_status error not yet implemented
#define MPI_Type_create_darray error not yet implemented
#define MPI_Type_create_hindexed error not yet implemented
#define MPI_Type_create_hvector error not yet implemented
#define MPI_Type_create_indexed_block error not yet implemented
#define MPI_Type_create_resized error not yet implemented
#define MPI_Type_create_subarray error not yet implemented
#define MPI_Unpack_external error not yet implemented
#define MPI_Win_create_errhandler error not yet implemented
#define MPI_Win_get_errhandler error not yet implemented
#define MPI_Win_set_errhandler error not yet implemented
#define MPI_Dist_graph_create_adjacent error not yet implemented
#define MPI_Dist_graph_create          error not yet implemented
#define MPI_Dist_graph_neighbors_count error not yet implemented
#define MPI_Dist_graph_neighbors       error not yet implemented
#define MPI_Intercomm_create error not yet implemented
#define MPI_Sendrecv_replace error not yet implemented
#define MPI_Get_elements error not yet implemented
