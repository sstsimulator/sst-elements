/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

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

#include <mask_mpi_macro.h>
#include <mpi_integers.h>
#include <mpi_status.h>
#include <mpi_types.h>
#include <stddef.h>

#define MPI_LOCK_SHARED 0
#define MPI_LOCK_EXCLUSIVE 1

#ifdef __cplusplus
extern "C" {
#endif

int mask_mpi_send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm);
int mask_mpi_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status);
int mask_mpi_get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);
int mask_mpi_bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm);
int mask_mpi_ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm);
int mask_mpi_rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm);
int mask_mpi_buffer_attach(void *buffer, int size);
int mask_mpi_buffer_detach(void *buffer_addr, int *size);
int mask_mpi_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request);
int mask_mpi_ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request);
int mask_mpi_issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request);
int mask_mpi_irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
               MPI_Comm comm, MPI_Request *request);
int mask_mpi_irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request);
int mask_mpi_wait(MPI_Request *request, MPI_Status *status);
int mask_mpi_test(MPI_Request *request, int *flag, MPI_Status *status);
int mask_mpi_request_free(MPI_Request *request);
int mask_mpi_waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status);
int mask_mpi_testany(int count, MPI_Request array_of_requests[], int *indx, int *flag,
                MPI_Status *status);
int mask_mpi_waitall(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]);
int mask_mpi_testall(int count, MPI_Request array_of_requests[], int *flag,
                MPI_Status array_of_statuses[]);
int mask_mpi_waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[]);
int mask_mpi_testsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[]);
int mask_mpi_iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int mask_mpi_probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
int mask_mpi_cancel(MPI_Request *request);
int mask_mpi_test_cancelled(const MPI_Status *status, int *flag);
int mask_mpi_send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                  MPI_Comm comm, MPI_Request *request);
int mask_mpi_bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request);
int mask_mpi_ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request);
int mask_mpi_rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                   MPI_Comm comm, MPI_Request *request);
int mask_mpi_recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                  MPI_Comm comm, MPI_Request *request);
int mask_mpi_start(MPI_Request *request);
int mask_mpi_startall(int count, MPI_Request array_of_requests[]);
int mask_mpi_sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int mask_mpi_sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest,
                         int sendtag, int source, int recvtag, MPI_Comm comm,
                         MPI_Status *status);
int mask_mpi_type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int mask_mpi_type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                    MPI_Datatype *newtype);
int mask_mpi_type_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                     MPI_Datatype *newtype);
int mask_mpi_type_indexed(int count, const int *array_of_blocklengths,
                     const int *array_of_displacements, MPI_Datatype oldtype,
                     MPI_Datatype *newtype);
int mask_mpi_type_hindexed(int count, const int *array_of_blocklengths,
                      const MPI_Aint *array_of_displacements, MPI_Datatype oldtype,
                      MPI_Datatype *newtype);
int mask_mpi_type_struct(int count, const int *array_of_blocklengths,
                    const MPI_Aint *array_of_displacements,
                    const MPI_Datatype *array_of_types, MPI_Datatype *newtype);
int mask_mpi_address(const void *location, MPI_Aint *address);
int mask_mpi_type_extent(MPI_Datatype datatype, MPI_Aint *extent);
int mask_mpi_type_size(MPI_Datatype datatype, int *size);
int mask_mpi_type_lb(MPI_Datatype datatype, MPI_Aint *displacement);
int mask_mpi_type_ub(MPI_Datatype datatype, MPI_Aint *displacement);
int mask_mpi_type_commit(MPI_Datatype *datatype);
int mask_mpi_type_free(MPI_Datatype *datatype);
int mask_mpi_get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count);
int mask_mpi_pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf,
             int outsize, int *position, MPI_Comm comm);
int mask_mpi_unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount,
               MPI_Datatype datatype, MPI_Comm comm);
int mask_mpi_pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);
int mask_mpi_barrier(MPI_Comm comm);
int mask_mpi_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int mask_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
               int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int mask_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                MPI_Comm comm);
int mask_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int mask_mpi_scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
                 MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm);
int mask_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_alltoallv(const void *sendbuf, const int *sendcounts, const int *sdispls,
                  MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                  const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                  const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int mask_mpi_exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, MPI_Comm comm);
int mask_mpi_reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm);
int mask_mpi_op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);
int mask_mpi_op_free(MPI_Op *op);
int mask_mpi_allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm);
int mask_mpi_reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int mask_mpi_scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
             MPI_Comm comm);
int mask_mpi_group_size(MPI_Group group, int *size);
int mask_mpi_group_rank(MPI_Group group, int *rank);
int mask_mpi_group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2,
                              int ranks2[]);
int mask_mpi_group_compare(MPI_Group group1, MPI_Group group2, int *result);
int mask_mpi_comm_group(MPI_Comm comm, MPI_Group *group);
int mask_mpi_group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int mask_mpi_group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int mask_mpi_group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int mask_mpi_group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int mask_mpi_group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int mask_mpi_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int mask_mpi_group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int mask_mpi_group_free(MPI_Group *group);
int mask_mpi_comm_size(MPI_Comm comm, int *size);
int mask_mpi_comm_rank(MPI_Comm comm, int *rank);
int mask_mpi_comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
int mask_mpi_comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int mask_mpi_comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
int mask_mpi_comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int mask_mpi_comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int mask_mpi_comm_free(MPI_Comm *comm);
int mask_mpi_comm_test_inter(MPI_Comm comm, int *flag);
int mask_mpi_comm_remote_size(MPI_Comm comm, int *size);
int mask_mpi_comm_remote_group(MPI_Comm comm, MPI_Group *group);
int mask_mpi_intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm,
                         int remote_leader, int tag, MPI_Comm *newintercomm);
int mask_mpi_intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int mask_mpi_keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn,
                      int *keyval, void *extra_state);
int mask_mpi_keyval_free(int *keyval);
int mask_mpi_attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int mask_mpi_attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int mask_mpi_attr_delete(MPI_Comm comm, int keyval);
int mask_mpi_topo_test(MPI_Comm comm, int *status);
int mask_mpi_cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[],
                    int reorder, MPI_Comm *comm_cart);
int mask_mpi_dims_create(int nnodes, int ndims, int dims[]);
int mask_mpi_graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[],
                     int reorder, MPI_Comm *comm_graph);
int mask_mpi_graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
int mask_mpi_graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]);
int mask_mpi_cartdim_get(MPI_Comm comm, int *ndims);
int mask_mpi_cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);
int mask_mpi_cart_rank(MPI_Comm comm, const int coords[], int *rank);
int mask_mpi_cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);
int mask_mpi_graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
int mask_mpi_graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]);
int mask_mpi_cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);
int mask_mpi_cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);
int mask_mpi_cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank);
int mask_mpi_graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank);
int mask_mpi_get_processor_name(char *name, int *resultlen);
int mask_mpi_get_version(int *version, int *subversion);
int mask_mpi_get_library_version(char *version, int *resultlen);
int mask_mpi_errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler);
int mask_mpi_errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
int mask_mpi_errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
int mask_mpi_errhandler_free(MPI_Errhandler *errhandler);
int mask_mpi_error_string(int errorcode, char *string, int *resultlen);
int mask_mpi_error_class(int errorcode, int *errorclass);
double mask_mpi_wtime(void);
double mask_mpi_wtick(void);
int mask_mpi_init(int *argc, char ***argv);
int mask_mpi_finalize(void);
int mask_mpi_initialized(int *flag);
int mask_mpi_abort(MPI_Comm comm, int errorcode);

/* Note that we may need to define a @PCONTROL_LIST@ depending on whether
   stdargs are supported */
int mask_mpi_pcontrol(const int level, ...);
int mask_mpi_dUP_FN(MPI_Comm oldcomm, int keyval, void *extra_state, void *attribute_val_in,
               void *attribute_val_out, int *flag);

/* Process Creation and Management */
int mask_mpi_close_port(const char *port_name);
int mask_mpi_comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                    MPI_Comm *newcomm);
int mask_mpi_comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm,
                     MPI_Comm *newcomm);
int mask_mpi_comm_disconnect(MPI_Comm *comm);
int mask_mpi_comm_get_parent(MPI_Comm *parent);
int mask_mpi_comm_join(int fd, MPI_Comm *intercomm);
int mask_mpi_comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root,
                   MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int mask_mpi_comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[],
                            const int array_of_maxprocs[], const MPI_Info array_of_info[],
                            int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int mask_mpi_lookup_name(const char *service_name, MPI_Info info, char *port_name);
int mask_mpi_open_port(MPI_Info info, char *port_name);
int mask_mpi_publish_name(const char *service_name, MPI_Info info, const char *port_name);
int mask_mpi_unpublish_name(const char *service_name, MPI_Info info, const char *port_name);
int mask_mpi_comm_set_info(MPI_Comm comm, MPI_Info info);
int mask_mpi_comm_get_info(MPI_Comm comm, MPI_Info *info);

/* One-Sided Communications */
int mask_mpi_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                   int target_rank, MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int mask_mpi_get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win);
int mask_mpi_put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
            int target_rank, MPI_Aint target_disp, int target_count,
            MPI_Datatype target_datatype, MPI_Win win);
int mask_mpi_win_complete(MPI_Win win);
int mask_mpi_win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                   MPI_Win *win);
int mask_mpi_win_fence(int assert, MPI_Win win);
int mask_mpi_win_free(MPI_Win *win);
int mask_mpi_win_get_group(MPI_Win win, MPI_Group *group);
int mask_mpi_win_lock(int lock_type, int rank, int assert, MPI_Win win);
int mask_mpi_win_post(MPI_Group group, int assert, MPI_Win win);
int mask_mpi_win_start(MPI_Group group, int assert, MPI_Win win);
int mask_mpi_win_test(MPI_Win win, int *flag);
int mask_mpi_win_unlock(int rank, MPI_Win win);
int mask_mpi_win_wait(MPI_Win win);

/* MPI-3 One-Sided Communication Routines */
int mask_mpi_win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr,
                     MPI_Win *win);
int mask_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm,
                            void *baseptr, MPI_Win *win);
int mask_mpi_win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);
int mask_mpi_win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);
int mask_mpi_win_attach(MPI_Win win, void *base, MPI_Aint size);
int mask_mpi_win_detach(MPI_Win win, const void *base);
int mask_mpi_win_get_info(MPI_Win win, MPI_Info *info_used);
int mask_mpi_win_set_info(MPI_Win win, MPI_Info info);
int mask_mpi_get_accumulate(const void *origin_addr, int origin_count,
                        MPI_Datatype origin_datatype, void *result_addr, int result_count,
                        MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                        int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int mask_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                      MPI_Datatype datatype, int target_rank, MPI_Aint target_disp,
                      MPI_Op op, MPI_Win win);
int mask_mpi_compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPI_Win win);
int mask_mpi_rput(const void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win,
              MPI_Request *request);
int mask_mpi_rget(void *origin_addr, int origin_count,
              MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPI_Win win,
              MPI_Request *request);
int mask_mpi_raccumulate(const void *origin_addr, int origin_count,
                     MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                     MPI_Request *request);
int mask_mpi_rget_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win,
                         MPI_Request *request);
int mask_mpi_win_lock_all(int assert, MPI_Win win);
int mask_mpi_win_unlock_all(MPI_Win win);
int mask_mpi_win_flush(int rank, MPI_Win win);
int mask_mpi_win_flush_all(MPI_Win win);
int mask_mpi_win_flush_local(int rank, MPI_Win win);
int mask_mpi_win_flush_local_all(MPI_Win win);
int mask_mpi_win_sync(MPI_Win win);

/* External Interfaces */
int mask_mpi_add_error_class(int *errorclass);
int mask_mpi_add_error_code(int errorclass, int *errorcode);
int mask_mpi_add_error_string(int errorcode, const char *string);
int mask_mpi_comm_call_errhandler(MPI_Comm comm, int errorcode);
int mask_mpi_comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn,
                           MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval,
                           void *extra_state);
int mask_mpi_comm_delete_attr(MPI_Comm comm, int comm_keyval);
int mask_mpi_comm_free_keyval(int *comm_keyval);
int mask_mpi_comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int mask_mpi_comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
int mask_mpi_comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
int mask_mpi_comm_set_name(MPI_Comm comm, const char *comm_name);
int mask_mpi_file_call_errhandler(MPI_File fh, int errorcode);
int mask_mpi_grequest_complete(MPI_Request request);
int mask_mpi_grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn,
                       MPI_Grequest_cancel_function *cancel_fn, void *extra_state,
                       MPI_Request *request);
int mask_mpi_init_thread(int *argc, char ***argv, int required, int *provided);
int mask_mpi_is_thread_main(int *flag);
int mask_mpi_query_thread(int *provided);
int mask_mpi_status_set_cancelled(MPI_Status *status, int flag);
int mask_mpi_status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count);
int mask_mpi_type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn,
                           MPI_Type_delete_attr_function *type_delete_attr_fn,
                           int *type_keyval, void *extra_state);
int mask_mpi_type_delete_attr(MPI_Datatype datatype, int type_keyval);
int mask_mpi_type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);
int mask_mpi_type_free_keyval(int *type_keyval);
int mask_mpi_type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag);
int mask_mpi_type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses,
                          int max_datatypes, int array_of_integers[],
                          MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]);
int mask_mpi_type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses,
                          int *num_datatypes, int *combiner);
int mask_mpi_type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen);
int mask_mpi_type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int mask_mpi_type_set_name(MPI_Datatype datatype, const char *type_name);
int mask_mpi_type_match_size(int typeclass, int size, MPI_Datatype *datatype);
int mask_mpi_win_call_errhandler(MPI_Win win, int errorcode);
int mask_mpi_win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                          MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval,
                          void *extra_state);
int mask_mpi_win_delete_attr(MPI_Win win, int win_keyval);
int mask_mpi_win_free_keyval(int *win_keyval);
int mask_mpi_win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int mask_mpi_win_get_name(MPI_Win win, char *win_name, int *resultlen);
int mask_mpi_win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
int mask_mpi_win_set_name(MPI_Win win, const char *win_name);

int mask_mpi_alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);
int mask_mpi_comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn,
                               MPI_Errhandler *errhandler);
int mask_mpi_comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler);
int mask_mpi_comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
int mask_mpi_file_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn,
                               MPI_Errhandler *errhandler);
int mask_mpi_file_get_errhandler(MPI_File file, MPI_Errhandler *errhandler);
int mask_mpi_file_set_errhandler(MPI_File file, MPI_Errhandler errhandler);
int mask_mpi_finalized(int *flag);
int mask_mpi_free_mem(void *base);
int mask_mpi_get_address(const void *location, MPI_Aint *address);
int mask_mpi_info_create(MPI_Info *info);
int mask_mpi_info_delete(MPI_Info info, const char *key);
int mask_mpi_info_dup(MPI_Info info, MPI_Info *newinfo);
int mask_mpi_info_free(MPI_Info *info);
int mask_mpi_info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag);
int mask_mpi_info_get_nkeys(MPI_Info info, int *nkeys);
int mask_mpi_info_get_nthkey(MPI_Info info, int n, char *key);
int mask_mpi_info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag);
int mask_mpi_info_set(MPI_Info info, const char *key, const char *value);
int mask_mpi_pack_external(const char datarep[], const void *inbuf, int incount,
                      MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);
int mask_mpi_pack_external_size(const char datarep[], int incount, MPI_Datatype datatype,
                           MPI_Aint *size);
int mask_mpi_request_get_status(MPI_Request request, int *flag, MPI_Status *status);
int mask_mpi_status_c2f(const MPI_Status *c_status, MPI_Fint *f_status);
int mask_mpi_status_f2c(const MPI_Fint *f_status, MPI_Status *c_status);
int mask_mpi_type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[],
                           const int array_of_distribs[], const int array_of_dargs[],
                           const int array_of_psizes[], int order, MPI_Datatype oldtype,
                           MPI_Datatype *newtype);
int mask_mpi_type_create_hindexed(int count, const int array_of_blocklengths[],
                             const MPI_Aint array_of_displacements[], MPI_Datatype oldtype,
                             MPI_Datatype *newtype);
int mask_mpi_type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                            MPI_Datatype *newtype);
int mask_mpi_type_create_indexed_block(int count, int blocklength, const int array_of_displacements[],
                                  MPI_Datatype oldtype, MPI_Datatype *newtype);
int mask_mpi_type_create_hindexed_block(int count, int blocklength,
                                   const MPI_Aint array_of_displacements[],
                                   MPI_Datatype oldtype, MPI_Datatype *newtype);
int mask_mpi_type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                            MPI_Datatype *newtype);
int mask_mpi_type_create_struct(int count, const int array_of_blocklengths[],
                           const MPI_Aint array_of_displacements[],
                           const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int mask_mpi_type_create_subarray(int ndims, const int array_of_sizes[],
                             const int array_of_subsizes[], const int array_of_starts[],
                             int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int mask_mpi_type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
int mask_mpi_type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);
int mask_mpi_unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize,
                        MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);
int mask_mpi_win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn,
                              MPI_Errhandler *errhandler);
int mask_mpi_win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
int mask_mpi_win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);

/* Fortran 90-related functions.  These routines are available only if
   Fortran 90 support is enabled
*/
int mask_mpi_type_create_f90_integer(int range, MPI_Datatype *newtype);
int mask_mpi_type_create_f90_real(int precision, int range, MPI_Datatype *newtype);
int mask_mpi_type_create_f90_complex(int precision, int range, MPI_Datatype *newtype);

int mask_mpi_reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                     MPI_Op op);
int mask_mpi_op_commutative(MPI_Op op, int *commute);
int mask_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                             MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int mask_mpi_dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[],
                                   const int sourceweights[], int outdegree,
                                   const int destinations[], const int destweights[],
                                   MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int mask_mpi_dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[],
                          const int destinations[], const int weights[], MPI_Info info,
                          int reorder, MPI_Comm *comm_dist_graph);
int mask_mpi_dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted);
int mask_mpi_dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[],
                             int maxoutdegree, int destinations[], int destweights[]);

/* Matched probe functionality */
int mask_mpi_improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message,
                MPI_Status *status);
int mask_mpi_imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
               MPI_Request *request);
int mask_mpi_mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);
int mask_mpi_mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message,
              MPI_Status *status);

///* Nonblocking collectives */
int mask_mpi_comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);
int mask_mpi_ibarrier(MPI_Comm comm, MPI_Request *request);
int mask_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request *request);
int mask_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                MPI_Request *request);
int mask_mpi_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request);
int mask_mpi_iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
                 MPI_Request *request);
int mask_mpi_iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request *request);
int mask_mpi_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int mask_mpi_iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request);
int mask_mpi_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int mask_mpi_ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                   MPI_Request *request);
int mask_mpi_ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request *request);
int mask_mpi_ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int mask_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm, MPI_Request *request);
int mask_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int mask_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                              MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                              MPI_Request *request);
int mask_mpi_iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
              MPI_Comm comm, MPI_Request *request);
int mask_mpi_iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm, MPI_Request *request);

///* Neighborhood collectives */
int mask_mpi_ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                            MPI_Comm comm, MPI_Request *request);
int mask_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int mask_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
                           MPI_Request *request);
int mask_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request);
int mask_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                            void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int mask_mpi_neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                            void *recvbuf, const int recvcounts[], const int displs[],
                            MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                           MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                           const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int mask_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                           const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                           const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);

int mask_mpi_win_flush(int rank, MPI_Win win);

int mask_mpi_comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm);

#ifdef __cplusplus
}
#endif
