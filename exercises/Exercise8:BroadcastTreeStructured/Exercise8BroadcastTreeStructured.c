/*
Implement a “broadcast” by using send and receive
*//*
========================EXERCISE 8: CUSTOM BROADCAST=============================================================================
Communication hands-on
Implement a "broadcast" operation using point-to-point MPI send and receive calls.
Process "p" (root) shares the content of a variable "a" with all other processes.

======================LOGIC:==========================================================================================================
- Define a root process "root" (e.g., process 0) that holds the payload variable "a".
- If rank == root:
    1. Initialize variable "a".
    2. Loop over all other processes (from 0 to size - 1, skipping root itself).
    3. Send the content of variable "a" to each process using MPI_Send.
- Else (rank != root):
    1. Receive the content of variable "a" directly from process "root" using MPI_Recv.
    2. Print the received value to confirm broadcast receipt.
*/

#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int a = 0;
    int root = 0; // Rank of root process performing the broadcast

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

    if (rank == root) {
        // Root process initializes payload variable "a"
        a = 100; // Payload value
        printf("Process %d (root): broadcasting variable a = %d to all processes\n", root, a);

        // Iterate through all processes and send data to everyone except root
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /* 
                 * MPI_Send parameters breakdown:
                 * 1. &a           : Pointer to buffer containing data
                 * 2. 1            : Number of elements
                 * 3. MPI_INT      : Data type
                 * 4. i            : Destination process rank
                 * 5. 0            : Message tag
                 * 6. MPI_COMM_WORLD: Communicator group
                 */
                MPI_Send(&a, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                printf("Process %d (root): sent variable a = %d to process %d\n", root, a, i);
            }
        }
    } else {
        // Worker processes receive broadcast data directly from root
        /* 
         * MPI_Recv parameters breakdown:
         * 1. &a           : Pointer to buffer where data will be saved
         * 2. 1            : Max elements to receive
         * 3. MPI_INT      : Data type
         * 4. root         : Source process rank expected
         * 5. 0            : Message tag
         * 6. MPI_COMM_WORLD: Communicator group
         * 7. MPI_STATUS_IGNORE: Status struct ignore pointer
         */
        MPI_Recv(&a, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: received broadcast variable a = %d from root process %d\n", rank, a, root);
    }

    MPI_Finalize();
    return 0;
}