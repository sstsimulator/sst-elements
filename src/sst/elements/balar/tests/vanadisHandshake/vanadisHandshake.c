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

int main( int argc, char* argv[] ) {
	printf("Hello World from Vanadis and Balar\n");

	double * devptr;
	BalarCudaCallPacket_t* gpu = (BalarCudaCallPacket_t*) 0xFFFF1000;
	BalarCudaCallReturnPacket_t* gpu_res = (BalarCudaCallReturnPacket_t*) 0xFFFF1000;
	
	// Send request to GPU
	BalarCudaCallPacket_t packet;
	packet.cuda_call_id = GPU_MALLOC;
	packet.cuda_malloc.devPtr = &devptr;
	packet.cuda_malloc.size = 200;

	*gpu = packet;

	// Read from GPU
	BalarCudaCallReturnPacket_t response = *gpu_res;
	printf("CUDA API ID: %d with error: %d\nMalloc addr: %p Dev addr: %p\n", 
			response.cuda_call_id, response.cuda_error,
			response.cudamalloc.malloc_addr, response.cudamalloc.devptr_addr);

	return 0;
}
