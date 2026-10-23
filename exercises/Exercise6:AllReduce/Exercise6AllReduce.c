/*
========================EXERCISE 5: CUSTOM REDUCE & ALLREDUCE===================================================================
Communication hands-on
1) Implement a "reduce" operation using MPI_Send and MPI_Recv to collect local data at a root process
   and compute reduction operations (sum, min, max, average).
2) Implement an "allreduce" operation by distributing the final reduced result back from the root process 
   to all other processes so everyone shares the result.

======================LOGIC:==========================================================================================================
- PHASE 1: REDUCE (Collect & Aggregate at Root)
    * Each process initializes a local value (e.g., local_val = rank + 1).
    * Workers (rank != root) send local_val to root using MPI_Send.
    * Root (rank == root) initializes global_sum, global_min, global_max with its own local_val.
    * Root loops over all workers, receives temp_val via MPI_Recv, and updates total sum, min, max.
    * Root computes average = global_sum / size.

- PHASE 2: ALLREDUCE (Distribute Reduced Result back to All Processes)
    * Root loops over all workers and sends global_sum to each using MPI_Send.
    * Workers receive global_sum from root using MPI_Recv.
    * Now all processes hold global_sum and print their confirmed value.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int root = 0; // Rank of process acting as collector/distributor

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // CHECK number of processes to avoid execution errors
    if (size < 2) {
        if (rank == 0) {
            printf("Error: This program requires at least 2 processes.\n");
        }
        MPI_Finalize();
        return 0;
    }

    int local_val = rank + 1; 
    int global_sum = 0;
    int global_min = 0;
    int global_max = 0;
    double global_avg = 0.0;

    // ==========================================
    // PHASE 1: CUSTOM REDUCE (Workers -> Root)
    // ==========================================
    if (rank != root) {
        /*
         * Workers send their local payload to root process
         * MPI_Send parameters breakdown:
         * 1. &local_val    : Pointer to payload buffer
         * 2. 1            : Number of elements
         * 3. MPI_INT      : Data type
         * 4. root         : Destination process rank
         * 5. 0            : Message tag
         * 6. MPI_COMM_WORLD: Communicator group
         */
        MPI_Send(&local_val, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
    } else {
        // Root process initializes reduction metrics with its own payload
        global_sum = local_val;
        global_min = local_val;
        global_max = local_val;

        int temp_val = 0;
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /*
                 * Root receives payload from worker i
                 */
                MPI_Recv(&temp_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Aggregate reduction operations
                global_sum += temp_val;
                if (temp_val < global_min) global_min = temp_val;
                if (temp_val > global_max) global_max = temp_val;
            }
        }
        
        // Calculate average over all collected data
        global_avg = (double)global_sum / size;
        
        printf("--- REDUCE COMPLETED AT ROOT (Process %d) ---\n", root);
        printf("Global Sum = %d | Min = %d | Max = %d | Avg = %.2f\n\n", global_sum, global_min, global_max, global_avg);
    }

    // ==========================================
    // PHASE 2: CUSTOM ALLREDUCE (Root -> All)
    // ==========================================
    if (rank == root) {
        /*
         * Root broadcasts/sends the reduced result to all worker processes
         */
        for (int i = 0; i < size; i++) {
            if (i != root) {
                MPI_Send(&global_sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        /*
         * Worker processes receive the final aggregated result from root
         */
        MPI_Recv(&global_sum, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Confirm that ALL processes now possess the same reduced result
    printf("Process %d (ALLREDUCE confirmed): global_sum = %d\n", rank, global_sum);

    MPI_Finalize();
    return 0;
}