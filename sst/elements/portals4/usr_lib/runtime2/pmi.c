/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 * This is the PMI client for the Kitten lightweight kernel.
 * It uses Portals4 to communicate with the Kitten PCT (the PMI server).
 * The code is based on simple_pmi from Argonne, updated to use Portals4.
 */

/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*********************** PMI implementation ********************************/
/*
 * This file implements the client-side of the PMI interface.
 *
 * Note that the PMI client code must not print error messages (except 
 * when an abort is required) because MPI error handling is based on
 * reporting error codes to which messages are attached.  
 *
 * In v2, we should require a PMI client interface to use MPI error codes
 * to provide better integration with MPICH2.  
 */
/***************************************************************************/

#define PMI_VERSION    1
#define PMI_SUBVERSION 1

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <portals4.h>

#define FOOBAR 1 
#include "pmi.h"
#include "kvs.h"
#include "simple_pmiutil.h"
#include "barrier.h"


#ifndef MPI_MAX_PORT_NAME
#define MPI_MAX_PORT_NAME 128
#endif


/* Set PMI_initialized to 1 for singleton init but no process manager
 * to help.  Initialized to 2 for normal initialization.  Initialized
 * to values higher than 2 when singleton_init by a process manager.
 * All values higher than 1 invlove a PM in some way. */
typedef enum {
	PMI_UNINITIALIZED = 0, 
	NORMAL_INIT_WITH_PM = 2,
} PMIState;


/* PMI client state */
static PMIState PMI_initialized = PMI_UNINITIALIZED;
static int      PMI_size = -1;
static int      PMI_rank = -1;
static int      PMI_kvsname_max = 0;
static int      PMI_keylen_max = 0;
static int      PMI_vallen_max = 0;
static int      PMI_debug = 0;
//static int      PMI_spawned = 0;


static int
GetResponse(const char *req, const char *expectedCmd, int checkRc)
{
    printf("%s():%d\n",__func__,__LINE__);
    abort();
}


static int
PMII_getmaxes(int *kvsname_max, int *keylen_max, int *vallen_max)
{
    *kvsname_max = 1024;
	*keylen_max = 1024;
	*vallen_max = 1024;
    return PMI_SUCCESS;
}


int
PMI_Initialized(PMI_BOOL *initialized)
{
    printf("%s():%d\n",__func__,__LINE__);
	/* Turn this into a logical value (1 or 0). This allows us
	 * to use PMI_initialized to distinguish between initialized with
	 * an PMI service (e.g., via mpiexec) and the singleton init, 
	 * which has no PMI service. */
	*initialized = (PMI_initialized != 0);
	return PMI_SUCCESS;
}


int
PMI_Init(int *spawned)
{
	char *p;
    printf("%s():%d\n",__func__,__LINE__);
    
	PMI_initialized = PMI_UNINITIALIZED;

	if ((p = getenv("PMI_DEBUG")) != NULL) {
		PMI_debug = atoi(p);
	} else {
		PMI_debug = 0;
	}

	if ((p = getenv("PMI_SIZE")) != NULL) {
		PMI_size = atoi(p);
	} else {
		return PMI_FAIL;
	}
	
	if ((p = getenv("PMI_RANK")) != NULL) {
		PMI_rank = atoi(p);
		/* Let the util routine know the rank of this process for 
		   any messages (usually debugging or error) */
		PMIU_Set_rank(PMI_rank);
	} else {
		return PMI_FAIL;
	}


	if (PMII_getmaxes(&PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max) != 0)
		return PMI_FAIL;

	*spawned = 0;

	PMI_initialized = NORMAL_INIT_WITH_PM;

#if FOOBAR
    if ( PMI_rank == 0 ) {
        kvs_put( "[5, 1]-ompi.rte-info", "1:m5.eecs.umich.edu,0" );
        kvs_put( "[5, 1]-MPI_THREAD_LEVEL", "AS  " );
        kvs_put( "[5, 1]-OMPI_ARCH", "4291367424" );
        kvs_put( "[5, 1]-mtl.portals4.1.9", "AQAAAAEAAAD " );
    } else {
        kvs_put( "[5, 0]-ompi.rte-info", "1:m5.eecs.umich.edu,0" );
        kvs_put( "[5, 0]-MPI_THREAD_LEVEL", "AS  ");
        kvs_put( "[5, 0]-OMPI_ARCH", "4291367424" );
        kvs_put( "[5, 0]-mtl.portals4.1.9", "AAAAAAEAAAD " );
    }
#endif

	return PMI_SUCCESS;
}


int
PMI_Get_size(int *size)
{
    printf("%s():%d %d\n",__func__,__LINE__,PMI_size);
	*size = PMI_size;
	return PMI_SUCCESS;
}


int
PMI_Get_rank(int *rank)
{
    printf("%s():%d %d\n",__func__,__LINE__,PMI_rank);
	*rank = PMI_rank;
	return PMI_SUCCESS;
}


int
PMI_Get_universe_size(int *size)
{
    printf("%s():%d\n",__func__,__LINE__);
    *size = 2;

	return PMI_SUCCESS;
}


int 
PMI_Get_appnum(int *appnum)
{
    printf("%s():%d\n",__func__,__LINE__);
	*appnum = 5;

	return PMI_SUCCESS;
}


int
PMI_Barrier(void)
{
    printf("%s():%d\n",__func__,__LINE__);
    cnos_barrier();
	return PMI_SUCCESS;
}


int
PMI_Finalize(void)
{
    printf("%s():%d\n",__func__,__LINE__);
	int status = GetResponse("cmd=finalize\n", "finalize_ack", 0);
	return status;
}


int
PMI_Abort(int exit_code, const char *error_msg)
{
	PMIU_printf(1, "aborting job:\n%s\n", error_msg);
	exit(exit_code);
	return -1;
}


/************************************* Keymap functions **********************/


int
PMI_KVS_Get_my_name(char *kvsname, int length)
{
    printf("%s():%d\n",__func__,__LINE__);
    strcpy( kvsname, "GEM5_KVS");

	return PMI_SUCCESS;
}


int
PMI_KVS_Get_name_length_max(int *maxlen)
{
    printf("%s():%d\n",__func__,__LINE__);
	*maxlen = PMI_kvsname_max;
	return PMI_SUCCESS;
}


int
PMI_KVS_Get_key_length_max(int *maxlen)
{
    printf("%s():%d\n",__func__,__LINE__);
	*maxlen = PMI_keylen_max;
	return PMI_SUCCESS;
}


int
PMI_KVS_Get_value_length_max(int *maxlen)
{
    printf("%s():%d\n",__func__,__LINE__);
	*maxlen = PMI_vallen_max;
	return PMI_SUCCESS;
}


int
PMI_KVS_Put(const char *kvsname, const char *key, const char *value)
{
#ifdef FOOBAR
    kvs_put( key, value );
#endif
    return PMI_SUCCESS;
}


int
PMI_KVS_Get(const char *kvsname, const char *key, char *value, int length)
{
#ifdef FOOBAR
    kvs_get( key, value, length );
#endif
	return PMI_SUCCESS;
}


int
PMI_KVS_Commit(const char *kvsname)
{
	/* no-op in this implementation */
	return PMI_SUCCESS;
}


/*************************** Name Publishing functions **********************/


int
PMI_Publish_name(const char *service_name, const char *port)
{
	char buf[PMIU_MAXLINE];
	int status;

	status = snprintf(buf, PMIU_MAXLINE,
	                  "cmd=publish_name service=%s port=%s\n",
	                  service_name, port);
	if (status < 0)
		return PMI_FAIL;

    printf("%s():%d\n",__func__,__LINE__);
	status = GetResponse(buf, "publish_result", 0);
	if (status != PMI_SUCCESS)
		return status;

	/* FIXME: This should have used rc and msg */
	PMIU_getval("info", buf, PMIU_MAXLINE);
	if (strcmp(buf, "ok") != 0) {
		PMIU_printf(1, "publish failed; reason = %s\n", buf);
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
}


int
PMI_Unpublish_name(const char *service_name)
{
	char buf[PMIU_MAXLINE];
	int status;

	status = snprintf(buf, PMIU_MAXLINE,
	                  "cmd=unpublish_name service=%s\n", 
	                  service_name);
	if (status < 0)
		return PMI_FAIL;

    printf("%s():%d\n",__func__,__LINE__);
	status = GetResponse(buf, "unpublish_result", 0);
	if (status != PMI_SUCCESS)
		return status;

	PMIU_getval("info", buf, PMIU_MAXLINE);
	if (strcmp(buf, "ok") != 0) {
		PMIU_printf(1, "unpublish failed; reason = %s\n", buf);
		return PMI_FAIL;
	}

	return PMI_SUCCESS;
}


int
PMI_Lookup_name(const char *service_name, char *port)
{
	char buf[PMIU_MAXLINE];
	int status;

	status = snprintf(buf, PMIU_MAXLINE,
	                  "cmd=lookup_name service=%s\n",
	                  service_name);
	if (status < 0)
		return PMI_FAIL;


    printf("%s():%d\n",__func__,__LINE__);
	status = GetResponse(buf, "lookup_result", 0);
	if (status != PMI_SUCCESS)
		return status;

	PMIU_getval("info", buf, PMIU_MAXLINE);
	if (strcmp(buf, "ok") != 0) {
		PMIU_printf(1, "lookup failed; reason = %s\n", buf);
		return PMI_FAIL;
	}

	PMIU_getval("port", port, MPI_MAX_PORT_NAME);

	return PMI_SUCCESS;
}


/************************** Process Creation functions **********************/


int
PMI_Spawn_multiple(int			count,
                   const char *         cmds[],
                   const char **        argvs[],
                   const int            maxprocs[],
                   const int            info_keyval_sizes[],
                   const PMI_keyval_t * info_keyval_vectors[],
                   int                  preput_keyval_size,
                   const PMI_keyval_t   preput_keyval_vector[],
                   int                  errors[]
)
{
	return PMI_FAIL;
}

int 
PMI_Get_clique_size( int *size )
{
    printf("%s():%d\n",__func__,__LINE__);
    *size = 1;
	return PMI_SUCCESS;
}

int
PMI_Get_clique_ranks( int* node_ranks, int size)
{
    printf("%s():%d\n",__func__,__LINE__);
    node_ranks[0] = PMI_rank;
	return PMI_SUCCESS;
}

