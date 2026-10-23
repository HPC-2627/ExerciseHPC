/*
========================EXERCISE=================================================================================================
Communication hands-on
Design a broadcast solution by using send and receive calls
What we expect: a process "p" wants to share the content of a variable "a" with all the other processes.


======================LOGIC:==========================================================================================================
- Define a root process "p" (e.g., process 0) that holds the variable "a".
- If you are Process "p" (rank == p):
    1. Initialize the variable "a".
    2. Loop over all other processes (from 0 to size - 1, skipping process p itself).
    3. Send the content of variable "a" to each process using MPI_Send.
- Else (rank != p):
    1. Receive the content of variable "a" directly from process "p" using MPI_Recv.
    2. Print the received value to confirm receipt.
*/
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int a = 0;
    int root = 0; // Rank of process "p" that performs the broadcast

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
        // Process "p" initializes variable "a"
        a = 42; // Example payload value
        printf("Process %d (root): broadcasting variable a = %d to all other processes\n", root, a);

        // Iterate through all processes and send data to everyone except self
        for (int i = 0; i < size; i++) {
            if (i != root) {
                /* 
                 * MPI_Send parameters breakdown:
                 * 1. &a           : Pointer to the buffer containing the data to send
                 * 2. 1            : Number of elements to send
                 * 3. MPI_INT      : Type of the elements in the buffer
                 * 4. i            : Rank of the destination process (target)
                 * 5. 0            : Message tag (used to distinguish different types of messages)
                 * 6. MPI_COMM_WORLD: Communicator (the scope/group of processes)
                 */
                MPI_Send(&a, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                printf("Process %d: sent variable a = %d to process %d\n", root, a, i);
            }
        }
    } else {
        // All other processes receive data directly from process "p"
        
        /* 
         * MPI_Recv parameters breakdown:
         * 1. &a           : Pointer to the buffer where the received data will be stored
         * 2. 1            : Maximum number of elements to receive
         * 3. MPI_INT      : Type of the elements in the buffer
         * 4. root         : Rank of the source process (who we expect the message from)
         * 5. 0            : Message tag (must match the sender's tag)
         * 6. MPI_COMM_WORLD: Communicator
         * 7. MPI_STATUS_IGNORE: Pointer to a status object to get info about the message (ignored here)
         */
        MPI_Recv(&a, 1, MPI_INT, root, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: received variable a = %d from process %d\n", rank, a, root);
    }

    MPI_Finalize();
    return 0;
}