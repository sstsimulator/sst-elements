/**
 * @file vanadisHandshake.c
 * @author Weili An (an107@purdue.edu)
 * @brief A simple program to test balar and vanadis handshake
 *        by writing packet to the MMIO address of balar
 * @version 0.1
 * @date 2022-06-01
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


enum GpuApi_t {
	GPU_REG_FAT_BINARY = 1,
	GPU_REG_FAT_BINARY_RET = 2,
	GPU_REG_FUNCTION = 3,
	GPU_REG_FUNCTION_RET = 4,
	GPU_MEMCPY = 5,
	GPU_MEMCPY_RET = 6,
	GPU_CONFIG_CALL = 7,
	GPU_CONFIG_CALL_RET = 8,
	GPU_SET_ARG = 9,
	GPU_SET_ARG_RET = 10,
	GPU_LAUNCH = 11,
	GPU_LAUNCH_RET = 12,
	GPU_FREE = 13,
	GPU_FREE_RET = 14,
	GPU_GET_LAST_ERROR = 15,
	GPU_GET_LAST_ERROR_RET = 16,
	GPU_MALLOC = 17,
	GPU_MALLOC_RET = 18,
	GPU_REG_VAR = 19,
	GPU_REG_VAR_RET = 20,
	GPU_MAX_BLOCK = 21,
	GPU_MAX_BLOCK_RET = 22,
};

enum cudaMemcpyKind
{
    cudaMemcpyHostToHost          =   0,      /**< Host   -> Host */
    cudaMemcpyHostToDevice        =   1,      /**< Host   -> Device */
    cudaMemcpyDeviceToHost        =   2,      /**< Device -> Host */
    cudaMemcpyDeviceToDevice      =   3,      /**< Device -> Device */
    cudaMemcpyDefault             =   4       /**< Direction of the transfer is inferred from the pointer values. Requires unified virtual addressing */
};

enum cudaError
{
  cudaSuccess = 0,
  cudaErrorMissingConfiguration = 1,
  cudaErrorMemoryAllocation = 2,
  cudaErrorInitializationError = 3,
  cudaErrorLaunchFailure = 4,
  cudaErrorPriorLaunchFailure = 5,
  cudaErrorLaunchTimeout = 6,
  cudaErrorLaunchOutOfResources = 7,
  cudaErrorInvalidDeviceFunction = 8,
  cudaErrorInvalidConfiguration = 9,
  cudaErrorInvalidDevice = 10,
  cudaErrorInvalidValue = 11,
  cudaErrorInvalidPitchValue = 12,
  cudaErrorInvalidSymbol = 13,
  cudaErrorMapBufferObjectFailed = 14,
  cudaErrorUnmapBufferObjectFailed = 15,
  cudaErrorInvalidHostPointer = 16,
  cudaErrorInvalidDevicePointer = 17,
  cudaErrorInvalidTexture = 18,
  cudaErrorInvalidTextureBinding = 19,
  cudaErrorInvalidChannelDescriptor = 20,
  cudaErrorInvalidMemcpyDirection = 21,
  cudaErrorAddressOfConstant = 22,
  cudaErrorTextureFetchFailed = 23,
  cudaErrorTextureNotBound = 24,
  cudaErrorSynchronizationError = 25,
  cudaErrorInvalidFilterSetting = 26,
  cudaErrorInvalidNormSetting = 27,
  cudaErrorMixedDeviceExecution = 28,
  cudaErrorCudartUnloading = 29,
  cudaErrorUnknown = 30,
  cudaErrorNotYetImplemented = 31,
  cudaErrorMemoryValueTooLarge = 32,
  cudaErrorInvalidResourceHandle = 33,
  cudaErrorNotReady = 34,
  cudaErrorInsufficientDriver = 35,
  cudaErrorSetOnActiveProcess = 36,
  cudaErrorNoDevice = 38,
  cudaErrorStartupFailure = 0x7f,
  cudaErrorApiFailureBase = 10000
};

typedef enum cudaError cudaError_t;

// TODO: Make this into a class with additional serialization methods?
// TODO: Make this into subclass of standardmem::request? and override the makeResponse function?
typedef struct BalarCudaCallPacket {
	enum GpuApi_t cuda_call_id;
	union {
		struct {
			// Use uint64_t to match with the 8 byte ptr width
			// in balar
			uint64_t devPtr;
			size_t size;
		} cuda_malloc;

		struct {
			char file_name[256];
		} register_fatbin;

		struct {
			uint64_t fatCubinHandle;
			uint64_t hostFun;
			char deviceFun[256];
		} register_function;
		
		struct {
			uint64_t dst;
			uint64_t src;
			size_t count;
			enum cudaMemcpyKind kind;
			volatile uint8_t *payload;
		} cuda_memcpy;

		struct {
			unsigned int gdx;
			unsigned int gdy;
			unsigned int gdz;
			unsigned int bdx;
			unsigned int bdy;
			unsigned int bdz;
			size_t sharedMem;
			uint64_t stream;
		} configure_call;

		struct {
			uint64_t arg;
			uint8_t value[8];
			size_t size;
			size_t offset;
		} setup_argument;

		struct {
			uint64_t func;
		} cuda_launch;

		struct {
			uint64_t devPtr;
		} cuda_free;

		struct {
			uint64_t fatCubinHandle;
			uint64_t hostVar; //pointer to...something
			uint64_t deviceAddress; //name of variable
			uint64_t deviceName; //name of variable
			int ext;
			int size;
			int constant;
			int global;
		} register_var;

		struct {
			uint64_t numBlock;
			uint64_t hostFunc;
			int blockSize;
			size_t dynamicSMemSize;
			unsigned int flags;
		} max_active_block;
	};
} BalarCudaCallPacket_t;

typedef struct BalarCudaCallReturnPacket {
	enum GpuApi_t cuda_call_id;
	cudaError_t cuda_error;
	union {
		uint64_t    fat_cubin_handle;
		struct {
			uint64_t    malloc_addr;
			uint64_t    devptr_addr;
		} cudamalloc;
		struct {
			volatile uint64_t    sim_data;
			volatile uint64_t    real_data;
			size_t      size;
			enum cudaMemcpyKind kind;
		} cudamemcpy;
	};
} BalarCudaCallReturnPacket_t;

int main( int argc, char* argv[] ) {
	printf("Hello from Vanadis and Balar\n");

	double * devptr;
	uint32_t* gpu = (uint32_t*) 0xFFFF1000;
	
	// TODO Assign a scratch memory via malloc to let the GPU manipulates on
	uint8_t *scratch_mem = calloc(512, sizeof(uint8_t));

	// Send request to GPU
	BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *)scratch_mem;
	call_packet_ptr->cuda_call_id = GPU_MALLOC;
	call_packet_ptr->cuda_malloc.size = 200;
	call_packet_ptr->cuda_malloc.devPtr = &devptr;
	printf("Packet address: %p\n", call_packet_ptr);
	printf("Packet size: %d\n", call_packet_ptr->cuda_malloc.size);
	printf("Packet devptr: %p\n", call_packet_ptr->cuda_malloc.devPtr);
	fflush(stdout);

	// Vanadis write with 4B chunk, thus use uint32_t to pass
	// the pointer to the balar packet
	*gpu = (uint32_t*) call_packet_ptr;

	// Read from GPU will return the address to the cuda return packet
	BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*gpu;
	printf("CUDA API ID: %d with error: %d\nMalloc addr: %p Dev addr: %p\n", 
			response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
			response_packet_ptr->cudamalloc.malloc_addr, response_packet_ptr->cudamalloc.devptr_addr);

	return 0;
}
