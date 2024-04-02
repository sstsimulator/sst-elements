#include <cstdio>
#include <cstdlib>
#include <mpi.h>
#include <omp.h>

int ariel_enable() {
    printf("App: ariel_enable called\n");
    return 0;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    ariel_enable();
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

#pragma omp parallel
    {
        int thread = omp_get_thread_num();

#pragma omp critical
        {
        if (!std::getenv("FAKEPIN")) {
            printf("Hello from rank %d/%d, thread %d!", rank, nranks, thread);
        } else {
            printf("Hello from rank %d/%d, thread %d! (Launched by fakepin)", rank, nranks, thread);
        }

        for (int i = 1; i < argc; i++) {
            printf(" -- %s", argv[i]);
        }
        printf("\n");
        }
    }

    int compute = 0;
    if (argc > 1) {
        compute = atoi(argv[1]);
    }

    if (compute) {
        if (rank == 0) {
            int *vec_a = (int*) malloc(sizeof(int) * compute);
            for (int i = 0; i < compute; i++) {
                vec_a[i] = 2;
            }
            for (int i = 0; i < compute; i++) {
                for (int j = 0; j < i; j++) {
                    for (int k = 0; k < j; k++) {
                            vec_a[i] += vec_a[j] + vec_a[k] + (i % 7 == 0 ? 3 : 5);
                    }
                }
            }
            printf("Rank 0: vec_a[%d] is %d\n", compute-1, vec_a[compute-1]);
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        printf("Rank 0: Barrier complete.\n");
    }

    MPI_Finalize();
}
