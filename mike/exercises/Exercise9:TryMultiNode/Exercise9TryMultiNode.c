/*
========================EXERCISE 5: CUSTOM REDUCE================================================================================
Communication hands-on
Implement a "reduce" operation by using send and receive calls.
Collect local values from all processes at a root process and compute the sum over the collected data set.

======================LOGIC:==========================================================================================================
- Define a root process (e.g., process 0) to collect and sum up data.
- Each process computes or initializes a local data value (e.g., local_val = rank + 1).
- If rank != root:
    * Send local_val to root process using MPI_Send.
- If rank == root:
    * Initialize global_sum with root's own local_val.
    * Loop through all other processes (from 0 to size - 1, skipping root).
    * Receive temp_val from process i using MPI_Recv and add it to global_sum.
    * Print the final total sum.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int root = 0; // Rank of root process that performs the reduction

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

    // Each process generates a local value based on its rank (e.g., rank + 1)
    int local_val = rank + 1;
    printf("Process %d: local value = %d\n", rank, local_val);

    if (rank != root) {
        /*
         * Worker processes send their local value to the root process
         * MPI_Send parameters breakdown:
         * 1. &local_val    : Pointer to data buffer to send
         * 2. 1            : Number of elements
         * 3. MPI_INT      : Data type
         * 4. root         : Rank of destination process
         * 5. 0            : Message tag
         * 6. MPI_COMM_WORLD: Communicator group
         */
        MPI_Send(&local_val, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
    } else {
        // Root process initializes global sum with its own local value
        int global_sum = local_val;
        int temp_val = 0;

        // Receive local values from all other processes and sum them up
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /*
                 * MPI_Recv parameters breakdown:
                 * 1. &temp_val    : Pointer to buffer where data will be stored
                 * 2. 1            : Max elements to receive
                 * 3. MPI_INT      : Data type
                 * 4. i            : Rank of expected source process
                 * 5. 0            : Message tag
                 * 6. MPI_COMM_WORLD: Communicator group
                 * 7. MPI_STATUS_IGNORE: Status struct ignore pointer
                 */
                MPI_Recv(&temp_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Process %d (root): received value %d from process %d\n", root, temp_val, i);
                global_sum += temp_val;
            }
        }

        printf("Process %d (root): Total sum (reduce result) = %d\n", root, global_sum);
    }

    MPI_Finalize();
    return 0;
}