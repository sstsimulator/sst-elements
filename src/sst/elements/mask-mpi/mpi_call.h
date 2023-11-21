/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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

#pragma once

#include <mpi_types.h>
#include <mpi_integers.h>
#include <mpi_api_fwd.h>
#include <mercury/common/timestamp.h>

namespace SST::MASKMPI {

typedef enum {
  Call_ID_MPI_Send=0,                    Call_ID_MPI_Recv,
  Call_ID_MPI_Get_count,                 Call_ID_MPI_Bsend,
  Call_ID_MPI_Ssend,                     Call_ID_MPI_Rsend,
  Call_ID_MPI_Buffer_attach,             Call_ID_MPI_Buffer_detach,
  Call_ID_MPI_Isend,                     Call_ID_MPI_Ibsend,
  Call_ID_MPI_Issend,                    Call_ID_MPI_Irsend,
  Call_ID_MPI_Irecv,                     Call_ID_MPI_Wait,
  Call_ID_MPI_Test,                      Call_ID_MPI_Request_free,
  Call_ID_MPI_Waitany,                   Call_ID_MPI_Testany,
  Call_ID_MPI_Waitall,                   Call_ID_MPI_Testall,
  Call_ID_MPI_Waitsome,                  Call_ID_MPI_Testsome,
  Call_ID_MPI_Iprobe,                    Call_ID_MPI_Probe,
  Call_ID_MPI_Cancel,                    Call_ID_MPI_Test_cancelled,
  Call_ID_MPI_Send_init,                 Call_ID_MPI_Bsend_init,
  Call_ID_MPI_Ssend_init,                Call_ID_MPI_Rsend_init,
  Call_ID_MPI_Recv_init,                 Call_ID_MPI_Start,
  Call_ID_MPI_Startall,                  Call_ID_MPI_Sendrecv,
  Call_ID_MPI_Sendrecv_replace,          Call_ID_MPI_Type_contiguous,
  Call_ID_MPI_Type_vector,               Call_ID_MPI_Type_hvector,
  Call_ID_MPI_Type_indexed,              Call_ID_MPI_Type_hindexed,
  Call_ID_MPI_Type_struct,               Call_ID_MPI_Address,
  Call_ID_MPI_Type_extent,               Call_ID_MPI_Type_size,
  Call_ID_MPI_Type_lb,                   Call_ID_MPI_Type_ub,
  Call_ID_MPI_Type_commit,               Call_ID_MPI_Type_free,
  Call_ID_MPI_Get_elements,              Call_ID_MPI_Pack,
  Call_ID_MPI_Unpack,                    Call_ID_MPI_Pack_size,
  Call_ID_MPI_Barrier,                   Call_ID_MPI_Bcast,
  Call_ID_MPI_Gather,                    Call_ID_MPI_Gatherv,
  Call_ID_MPI_Scatter,                   Call_ID_MPI_Scatterv,
  Call_ID_MPI_Allgather,                 Call_ID_MPI_Allgatherv,
  Call_ID_MPI_Alltoall,                  Call_ID_MPI_Alltoallv,
  Call_ID_MPI_Reduce,                    Call_ID_MPI_Op_create,
  Call_ID_MPI_Op_free,                   Call_ID_MPI_Allreduce,
  Call_ID_MPI_Reduce_scatter,            Call_ID_MPI_Scan,
  Call_ID_MPI_Ibarrier,                   Call_ID_MPI_Ibcast,
  Call_ID_MPI_Igather,                    Call_ID_MPI_Igatherv,
  Call_ID_MPI_Iscatter,                   Call_ID_MPI_Iscatterv,
  Call_ID_MPI_Iallgather,                 Call_ID_MPI_Iallgatherv,
  Call_ID_MPI_Ialltoall,                  Call_ID_MPI_Ialltoallv,
  Call_ID_MPI_Ireduce,                    Call_ID_MPI_Iallreduce,
  Call_ID_MPI_Ireduce_scatter,            Call_ID_MPI_Iscan,
  Call_ID_MPI_Reduce_scatter_block,
  Call_ID_MPI_Ireduce_scatter_block,
  Call_ID_MPI_Group_size,                Call_ID_MPI_Group_rank,
  Call_ID_MPI_Group_translate_ranks,     Call_ID_MPI_Group_compare,
  Call_ID_MPI_Comm_group,                Call_ID_MPI_Group_union,
  Call_ID_MPI_Group_intersection,        Call_ID_MPI_Group_difference,
  Call_ID_MPI_Group_incl,                Call_ID_MPI_Group_excl,
  Call_ID_MPI_Group_range_incl,          Call_ID_MPI_Group_range_excl,
  Call_ID_MPI_Group_free,                Call_ID_MPI_Comm_size,
  Call_ID_MPI_Comm_rank,                 Call_ID_MPI_Comm_compare,
  Call_ID_MPI_Comm_dup,                  Call_ID_MPI_Comm_create,
  Call_ID_MPI_Comm_create_group,
  Call_ID_MPI_Comm_split,                Call_ID_MPI_Comm_free,
  Call_ID_MPI_Comm_test_inter,           Call_ID_MPI_Comm_remote_size,
  Call_ID_MPI_Comm_remote_group,         Call_ID_MPI_Intercomm_create,
  Call_ID_MPI_Intercomm_merge,           Call_ID_MPI_Keyval_create,
  Call_ID_MPI_Keyval_free,               Call_ID_MPI_Attr_put,
  Call_ID_MPI_Attr_get,                  Call_ID_MPI_Attr_delete,
  Call_ID_MPI_Topo_test,                 Call_ID_MPI_Cart_create,
  Call_ID_MPI_Dims_create,               Call_ID_MPI_Graph_create,
  Call_ID_MPI_Graphdims_get,             Call_ID_MPI_Graph_get,
  Call_ID_MPI_Cartdim_get,               Call_ID_MPI_Cart_get,
  Call_ID_MPI_Cart_rank,                 Call_ID_MPI_Cart_coords,
  Call_ID_MPI_Graph_neighbors_count,     Call_ID_MPI_Graph_neighbors,
  Call_ID_MPI_Cart_shift,                Call_ID_MPI_Cart_sub,
  Call_ID_MPI_Cart_map,                  Call_ID_MPI_Graph_map,
  Call_ID_MPI_Get_processor_name,        Call_ID_MPI_Get_version,
  Call_ID_MPI_Errhandler_create,         Call_ID_MPI_Errhandler_set,
  Call_ID_MPI_Errhandler_get,            Call_ID_MPI_Errhandler_free,
  Call_ID_MPI_Error_string,              Call_ID_MPI_Error_class,
  Call_ID_MPI_Wtime,                     Call_ID_MPI_Wtick,
  Call_ID_MPI_Init,                      Call_ID_MPI_Finalize,
  Call_ID_MPI_Initialized,               Call_ID_MPI_Abort,
  Call_ID_MPI_Pcontrol,                  Call_ID_MPI_Close_port,
  Call_ID_MPI_Comm_accept,               Call_ID_MPI_Comm_connect,
  Call_ID_MPI_Comm_disconnect,           Call_ID_MPI_Comm_get_parent,
  Call_ID_MPI_Comm_join,                 Call_ID_MPI_Comm_spawn,
  Call_ID_MPI_Comm_spawn_multiple,       Call_ID_MPI_Lookup_name,
  Call_ID_MPI_Open_port,                 Call_ID_MPI_Publish_name,
  Call_ID_MPI_Unpublish_name,            Call_ID_MPI_Accumulate,
  Call_ID_MPI_Get,                       Call_ID_MPI_Put,
  Call_ID_MPI_Win_complete,              Call_ID_MPI_Win_create,
  Call_ID_MPI_Win_fence,                 Call_ID_MPI_Win_free,
  Call_ID_MPI_Win_get_group,             Call_ID_MPI_Win_lock,
  Call_ID_MPI_Win_post,                  Call_ID_MPI_Win_start,
  Call_ID_MPI_Win_test,                  Call_ID_MPI_Win_unlock,
  Call_ID_MPI_Win_wait,                  Call_ID_MPI_Alltoallw,
  Call_ID_MPI_Exscan,                    Call_ID_MPI_Add_error_class,
  Call_ID_MPI_Add_error_code,            Call_ID_MPI_Add_error_string,
  Call_ID_MPI_Comm_call_errhandler,      Call_ID_MPI_Comm_create_keyval,
  Call_ID_MPI_Comm_delete_attr,          Call_ID_MPI_Comm_free_keyval,
  Call_ID_MPI_Comm_get_attr,             Call_ID_MPI_Comm_get_name,
  Call_ID_MPI_Comm_set_attr,             Call_ID_MPI_Comm_set_name,
  Call_ID_MPI_File_call_errhandler,      Call_ID_MPI_Grequest_complete,
  Call_ID_MPI_Grequest_start,            Call_ID_MPI_Init_thread,
  Call_ID_MPI_Is_thread_main,            Call_ID_MPI_Query_thread,
  Call_ID_MPI_Status_set_cancelled,      Call_ID_MPI_Status_set_elements,
  Call_ID_MPI_Type_create_keyval,        Call_ID_MPI_Type_delete_attr,
  Call_ID_MPI_Type_dup,                  Call_ID_MPI_Type_free_keyval,
  Call_ID_MPI_Type_get_attr,             Call_ID_MPI_Type_get_contents,
  Call_ID_MPI_Type_get_envelope,         Call_ID_MPI_Type_get_name,
  Call_ID_MPI_Type_set_attr,             Call_ID_MPI_Type_set_name,
  Call_ID_MPI_Type_match_size,           Call_ID_MPI_Win_call_errhandler,
  Call_ID_MPI_Win_create_keyval,         Call_ID_MPI_Win_delete_attr,
  Call_ID_MPI_Win_free_keyval,           Call_ID_MPI_Win_get_attr,
  Call_ID_MPI_Win_get_name,              Call_ID_MPI_Win_set_attr,
  Call_ID_MPI_Win_set_name,              Call_ID_MPI_Alloc_mem,
  Call_ID_MPI_Comm_create_errhandler,    Call_ID_MPI_Comm_get_errhandler,
  Call_ID_MPI_Comm_set_errhandler,       Call_ID_MPI_File_create_errhandler,
  Call_ID_MPI_File_get_errhandler,       Call_ID_MPI_File_set_errhandler,
  Call_ID_MPI_Finalized,                 Call_ID_MPI_Free_mem,
  Call_ID_MPI_Get_address,               Call_ID_MPI_Info_create,
  Call_ID_MPI_Info_delete,               Call_ID_MPI_Info_dup,
  Call_ID_MPI_Info_free,                 Call_ID_MPI_Info_get,
  Call_ID_MPI_Info_get_nkeys,            Call_ID_MPI_Info_get_nthkey,
  Call_ID_MPI_Info_get_valuelen,         Call_ID_MPI_Info_set,
  Call_ID_MPI_Pack_external,             Call_ID_MPI_Pack_external_size,
  Call_ID_MPI_Request_get_status,        Call_ID_MPI_Type_create_darray,
  Call_ID_MPI_Type_create_hindexed,      Call_ID_MPI_Type_create_hvector,
  Call_ID_MPI_Type_create_indexed_block, Call_ID_MPI_Type_create_resized,
  Call_ID_MPI_Type_create_struct,        Call_ID_MPI_Type_create_subarray,
  Call_ID_MPI_Type_get_extent,           Call_ID_MPI_Type_get_true_extent,
  Call_ID_MPI_Unpack_external,           Call_ID_MPI_Win_create_errhandler,
  Call_ID_MPI_Win_get_errhandler,        Call_ID_MPI_Win_set_errhandler
} MPI_function;

struct MPI_Call {
  MPI_function ID;
  SST::Hg::TimeDelta idle;
  SST::Hg::Timestamp start;

  const char* ID_str() const {
    return ID_str(ID);
  }

  static const char* ID_str(MPI_function func);
};

}
