/*
========================EXERCISE=================================================================================================
Communication hand-ons
Implement a ring by using send and receives
Write a program that takes data from process zero and sends it to all of the other processes by sending it in a ring.
That is, process i should receive the data and send it to process i+1, until the last process is reached


======================LOGIC:==========================================================================================================
- If you are Process 0 (rank == 0): You start, initialize the data, and shoot it to Process 1 (MPI_Send to 1). Done.
- else:
    1. Everyone receives the data from their predecessor (rank - 1).
    2. You check: If you are not the last one (rank < size - 1), you immediately forward it to the next one (rank + 1).
    3. If you are the last one, you don't forward it to anyone: you receive the data, print that you've reached the end of the chain, and "that's it" (or "and done").
*/
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    int rank, size;
    int data = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //CHECK number of processes, to avoid possible stupid error
    if (size < 2) {
        if (rank == 0) {
            printf("Error: This program requires at least 2 processes.\n");
        }
        MPI_Finalize();
        return 0;
    }

    if (rank == 0) {
        // Process 0 initializes the data and sends it to process 1
        data = 10 ; // Example value
        printf("Process 0: sending data %d to process 1\n", data);
        
        /* 
         * MPI_Send parameters breakdown:
         * 1. &data        : Pointer to the buffer containing the data to send
         * 2. 1            : Number of elements to send
         * 3. MPI_INT      : Type of the elements in the buffer
         * 4. 1            : Rank of the destination process (target)
         * 5. 0            : Message tag (used to distinguish different types of messages)
         * 6. MPI_COMM_WORLD: Communicator (the scope/group of processes)
         */
        MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        
    } else {
        // Intermediate and last processes receive from the previous process
        
        /* 
         * MPI_Recv parameters breakdown:
         * 1. &data        : Pointer to the buffer where the received data will be stored
         * 2. 1            : Maximum number of elements to receive
         * 3. MPI_INT      : Type of the elements in the buffer
         * 4. rank - 1     : Rank of the source process (who we expect the message from)
         * 5. 0            : Message tag (must match the sender's tag)
         * 6. MPI_COMM_WORLD: Communicator
         * 7. MPI_STATUS_IGNORE: Pointer to a status object to get info about the message (ignored here)
         */
        MPI_Recv(&data, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d: received data %d from process %d\n", rank, data, rank - 1);

        // If we are not the last process, forward it to the next one
        if (rank < size - 1) {
            /* 
             * MPI_Send parameters breakdown (same as above):
             * 1. &data        : Buffer to send
             * 2. 1            : Element count
             * 3. MPI_INT      : Data type
             * 4. rank + 1     : Destination process rank (next process in the ring)
             * 5. 0            : Message tag
             * 6. MPI_COMM_WORLD: Communicator
             */
            MPI_Send(&data, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            printf("Process %d: forwarded data %d to process %d\n", rank, data, rank + 1);
        } else {
            printf("Process %d (last): end of the chain, final data received = %d\n", rank, data);
        }
    }

    MPI_Finalize();
    return 0;
}