/*
 * Point-to-point ping-pong benchmark (MPI_Send / MPI_Recv).
 *
 * Two processes bounce a message of n bytes back and forth, for n = 1, 2, 4,
 * ..., 2^max_exp. Every repetition is timed separately by rank 0 with
 * MPI_Wtime (no clock synchronization needed across nodes).
 *
 *   one-way time   t = (round-trip time) / 2
 *   bandwidth      B = n / t      [MB/s, 1 MB = 10^6 bytes]
 *
 * Usage:  mpiexec -n 2 ./benchmark [max_exp=20] [reps=200] [warmup=20]
 *
 * Output (stdout, CSV; lines starting with '#' are metadata):
 *   n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

#define MB_BYTES 1.0e6   /* bytes per "MB" in the bandwidth formula (decimal MB) */

void Get_args(int argc, char* argv[], int my_rank, MPI_Comm comm,
      int* max_exp_p /* out */, int* reps_p /* out */, int* warmup_p /* out */);
void Print_header(int max_exp /* in */, int reps /* in */, int warmup /* in */,
      MPI_Comm comm /* in */);
int  Cmp_double(const void* a /* in */, const void* b /* in */);
void Print_stats(int n /* in */, int r /* in */, double times[] /* in/out */);

int main(int argc, char* argv[]) {
   int comm_sz;   /* Number of processes */
   int my_rank;   /* My process rank     */
   int max_exp, reps, warmup;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

   if (comm_sz != 2) {
      if (my_rank == 0)
         fprintf(stderr, "This benchmark requires exactly 2 processes (got %d)\n", comm_sz);
      MPI_Finalize();
      return 1;
   }

   Get_args(argc, argv, my_rank, MPI_COMM_WORLD, &max_exp, &reps, &warmup);

   /* One buffer reused for every size, allocated once at the max size tested */
   size_t max_size = (size_t) 1 << max_exp;
   char* buf = (char*) malloc(max_size);
   double* times = (double*) malloc((size_t) reps * sizeof(double));
   if (buf == NULL || times == NULL) {
      fprintf(stderr, "Rank %d: malloc failed\n", my_rank);
      MPI_Abort(MPI_COMM_WORLD, 1);
   }

   /* Touch every page BEFORE timing: first-write page faults would skew results */
   memset(buf, my_rank + 1, max_size);

   Print_header(max_exp, reps, warmup, MPI_COMM_WORLD);

   for (int e = 0; e <= max_exp; e++) {
      int n = 1 << e;   /* current message size in bytes */

      /* Large messages (> 64 KiB) take longer per repetition and are less
       * noisy, so fewer repetitions (reps/5, at least 20) are enough */
      int r = (n > 65536) ? (reps / 5 > 20 ? reps / 5 : 20) : reps;
      if (r > reps) r = reps;

      /* Both ranks start this size together */
      MPI_Barrier(MPI_COMM_WORLD);

      /* The first 'warmup' repetitions are discarded, the next 'r' are timed */
      for (int i = 0; i < warmup + r; i++) {
         if (my_rank == 0) {
            double t0 = MPI_Wtime();
            MPI_Send(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(buf, n, MPI_BYTE, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            double t1 = MPI_Wtime();
            if (i >= warmup) times[i - warmup] = (t1 - t0) / 2.0;   /* one-way */
         } else {
            MPI_Recv(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(buf, n, MPI_BYTE, 0, 0, MPI_COMM_WORLD);
         }
      }

      if (my_rank == 0) Print_stats(n, r, times);
   }

   free(buf);
   free(times);
   MPI_Finalize();
   return 0;
}  /* main */

/*-------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Read optional command-line arguments (with defaults) and
 *            validate them
 */
void Get_args(
      int       argc      /* in  */,
      char*     argv[]    /* in  */,
      int       my_rank   /* in  */,
      MPI_Comm  comm      /* in  */,
      int*      max_exp_p /* out */,
      int*      reps_p    /* out */,
      int*      warmup_p  /* out */) {

   *max_exp_p = (argc > 1) ? atoi(argv[1]) : 20;    /* 2^20 B = 1 MiB */
   *reps_p    = (argc > 2) ? atoi(argv[2]) : 200;   /* timed repetitions */
   *warmup_p  = (argc > 3) ? atoi(argv[3]) : 20;    /* discarded repetitions */

   /* reps >= 2 because the std deviation divides by (reps - 1) */
   if (*max_exp_p < 0 || *max_exp_p > 30 || *reps_p < 2 || *warmup_p < 0) {
      if (my_rank == 0) fprintf(stderr, "Invalid arguments\n");
      MPI_Finalize();
      exit(1);
   }
}  /* Get_args */

/*-------------------------------------------------------------------
 * Function:  Print_header
 * Purpose:   Rank 1 sends its hostname to rank 0, which prints the metadata
 *            lines and the CSV header. The hostnames show whether the PBS
 *            placement (pack / scatter) is the one requested.
 */
void Print_header(
      int       max_exp  /* in */,
      int       reps     /* in */,
      int       warmup   /* in */,
      MPI_Comm  comm     /* in */) {
   int my_rank, len;
   char name[MPI_MAX_PROCESSOR_NAME] = {0};
   char name1[MPI_MAX_PROCESSOR_NAME] = {0};

   MPI_Comm_rank(comm, &my_rank);
   MPI_Get_processor_name(name, &len);

   if (my_rank == 1) {
      MPI_Send(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, 99, comm);   /* tag 99: not a benchmark msg */
   } else {
      MPI_Recv(name1, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 1, 99, comm, MPI_STATUS_IGNORE);
      printf("# rank0_host=%s rank1_host=%s same_node=%s\n", name, name1,
             strcmp(name, name1) == 0 ? "yes" : "no");
      /* MPI_Wtick: timer resolution; measured times must be >> this value */
      printf("# wtick=%.3e max_exp=%d reps=%d warmup=%d\n", MPI_Wtick(), max_exp, reps, warmup);
      printf("n_bytes,reps,t_mean_s,t_std_s,t_min_s,t_median_s,rate_mean_MBps,rate_peak_MBps\n");
   }
}  /* Print_header */

/*-------------------------------------------------------------------
 * Function:  Cmp_double
 * Purpose:   qsort comparator for doubles
 */
int Cmp_double(const void* a /* in */, const void* b /* in */) {
   double x = *(const double*) a, y = *(const double*) b;
   return (x > y) - (x < y);
}  /* Cmp_double */

/*-------------------------------------------------------------------
 * Function:  Print_stats
 * Purpose:   Compute mean, sample std deviation, min and median of the r
 *            one-way times and print one CSV row
 * Note:      times is sorted in place
 */
void Print_stats(
      int     n        /* in     */,
      int     r        /* in     */,
      double  times[]  /* in/out */) {
   double sum = 0.0, sq = 0.0;

   for (int i = 0; i < r; i++) sum += times[i];
   double mean = sum / r;

   for (int i = 0; i < r; i++) sq += (times[i] - mean) * (times[i] - mean);
   double sd = sqrt(sq / (r - 1));

   /* Sorted times give min and median (median is robust to outliers) */
   qsort(times, r, sizeof(double), Cmp_double);
   double tmin = times[0];
   double tmed = (r % 2) ? times[r / 2]
                         : 0.5 * (times[r / 2 - 1] + times[r / 2]);

   /* mean bandwidth = n / mean time ; peak bandwidth = n / min time */
   printf("%d,%d,%.9e,%.9e,%.9e,%.9e,%.6f,%.6f\n", n, r, mean, sd, tmin, tmed,
          n / MB_BYTES / mean, n / MB_BYTES / tmin);
   fflush(stdout);   /* nothing lost if the job is killed */
}  /* Print_stats */
