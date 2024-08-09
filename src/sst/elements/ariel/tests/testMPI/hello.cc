#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>
#include <omp.h>
#include <arielapi.h>

// Useage: ./hello [output-file]
// If running with multiple ranks, each will output to its own file
int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);

    int rank = 0;
    int nranks = 0;
    int ret  = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (ret != MPI_SUCCESS) {
        printf("Error: MPI_Comm_rank retuned error: %d\n", ret);
        exit(1);
    }
    ret  = MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    if (ret != MPI_SUCCESS) {
        printf("Error: MPI_Comm_rank retuned error: %d\n", ret);
        exit(1);
    }

    // Redirect output to a file if an argument was given. Append the rank to each filename.
    FILE *output = stdout;
    if (argc > 1) {
        int len = strlen(argv[1]) + 12;// Space for underscore, plus up to a 10 digit integer, plus the null character
        char *outfile = (char*)malloc(len);
        if (!outfile) {
            printf("Error allocating space for filename\n");
        }
        snprintf(outfile, len, "%s_%d", argv[1], rank);
        output = fopen(outfile, "w");
        if (!output) {
            printf("Error opening %s\n");
            exit(1);
        }
        free(outfile);
    }


    ariel_enable();
#pragma omp parallel
    {
        int thread = omp_get_thread_num();
#pragma omp critical
        {
            // ./fakepin sets the FAKEPIN environment variale. This is useful for debugging but
            // not needed for our Ariel MPI testsuite.
            // We only want output from the traced process
            if (std::getenv("FAKEPIN")) {
                fprintf(output, "Hello from rank %d of %d, thread %d! (Launched by fakepin)\n", rank, nranks, thread);
            } else if (std::getenv("PIN_CRT_TZDATA") || std::getenv("PIN_APP_LD_LIBRARY_PATH")) {
                fprintf(output, "Hello from rank %d of %d, thread %d! (Launched by pin)\n", rank, nranks, thread);
            } else {
                fprintf(output, "Hello from rank %d of %d, thread %d!\n", rank, nranks, thread);
            }
        }
    }

    // This is here just to make sure it doesn't crash when the processes try to communicate.
    MPI_Barrier(MPI_COMM_WORLD);
    ariel_disable();
    if (rank == 0) {
        printf("Rank 0: Barrier complete.\n");
    }

    if (argc > 1) {
        fclose(output);
    }

    MPI_Finalize();
}
