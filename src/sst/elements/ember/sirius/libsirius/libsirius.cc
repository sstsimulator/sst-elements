// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <map>

#include "sirius/siriusglobals.h"

#ifdef SIRIUS_BACKTRACE
#include <execinfo.h>
#endif

int sirius_rank;
int sirius_npes;
double load_library;

FILE* trace_dump;
std::map<MPI_Comm, uint32_t> commPtrMap;

#ifdef __MACH__
clock_serv_t the_clock;
#endif

int sirius_output = 1;

inline double get_time() {
	#ifdef __MACH__
		mach_timespec_t now;
		clock_get_time(the_clock, &now);
		double dbl_now = (now.tv_sec + (now.tv_nsec * 1.0e-9)) - load_library;
	#else
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		double dbl_now = (now.tv_sec + (now.tv_nsec * 1.0e-9)) - load_library;
	#endif

	return dbl_now;
}

__attribute__((constructor)) void init_sirius() {
	// Do special clock initialization if we are on a Mac
#ifdef __MACH__
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &the_clock);
#endif

	sirius_rank = 0;
	sirius_npes = 1;
	load_library = get_time();
}

__attribute__((destructor))  void fini_sirius() {

}

void printTime() {
	double dbl_now = get_time();
	fwrite(&dbl_now, 1, sizeof(double), trace_dump);
}

void printUINT32(uint32_t value) {
	fwrite(&value, 1, sizeof(uint32_t), trace_dump);
}

void printUINT64(uint64_t value) {
	fwrite(&value, 1, sizeof(uint64_t), trace_dump);
}

void printINT32(int32_t value) {
	fwrite(&value, 1, sizeof(int32_t), trace_dump);
}

void printMPIOp(MPI_Op op) {
	uint32_t convert = 0;

	if(op == MPI_SUM) {
		convert = SIRIUS_MPI_SUM;
	} else if(op == MPI_MAX) {
		convert = SIRIUS_MPI_MAX;
	} else if(op == MPI_MIN) {
		convert = SIRIUS_MPI_MIN;
	} else if(op == MPI_MINLOC) {
		convert = SIRIUS_MPI_MIN;
	} else if(op == MPI_MAXLOC) {
		convert = SIRIUS_MPI_MAX;
	} else {
		printf("TODO: FIXME: An unknown MPI operation was encountered, set to SUM for now.\n");
		convert = SIRIUS_MPI_SUM;
	}

	printUINT32(convert);
}

void printMPIComm(MPI_Comm comm) {
	uint32_t convert = 0;

	std::map<MPI_Comm, uint32_t>::iterator findEntry = commPtrMap.find(comm);

	if(findEntry == commPtrMap.end()) {
		// Error, can't find the communicator group
		fprintf(stderr, "Error: unable to find a communicator group in the recorded set.\n");
		PMPI_Abort(MPI_COMM_WORLD, 8);
	} else {
		convert = findEntry->second;
	}

	printUINT32(convert);
}

void printMPIDatatype(MPI_Datatype the_type) {
	uint32_t convert = 0;

	if ( the_type == MPI_INT ) {
		convert = SIRIUS_MPI_INTEGER;
	} else if ( the_type == MPI_DOUBLE ) {
		convert = SIRIUS_MPI_DOUBLE;
	} else if ( the_type == MPI_CHAR ) {
		convert = SIRIUS_MPI_CHAR;
	} else if ( the_type == MPI_LONG ) {
		convert = SIRIUS_MPI_LONG;
	} else if ( the_type == MPI_INTEGER ) {
		convert = SIRIUS_MPI_INTEGER;
	} else if ( the_type == MPI_FLOAT ) {
		convert = SIRIUS_MPI_FLOAT;
	} else if ( the_type == MPI_COMPLEX ) {
		convert = SIRIUS_MPI_COMPLEX;
	} else {
#ifdef SIRIUS_STRICT_DATATYPE
		fprintf(stderr, "Unknown data type\n");

#ifdef SIRIUS_BACKTRACE
		const int N = 100;
		void** buffer = (void**) malloc(sizeof(void*) * N);
           	char** symbol_strings;

		const int stack_items = backtrace(buffer, N);
		symbol_strings = backtrace_symbols(buffer, stack_items);

		if( NULL == symbol_strings ) {
			fprintf(stderr, "Unable to backtrace symbols\n");
		}

		for(int i = 0; i < stack_items; i++) {
			printf("[%d]: %s\n", i, symbol_strings[i]);
		}

		free(symbol_strings);
#endif
		MPI_Abort(MPI_COMM_WORLD, -1);
#else
		convert = SIRIUS_MPI_DOUBLE;
#endif
	}

	printUINT32(convert);
}

extern "C" int MPI_Init(int* argc, char** argv[]) {

	int result = PMPI_Init(argc, argv);

	PMPI_Comm_rank(MPI_COMM_WORLD, &sirius_rank);
	PMPI_Comm_size(MPI_COMM_WORLD, &sirius_npes);

	// Start with SIRIUS enabled?
	char* checkSiriusEnv = getenv("SIRIUS_ENABLE_PROFILING");

	if(NULL == checkSiriusEnv) {
		sirius_output = 1;
	} else {
		sirius_output = atoi(checkSiriusEnv);

		if(0 == sirius_output) {
			if(0 == sirius_rank) {
				printf("SIRIUS: Profiling disabled by environment variable (SIRIUS_ENABLE_PROFILING)\n");
			}
		}
	}

	char buffer[1024];
	sprintf(buffer, "%s-%d.stf.%d", (*argv)[0], sirius_npes, sirius_rank);

	trace_dump = fopen(buffer, "wb");

	printUINT32((uint32_t) SIRIUS_MPI_INIT);
	printTime();

	if(sirius_rank == 0) {
		char buffer_meta[512];
		sprintf(buffer_meta, "%s-%d.meta", 
			(*argv)[0], sirius_npes);
		FILE* meta_file = fopen(buffer_meta, "wt");

		fprintf(meta_file, "MPI Information:\n");
		fprintf(meta_file, "- Rank Count:     %8d\n", sirius_npes);
		fprintf(meta_file, "Application Information:\n");
		fprintf(meta_file, "- Arg Count:      %4d\n", *argc);

		int i = 0;
		for(i = 0; i < (*argc); ++i) {
		fprintf(meta_file, "- Arg [%4d]: %s\n", i, (*argv)[i]);
		}

		fclose(meta_file);

		printf("SIRIUS: =============================================================\n");		
		printf("SIRIUS: MPI Profiling Enabled\n");
		printf("SIRIUS: =============================================================\n");
	}

	commPtrMap.insert( std::pair<MPI_Comm, uint32_t>(MPI_COMM_WORLD, 0) );
	commPtrMap.insert( std::pair<MPI_Comm, uint32_t>(MPI_COMM_SELF,  1) );

	printTime();
	printINT32((int32_t) result);

	return result;
}

extern "C" int MPI_Comm_disconnect(MPI_Comm *comm) {
	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_COMM_DISCONNECT);
		printTime();
		printMPIComm(*comm);
	}

	const int result = PMPI_Comm_disconnect( comm );

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Comm_split(MPI_Comm comm, int color, int key,
    MPI_Comm *newcomm) {
	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_COMM_SPLIT);
		printTime();
		printMPIComm(comm);
		printINT32((int32_t) color);
		printINT32((int32_t) key);
	}

	const int result = PMPI_Comm_split(comm, color, key, newcomm);

	if(sirius_output) {
		for(uint32_t i = 2; i < UINT32_MAX; i++) {
			bool found = false;

			for(auto commSearch = commPtrMap.cbegin(); commSearch != commPtrMap.cend();
				commSearch++) {

				if(i == commSearch->second) {
					found = true;
					break;
				}
			}

			if(! found) {
				commPtrMap.insert( std::pair<MPI_Comm, uint32_t>(*newcomm, i) );
				printMPIComm(*newcomm);

				break;
			}
		}

		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Finalize() {
	// override, we must output the finalize statement
	// to allow simulations to end correctly.
	sirius_output = 1;
	printUINT32((uint32_t) SIRIUS_MPI_FINALIZE);
	printTime();

	int result = PMPI_Finalize();

	printTime();
	printINT32((int32_t) result);

	fclose(trace_dump);

	return result;
}

extern "C" int MPI_Pcontrol(int control, ...) {
	if(control == 0) {
		sirius_output = 0;

		if(sirius_rank == 0) {
			printf("SIRIUS: Disable profiling.\n");
		}
	} else if (control == 1) {
		sirius_output = 1;

		if(sirius_rank == 0) {
			printf("SIRIUS: Enable profiling.\n");
		}
	}

	return MPI_SUCCESS;
}

extern "C" int MPI_Send(SIRIUS_MPI_CONST void* buffer,
	int count, MPI_Datatype datatype, int dest, int tag,
	MPI_Comm comm) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_SEND);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);
		printINT32((int32_t) dest);

		if(MPI_ANY_TAG == tag) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) tag);
		}

		printMPIComm(comm);
	}

	const int result = PMPI_Send(buffer, count, datatype, dest, tag, comm);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Irecv(void *buffer, int count, MPI_Datatype datatype, int src,
              int tag, MPI_Comm comm, MPI_Request *request) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_IRECV);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);

		if(MPI_ANY_SOURCE == src) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) src);
		}

		if(MPI_ANY_TAG == tag) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) tag);
		}

		printMPIComm(comm);
	}

	const int result = PMPI_Irecv(buffer, count, datatype, src, tag, comm, request);

	if(sirius_output) {
		if( (NULL == request) || ( (*request) == MPI_REQUEST_NULL) ) {
			printUINT64((uint64_t) SIRIUS_MPI_REQUEST_NULL);
		} else {
			printUINT64((uint64_t) (*request));
		}

		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Isend(SIRIUS_MPI_CONST void *buffer, int count,
		MPI_Datatype datatype, int dest,
              	int tag, MPI_Comm comm, MPI_Request *request) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_ISEND);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);
		printINT32((int32_t) dest);

		if(MPI_ANY_TAG == tag) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) tag);
		}
	}

	int result = PMPI_Isend(buffer, count, datatype, dest, tag, comm, request);

	if(sirius_output) {
		printMPIComm(comm);

		if( (NULL == request) || ( (*request) = MPI_REQUEST_NULL) ) {
			printUINT64((uint64_t) SIRIUS_MPI_REQUEST_NULL);
		} else {
			printUINT64((uint64_t) (*request));
		}

		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Recv(void* buffer, int count, MPI_Datatype datatype, int src, int tag,
	MPI_Comm comm, MPI_Status* status) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_RECV);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);

		if(MPI_ANY_SOURCE == src) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) src);
		}

		if(MPI_ANY_TAG == tag) {
			printINT32((int32_t) INT32_MAX);
		} else {
			printINT32((int32_t) tag);
		}

		printMPIComm(comm);
	}

	const int result = PMPI_Recv(buffer, count, datatype, src, tag, comm, status);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Barrier(MPI_Comm comm) {
	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_BARRIER);
		printTime();
		printMPIComm(comm);
	}

	const int result = PMPI_Barrier(MPI_COMM_WORLD);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Allreduce(SIRIUS_MPI_CONST void* buffer, void* recv,
		int count, MPI_Datatype datatype,
		MPI_Op op, MPI_Comm comm) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_ALLREDUCE);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT64((uint64_t) recv);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);
		printMPIOp(op);
		printMPIComm(comm);
	}

	const int result = PMPI_Allreduce(buffer, recv, count, datatype, op, comm);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Wait(MPI_Request *request, MPI_Status *status) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_WAIT);
		printTime();

		if( (NULL == request) || MPI_REQUEST_NULL == (*request) ) {
                        printUINT64((uint64_t) SIRIUS_MPI_REQUEST_NULL);
                } else {
                        printUINT64((uint64_t) (*request));
                }

		printUINT64((uint64_t) status);
	}

	const int result = PMPI_Wait(request, status);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Waitall(int count, MPI_Request array_of_requests[],
    		MPI_Status *array_of_statuses) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_WAITALL);
		printTime();
		printUINT32((uint32_t) count);

		for(int i = 0; i < count; i++) {
			if( MPI_REQUEST_NULL == (array_of_requests[i]) ) {
                        	printUINT64((uint64_t) SIRIUS_MPI_REQUEST_NULL);
                	} else {
                        	printUINT64((uint64_t) array_of_requests[i]);
                	}
		}
	}

	const int result = PMPI_Waitall(count, array_of_requests, array_of_statuses);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_BCAST);
		printTime();
		printUINT64((uint64_t) buffer);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);
		printINT32((int32_t) root);
		printMPIComm(comm);
	}

	const int result = PMPI_Bcast(buffer, count, datatype, root, comm);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}

extern "C" int MPI_Reduce(SIRIUS_MPI_CONST void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root,
               MPI_Comm comm) {

	if(sirius_output) {
		printUINT32((uint32_t) SIRIUS_MPI_REDUCE);
		printTime();
		printUINT64((uint64_t) sendbuf);
		printUINT64((uint64_t) recvbuf);
		printUINT32((uint32_t) count);
		printMPIDatatype(datatype);
		printMPIOp(op);
		printINT32((int32_t) root);
		printMPIComm(comm);
	}

	const int result = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

	if(sirius_output) {
		printTime();
		printINT32((int32_t) result);
	}

	return result;
}
