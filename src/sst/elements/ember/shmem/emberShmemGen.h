// Copyright 2009-2021 NTESS. Under the terms

// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_EMBER_SHMEM_GENERATOR
#define _H_EMBER_SHMEM_GENERATOR

#include "embergen.h"
#include "libs/emberShmemLib.h"
#include "libs/misc.h"

#define enQ_init shmem().init
#define enQ_n_pes shmem().n_pes
#define enQ_my_pe shmem().my_pe
#define enQ_put shmem().put
#define enQ_putv shmem().putv
#define enQ_get shmem().get
#define enQ_getv shmem().getv
#define enQ_put_nbi shmem().put_nbi
#define enQ_get_nbi shmem().get_nbi
#define enQ_add shmem().add
#define enQ_fadd shmem().fadd
#define enQ_swap shmem().swap
#define enQ_cswap shmem().cswap
#define enQ_quiet shmem().quiet
#define enQ_wait shmem().wait
#define enQ_wait_until shmem().wait_until

#define enQ_barrier shmem().barrier
#define enQ_barrier_all shmem().barrier_all
#define enQ_collect64 shmem().collect64
#define enQ_collect32 shmem().collect32
#define enQ_fcollect32 shmem().fcollect32
#define enQ_fcollect64 shmem().fcollect64
#define enQ_alltoall32 shmem().alltoall32
#define enQ_alltoalls32 shmem().alltoalls32
#define enQ_alltoall64 shmem().alltoall64
#define enQ_alltoalls64 shmem().alltoalls64
#define enQ_broadcast32 shmem().broadcast32
#define enQ_broadcast64 shmem().broadcast64

#define enQ_fam_initialize shmem().fam_initialize

#define enQ_fam_add shmem().fam_add
#define enQ_fam_compare_swap shmem().fam_compare_swap

#define enQ_fam_get_blocking shmem().fam_get_blocking
#define enQ_fam_get_nonblocking shmem().fam_get_nonblocking
#define enQ_fam_put_blocking shmem().fam_put_blocking
#define enQ_fam_put_nonblocking shmem().fam_put_nonblocking

#define enQ_fam_gather_blocking shmem().fam_gather_blocking
#define enQ_fam_gather_nonblocking shmem().fam_gather_nonblocking
#define enQ_fam_gatherv_blocking shmem().fam_gatherv_blocking
#define enQ_fam_gatherv_nonblocking shmem().fam_gatherv_nonblocking
#define enQ_fam_scatter_blocking shmem().fam_scatter_blocking
#define enQ_fam_scatter_nonblocking shmem().fam_scatter_nonblocking
#define enQ_fam_scatterv_blocking shmem().fam_scatterv_blocking
#define enQ_fam_scatterv_nonblocking shmem().fam_scatterv_nonblocking

#define enQ_int_and_to_all shmem().int_and_to_all
#define enQ_int_or_to_all shmem().int_or_to_all
#define enQ_int_xor_to_all shmem().int_xor_to_all
#define enQ_int_sum_to_all shmem().int_sum_to_all
#define enQ_int_max_to_all shmem().int_max_to_all
#define enQ_int_min_to_all shmem().int_min_to_all
#define enQ_int_prod_to_all shmem().int_prod_to_all

#define enQ_long_and_to_all shmem().long_and_to_all
#define enQ_long_or_to_all shmem().long_or_to_all
#define enQ_long_xor_to_all shmem().long_xor_to_all
#define enQ_long_sum_to_all shmem().long_sum_to_all
#define enQ_long_min_to_all shmem().long_min_to_all
#define enQ_long_max_to_all shmem().long_max_to_all
#define enQ_long_prod_to_all shmem().long_prod_to_all

#define enQ_longlong_prod_to_all shmem().longlong_prod_to_all

#define enQ_float_sum_to_all shmem().float_sum_to_all
#define enQ_float_prod_to_all shmem().float_prod_to_all
#define enQ_float_min_to_all shmem().float_min_to_all
#define enQ_float_max_to_all shmem().float_max_to_all

#define enQ_double_sum_to_all shmem().double_sum_to_all
#define enQ_double_prod_to_all shmem().double_prod_to_all
#define enQ_double_min_to_all shmem().double_min_to_all
#define enQ_double_max_to_all shmem().double_max_to_all

#define enQ_malloc shmem().malloc

using namespace Hermes;

namespace SST {
namespace Ember {

class EmberShmemGenerator : public EmberGenerator {

public:

	EmberShmemGenerator( ComponentId_t, Params& params, std::string name ="");
	~EmberShmemGenerator() {}
    virtual void completed( const SST::Output*, uint64_t time ) {}
	virtual void setup();

protected:
	EmberShmemLib& shmem() { return *m_shmem; };

	EmberMiscLib* m_miscLib;

private:
	EmberShmemLib* m_shmem;
};

}
}

#endif
