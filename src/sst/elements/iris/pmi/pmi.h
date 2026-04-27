#ifndef PMI_H_INCLUDED
#define PMI_H_INCLUDED

#define PMI_SUCCESS                  0
#define PMI_FAIL                    -1
#define PMI_ERR_INIT                 1
#define PMI_ERR_NOMEM                2
#define PMI_ERR_INVALID_ARG          3
#define PMI_ERR_INVALID_KEY          4
#define PMI_ERR_INVALID_KEY_LENGTH   5
#define PMI_ERR_INVALID_VAL          6
#define PMI_ERR_INVALID_VAL_LENGTH   7
#define PMI_ERR_INVALID_LENGTH       8
#define PMI_ERR_INVALID_NUM_ARGS     9
#define PMI_ERR_INVALID_ARGS        10
#define PMI_ERR_INVALID_NUM_PARSED  11
#define PMI_ERR_INVALID_KEYVALP     12
#define PMI_ERR_INVALID_SIZE        13

typedef int PMI_BOOL;
#define PMI_TRUE  1
#define PMI_FALSE 0

typedef struct PMI_keyval_t {
    const char * key;
    char * val;
} PMI_keyval_t;

#ifdef __cplusplus
extern "C" {
#endif

int PMI_Init(int *spawned);
int PMI_Initialized(int *initialized);
int PMI_Finalize(void);
int PMI_Abort(int exit_code, const char error_msg[]);

int PMI_Get_size(int *size);
int PMI_Get_rank(int *rank);
int PMI_Get_universe_size(int *size);
int PMI_Get_appnum(int *appnum);

int PMI_Barrier(void);
int PMI_Ibarrier(void);
int PMI_Wait(void);
int PMI_Allgather(void *in, void *out, int len);

int PMI_KVS_Get_my_name(char kvsname[], int length);
int PMI_KVS_Get_name_length_max(int *length);
int PMI_KVS_Get_key_length_max(int *length);
int PMI_KVS_Get_value_length_max(int *length);
int PMI_KVS_Put(const char kvsname[], const char key[], const char value[]);
int PMI_KVS_Commit(const char kvsname[]);
int PMI_KVS_Get(const char kvsname[], const char key[], char value[], int length);

int PMI_Publish_name(const char service_name[], const char port[]);
int PMI_Unpublish_name(const char service_name[]);
int PMI_Lookup_name(const char service_name[], char port[]);

int PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizesp[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[]);

int PMI_Get_nidlist_ptr(void **nidlist);
int PMI_Get_numpes_on_smp(int *num);

/* PMI-2 */
int PMI2_Init(int *spawned, int *size, int *rank, int *appnum);
int PMI2_Finalize(void);
int PMI2_Abort(void);
int PMI2_Job_GetId(char jobid[], int jobid_size);
int PMI2_KVS_Put(const char key[], const char value[]);
int PMI2_KVS_Get(const char *jobid, int src_pmi_id, const char key[],
                 char value[], int maxvalue, int *vallen);
int PMI2_KVS_Fence(void);

#ifdef __cplusplus
}
#endif

#endif /* PMI_H_INCLUDED */
