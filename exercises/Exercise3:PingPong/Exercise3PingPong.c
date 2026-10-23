/*
========================EXERCISE 3: PING PONG===================================================================================
Communication hands-on
Implement a "ping pong" using MPI send and receive.
Two processes (e.g., rank 0 and rank 1) exchange a message back and forth.

======================LOGIC:==========================================================================================================
- Initialize MPI environment and obtain rank and size.
- Ensure at least 2 processes are present in MPI_COMM_WORLD.
- Set a limit for the number of bounces (e.g., PING_PONG_LIMIT = 10).
- Process 0 and Process 1 alternate sending and receiving a counter (`ping_pong_count`):
    * The sender increments `ping_pong_count` and uses MPI_Send to pass it to the partner.
    * The receiver waits using MPI_Recv to get the updated `ping_pong_count`.
- Non-participating processes (rank >= 2) remain idle.
*/

#include <mpi.h>
#include <stdio.h>

#define PING_PONG_LIMIT 10

int main(int argc, char** argv) {
    int rank, size;

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

    int ping_pong_count = 0;
    int partner_rank = (rank == 0) ? 1 : 0;

    // Only rank 0 and rank 1 participate in the ping pong exchange
    if (rank == 0 || rank == 1) {
        while (ping_pong_count < PING_PONG_LIMIT) {
            if (rank == ping_pong_count % 2) {
                ping_pong_count++;                      // Sender process: increment count and send to partner
                /*
                 * MPI_Send parameters breakdown:
                 * 1. &ping_pong_count : Pointer to the payload buffer
                 * 2. 1                 : Send 1 integer element
                 * 3. MPI_INT           : Data type of element
                 * 4. partner_rank      : Rank of destination process (0 or 1)
                 * 5. 0                 : Tag identifier
                 * 6. MPI_COMM_WORLD    : Communicator group
                 */
                MPI_Send(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD);
                printf("Process %d sent ping_pong_count = %d to Process %d\n", rank, ping_pong_count, partner_rank);
            } else {
                // Receiver process: wait for payload from partner
                /*
                 * MPI_Recv parameters breakdown:
                 * 1. &ping_pong_count : Pointer to buffer where data will be saved
                 * 2. 1                 : Max elements to receive
                 * 3. MPI_INT           : Data type
                 * 4. partner_rank      : Rank of source process expected
                 * 5. 0                 : Tag identifier matching sender
                 * 6. MPI_COMM_WORLD    : Communicator group
                 * 7. MPI_STATUS_IGNORE : Status struct ignore pointer
                 */
                MPI_Recv(&ping_pong_count, 1, MPI_INT, partner_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Process %d received ping_pong_count = %d from Process %d\n", rank, ping_pong_count, partner_rank);
            }
        }
    } else {
        printf("Process %d: Idle (Ping Pong is between rank 0 and rank 1)\n", rank);
    }

    MPI_Finalize();
    return 0;
}