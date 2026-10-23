/*
 * ============================================================================
 *  POINT-TO-POINT COMMUNICATION BENCHMARK (MPI_Send / MPI_Recv)
 * ============================================================================
 *
 *  GOAL
 *  Measure the time needed to send messages of increasing size
 *  (1, 2, 4, ..., 2^MAX_EXP bytes) from one process to another and, from
 *  that, estimate the LATENCY (small messages) and the BANDWIDTH (large
 *  messages).
 *
 *  HOW IT WORKS
 *  Two processes (rank 0 and rank 1) run a "ping-pong":
 *      rank 0 --- n-byte message ---> rank 1
 *      rank 0 <--- n-byte message --- rank 1
 *  For each size the ping-pong is repeated many times and each repetition is
 *  timed separately with MPI_Wtime, on rank 0 only (so there is no need to
 *  synchronize the clocks of different nodes).
 *
 *  FORMULAS
 *      one-way time   t = (round-trip time) / 2
 *      bandwidth      B = n / t        [MB/s, with 1 MB = 10^6 bytes]
 *
 *  USAGE
 *      mpirun -np 2 ./pingpong_bench [max_exp=20] [reps=200] [warmup=20]
 *        max_exp : the maximum size is 2^max_exp bytes (20 -> 1 MiB)
 *        reps    : number of measured repetitions for each size
 *        warmup  : initial repetitions that are discarded (warm-up)
 *
 *  OUTPUT (stdout, CSV format; lines starting with '#' are metadata)
 *      n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* How many bytes correspond to "1 MB" in the bandwidth computation.
 * - 1.0e6 -> decimal MB; for MiB use 1048576.0.
 * - (The convention used must be stated in the report.) */
#define MB_BYTES 1.0e6

/* Comparison function for qsort: used to sort the measured times
 * (needed to compute the minimum and the median). */
static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);   /* -1 if x<y, 0 if equal, +1 if x>y */
}

int main(int argc, char **argv) {
    int rank, size;

    /* ---- Initialization of the MPI environment ---- */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   /* my identifier (0 or 1) */
    MPI_Comm_size(MPI_COMM_WORLD, &size);   /* total number of processes */

    /* ---- Check: the benchmark requires EXACTLY 2 processes ---- */
    if (size != 2) {
        if (rank == 0) fprintf(stderr, "Errore: eseguire con esattamente 2 processi (-np 2).\n");
        MPI_Finalize();
        return 1;
    }

    /* ---- Read the parameters from the command line (with default values) ---- */
    int max_exp = (argc > 1) ? atoi(argv[1]) : 20;     /* 2^20 bytes = 1 MiB */
    int reps    = (argc > 2) ? atoi(argv[2]) : 200;    /* measured repetitions */
    int warmup  = (argc > 3) ? atoi(argv[3]) : 20;     /* warm-up repetitions */

    /* Parameter validation (max_exp <= 30 avoids buffers > 1 GiB;
     * reps >= 2 is needed because the standard deviation divides by reps-1). */
    if (max_exp < 0 || max_exp > 30 || reps < 2 || warmup < 0) {
        if (rank == 0) fprintf(stderr, "Argomenti non validi.\n");
        MPI_Finalize();
        return 1;
    }

    /* ---- Buffer allocation ---- */
    size_t max_size = (size_t)1 << max_exp;                 /* bytes of the largest message */
    char *buf = malloc(max_size);                           /* buffer used for all messages */
    double *times = malloc((size_t)reps * sizeof(double));  /* one-way times of the repetitions */
    if (!buf || !times) {
        fprintf(stderr, "Rank %d: malloc fallita\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* "Touch" all the buffer pages BEFORE measuring: the first write to
     * memory causes page faults that would distort the timings. */
    memset(buf, rank + 1, max_size);

    /* ---- Metadata: where do the two ranks run? ----
     * Used to verify that the PBS placement (pack / scatter) is the intended
     * one: with "pack" both ranks are on the same node, with "scatter" on
     * different nodes. */
    char name[MPI_MAX_PROCESSOR_NAME] = {0}, name1[MPI_MAX_PROCESSOR_NAME] = {0};
    int len;
    MPI_Get_processor_name(name, &len);      /* name of the host I am running on */
    if (rank == 1) {
        /* rank 1 sends its own hostname to rank 0 (tag 99 so it is not
         * confused with the benchmark messages, which use tag 0) */
        MPI_Send(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, 99, MPI_COMM_WORLD);
    } else {
        /* rank 0 receives the hostname of rank 1 and prints the header */
        MPI_Recv(name1, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 1, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("# rank0_host=%s rank1_host=%s same_node=%s\n", name, name1,
               strcmp(name, name1) == 0 ? "yes" : "no");
        /* MPI_Wtick = timer resolution: the measured times must be much
         * larger than this value to be reliable */
        printf("# wtick=%.3e max_exp=%d reps=%d warmup=%d\n", MPI_Wtick(), max_exp, reps, warmup);
        /* CSV column headers */
        printf("n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps\n");
    }

    /* ======================= LOOP OVER THE SIZES ======================= */
    for (int e = 0; e <= max_exp; e++) {
        int n = 1 << e;     /* current size: 1, 2, 4, ..., 2^max_exp bytes */

        /* For large messages (> 64 KiB) fewer repetitions are used (reps/5, but
         * at least 20): each repetition takes longer and the relative noise is lower. */
        int r = (n > 65536) ? (reps / 5 > 20 ? reps / 5 : 20) : reps;
        if (r > reps) r = reps;     /* never exceed the requested repetitions */

        /* Barrier: both processes start together from this size */
        MPI_Barrier(MPI_COMM_WORLD);

        /* ---- Repetitions: the first 'warmup' are discarded, then 'r' are measured ---- */
        for (int i = 0; i < warmup + r; i++) {
            if (rank == 0) {
                /* PING: rank 0 sends and then waits for the reply. */
                double t0 = MPI_Wtime();                                      /* start */
                MPI_Send(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD);             /* send to rank 1 */
                MPI_Recv(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); /* wait for the return */
                double t1 = MPI_Wtime();                                      /* end */
                /* Store the time only after the warm-up.
                 * Divide by 2 because t1-t0 is a round trip. */
                if (i >= warmup) times[i - warmup] = (t1 - t0) / 2.0;
            } else {
                /* PONG: rank 1 receives and sends the same message back. */
                MPI_Recv(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
            }
        }

        /* ---- Statistics and printing (rank 0 only, which owns the times) ---- */
        if (rank == 0) {
            /* Arithmetic mean of the one-way times */
            double sum = 0.0, sq = 0.0;
            for (int i = 0; i < r; i++) sum += times[i];
            double mean = sum / r;

            /* Sample standard deviation (division by r-1) */
            for (int i = 0; i < r; i++) sq += (times[i] - mean) * (times[i] - mean);
            double sd = sqrt(sq / (r - 1));

            /* Sort the times to get the minimum and the median
             * (the median is robust against possible outliers) */
            qsort(times, r, sizeof(double), cmp_double);
            double tmin = times[0];     /* best case: minimum time */
            double tmed = (r % 2) ? times[r / 2]                              /* r odd: middle element */
                                  : 0.5 * (times[r / 2 - 1] + times[r / 2]);  /* r even: mean of the two middle ones */

            /* CSV row: "mean" bandwidth = n / mean time,
             *          "peak" bandwidth = n / minimum time */
            printf("%d,%d,%.9e,%.9e,%.9e,%.9e,%.6f,%.6f\n", n, r, mean, sd, tmin, tmed,
                   n / MB_BYTES / mean, n / MB_BYTES / tmin);
            fflush(stdout);     /* flush the buffer right away, so nothing is lost if the job is killed */
        }
    }

    /* ---- Cleanup and shutdown ---- */
    free(buf);
    free(times);
    MPI_Finalize();
    return 0;
}
