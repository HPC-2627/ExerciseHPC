#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* Message sizes tested: 1MB, 2MB, 4MB, ... up to MAX_BYTES (exponential growth) */
#define MIN_BYTES   (1L * 1024 * 1024)
#define MAX_BYTES   (128L * 1024 * 1024)
#define NUM_TRIALS  20   /* repeated trials per size, for a statistically stable average */

int main(int argc, char** argv) {
    int comm_sz;   /* Number of processes */
    int my_rank;   /* My process rank */

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (comm_sz != 2) {
        if (my_rank == 0)
            fprintf(stderr, "This benchmark requires exactly 2 processes (got %d)\n", comm_sz);
        MPI_Finalize();
        return 1;
    }

    /* Single buffer reused for every size; allocate once at the max size tested */
    char* buf = (char*) malloc((size_t) MAX_BYTES);
    if (buf == NULL) {
        fprintf(stderr, "Rank %d: malloc failed for %ld bytes\n", my_rank, MAX_BYTES);
        MPI_Finalize();
        return 1;
    }
    for (long i = 0; i < MAX_BYTES; i++) {
        buf[i] = (char) (i & 0xFF);
    }

    if (my_rank == 0)
        printf("%-12s %-15s %-15s\n", "Bytes", "Time (s)", "Rate (MB/s)");

    for (long size = MIN_BYTES; size <= MAX_BYTES; size *= 2) {
        double total_time = 0.0;

        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            /* Barrier before each trial keeps both ranks starting from the same point,
             * so the timer on rank 0 isn't skewed by rank 1 arriving late. */
            MPI_Barrier(MPI_COMM_WORLD);

            if (my_rank == 0) {
                double start = MPI_Wtime();
                MPI_Send(buf, (int) size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                /* 1-byte ack from rank 1 lets rank 0 time the full round trip;
                 * with only 2 ranks and no synchronized clocks, this is the only
                 * way to measure "time to send" from a single process. */
                MPI_Recv(buf, 1, MPI_CHAR, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                double finish = MPI_Wtime();
                total_time += (finish - start);
            } else {
                MPI_Recv(buf, (int) size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
            }
        }

        if (my_rank == 0) {
            double avg_time = total_time / NUM_TRIALS;
            double mb = (double) size / (1024.0 * 1024.0);
            double rate = mb / avg_time;
            printf("%-12ld %-15.6f %-15.2f\n", size, avg_time, rate);
        }
    }

    free(buf);
    MPI_Finalize();
    return 0;
}
