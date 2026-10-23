/*
========================EXERCISE 4: RING COMMUNICATION==========================================================================
Communication hands-on
Implement a "ring" topology by using send and receive calls.
Each process passes a message/token to its right neighbor, forming a logical ring.

======================LOGIC:==========================================================================================================
- PHASE 1: Initiation & Propagation
    * Process 0 initiates the ring by sending the initial token to Process 1.
    * All other processes (rank > 0) receive from their left neighbor (prev), process/update the token, and send it to their right neighbor (next).
- PHASE 2: Completion
    * Process 0 receives the final token back from the last process (size - 1) and prints the result.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int token = 0;

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

    // Identify left and right neighbors in the ring topology
    int next = (rank + 1) % size;
    int prev = (rank - 1 + size) % size;

    // PHASE 1: Ring initiation (rank 0) and token propagation (ranks 1 to size - 1)
    if (rank == 0) {
        token = 100; // Initial token payload value
        printf("Process 0: Starting ring with token = %d\n", token);

        /* 
         * Process 0 sends the initial token to Process 1
         */
        MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        printf("Process 0: sent token = %d to process %d\n", token, next);
    } else {
        /*
         * Processes 1 to size-1 wait to receive from their left neighbor (prev),
         * update the token, and forward it to their right neighbor (next).
         */
        MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: received token = %d from process %d\n", rank, token, prev);

        token += rank; // Modify payload to track traversal along the ring

        MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        printf("Process %d: sent updated token = %d to process %d\n", rank, token, next);
    }

    // PHASE 2: Ring completion
    if (rank == 0) {
        /* 
         * Process 0 waits to receive the final token back from the last process (prev = size - 1)
         */
        MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process 0: received completed ring token = %d from process %d\n", token, prev);
    }

    MPI_Finalize();
    return 0;
}